/*
=== Kraken Decompressor for Windows ===
Copyright (C) 2016, Powzix

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER

#include <stdlib.h>
#define bswap_16(x) _byteswap_ushort(x)
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)

#elif defined(__APPLE__)

// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define bswap_16(x) OSSwapInt16(x)
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)

#elif defined(__sun) || defined(sun)

#include <sys/byteorder.h>
#define bswap_16(x) BSWAP_16(x)
#define bswap_32(x) BSWAP_32(x)
#define bswap_64(x) BSWAP_64(x)

#elif defined(__FreeBSD__)

#include <sys/endian.h>
#define bswap_16(x) bswap16(x)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)

#elif defined(__OpenBSD__)

#include <sys/types.h>
#define bswap_16(x) swap16(x)
#define bswap_32(x) swap32(x)
#define bswap_64(x) swap64(x)

#elif defined(__NetBSD__)

#include <machine/bswap.h>
#include <sys/types.h>
#if defined(__BSWAP_RENAME) && !defined(__bswap_32)
#define bswap_16(x) bswap16(x)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)
#endif

#else

#include <byteswap.h>

#endif

static unsigned char bit_scan_forward(unsigned long *index, unsigned long mask)
{
#if defined(_MSC_VER)
	return _BitScanForward(index, mask);
#elif defined(__GNUC__) || defined(__clang__)
	if (mask == 0)
		return 0;
	*index = (unsigned long)__builtin_ctzl(mask);
	return 1;
#else
	if (mask == 0)
		return 0;

	*index = 0;
	while ((mask & 1) == 0)
	{
		mask >>= 1;
		(*index)++;
	}
	return 1;
#endif
}

static unsigned char bit_scan_reverse(unsigned long *index, unsigned long mask)
{
#if defined(_MSC_VER)
	return _BitScanReverse(index, mask);
#elif defined(__GNUC__) || defined(__clang__)
	if (mask == 0)
		return 0;
	*index =
		(unsigned long)(sizeof(unsigned long) * 8 - 1 - __builtin_clzl(mask));
	return 1;
#else
	if (mask == 0)
		return 0;

	*index = sizeof(unsigned long) * 8 - 1;
	while ((mask & (1UL << *index)) == 0)
		(*index)--;
	return 1;
#endif
}

static int rotl(unsigned int x, int r)
{
#ifdef _MSC_VER
	return _rotl(x, r);
#else
	return (x << r) | (x >> (32 - r));
#endif
}

// Header in front of each 256k block
typedef struct KrakenHeader
{
	// Type of decoder used, 6 means kraken
	int decoder_type;

	// Whether to restart the decoder
	bool restart_decoder;

	// Whether this block is uncompressed
	bool uncompressed;

	// Whether this block uses checksums.
	bool use_checksums;
} KrakenHeader;

// Additional header in front of each 256k block ("quantum").
typedef struct KrakenQuantumHeader
{
	// The compressed size of this quantum. If this value is 0 it means
	// the quantum is a special quantum such as memset.
	uint32_t compressed_size;
	// If checksums are enabled, holds the checksum.
	uint32_t checksum;
	// Two flags
	uint8_t flag1;
	uint8_t flag2;
	// Whether the whole block matched a previous block
	uint32_t whole_match_distance;
} KrakenQuantumHeader;

// Kraken decompression happens in two phases, first one decodes
// all the literals and copy lengths using huffman and second
// phase runs the copy loop. This holds the tables needed by stage 2.
typedef struct KrakenLzTable
{
	// Stream of (literal, match) pairs. The flag uint8_t contains
	// the length of the match, the length of the literal and whether
	// to use a recent offset.
	uint8_t *cmd_stream;
	int cmd_stream_size;

	// Holds the actual distances in case we're not using a recent
	// offset.
	int *offs_stream;
	int offs_stream_size;

	// Holds the sequence of literals. All literal copying happens from
	// here.
	uint8_t *lit_stream;
	int lit_stream_size;

	// Holds the lengths that do not fit in the flag stream. Both literal
	// lengths and match length are stored in the same array.
	int *len_stream;
	int len_stream_size;
} KrakenLzTable;

typedef struct KrakenDecoder
{
	// Updated after the |*_DecodeStep| function completes to hold
	// the number of bytes read and written.
	int src_used, dst_used;

	// Pointer to a 256k buffer that holds the intermediate state
	// in between decode phase 1 and 2.
	uint8_t *scratch;
	size_t scratch_size;

	KrakenHeader hdr;
} KrakenDecoder;

typedef struct BitReader
{
	// |p| holds the current uint8_t and |p_end| the end of the buffer.
	const uint8_t *p, *p_end;
	// Bits accumulated so far
	uint32_t bits;
	// Next uint8_t will end up in the |bitpos| position in |bits|.
	int bitpos;
} BitReader;

typedef struct
{
	uint8_t bits2len[2048];
	uint8_t bits2sym[2048];
} HuffRevLut;

typedef struct HuffReader
{
	// Array to hold the output of the huffman read array operation
	uint8_t *output, *output_end;
	// We decode three parallel streams, two forwards, |src| and |src_mid|
	// while |src_end| is decoded backwards.
	const uint8_t *src, *src_mid, *src_end, *src_mid_org;
	int src_bitpos, src_mid_bitpos, src_end_bitpos;
	uint32_t src_bits, src_mid_bits, src_end_bits;
} HuffReader;

inline size_t Max(size_t a, size_t b)
{
	return a > b ? a : b;
}
inline size_t Min(size_t a, size_t b)
{
	return a < b ? a : b;
}

#define ALIGN_POINTER(p, align)                                               \
	((uint8_t *)(((uintptr_t)(p) + (align - 1)) & ~(align - 1)))

typedef struct
{
	uint16_t symbol;
	uint16_t num;
} HuffRange;

int Kraken_DecodeBytes(
	uint8_t **output, const uint8_t *src, const uint8_t *src_end,
	int *decoded_size, size_t output_size, bool force_memmove,
	uint8_t *scratch, uint8_t *scratch_end);
int Kraken_GetBlockSize(
	const uint8_t *src, const uint8_t *src_end, int *dest_size,
	int dest_capacity);
int Huff_ConvertToRanges(
	HuffRange *range, int num_symbols, int P, const uint8_t *symlen,
	BitReader *bits);

// Allocate memory with a specific alignment
void *MallocAligned(size_t size, size_t alignment)
{
	void *x = malloc(size + (alignment - 1) + sizeof(void *)), *x_org = x;
	if (x)
	{
		x = (void *)(((intptr_t)x + alignment - 1 + sizeof(void *)) &
					 ~(alignment - 1));
		((void **)x)[-1] = x_org;
	}
	return x;
}

// Free memory allocated through |MallocAligned|
void FreeAligned(void *p)
{
	free(((void **)p)[-1]);
}

uint32_t BSR(uint32_t x)
{
	unsigned long index = ~0u;
	bit_scan_reverse(&index, x);
	return (uint32_t)index;
}

uint32_t BSF(uint32_t x)
{
	unsigned long index = ~0u;
	bit_scan_forward(&index, x);
	return (uint32_t)index;
}

// Read more bytes to make sure we always have at least 24 bits in |bits|.
static void BitReader_Refill(BitReader *bits)
{
	assert(bits->bitpos <= 24);
	while (bits->bitpos > 0)
	{
		bits->bits |= (bits->p < bits->p_end ? *bits->p : 0) << bits->bitpos;
		bits->bitpos -= 8;
		bits->p++;
	}
}

// Read more bytes to make sure we always have at least 24 bits in |bits|,
// used when reading backwards.
void BitReader_RefillBackwards(BitReader *bits)
{
	assert(bits->bitpos <= 24);
	while (bits->bitpos > 0)
	{
		bits->p--;
		bits->bits |= (bits->p >= bits->p_end ? *bits->p : 0) << bits->bitpos;
		bits->bitpos -= 8;
	}
}

// Refill bits then read a single bit.
int BitReader_ReadBit(BitReader *bits)
{
	int r;
	BitReader_Refill(bits);
	r = bits->bits >> 31;
	bits->bits <<= 1;
	bits->bitpos += 1;
	return r;
}

int BitReader_ReadBitNoRefill(BitReader *bits)
{
	int r;
	r = bits->bits >> 31;
	bits->bits <<= 1;
	bits->bitpos += 1;
	return r;
}

// Read |n| bits without refilling.
int BitReader_ReadBitsNoRefill(BitReader *bits, int n)
{
	int r = (bits->bits >> (32 - n));
	bits->bits <<= n;
	bits->bitpos += n;
	return r;
}

// Read |n| bits without refilling, n may be zero.
int BitReader_ReadBitsNoRefillZero(BitReader *bits, int n)
{
	int r = (bits->bits >> 1 >> (31 - n));
	bits->bits <<= n;
	bits->bitpos += n;
	return r;
}

uint32_t BitReader_ReadMoreThan24Bits(BitReader *bits, int n)
{
	uint32_t rv;
	if (n <= 24)
	{
		rv = BitReader_ReadBitsNoRefillZero(bits, n);
	}
	else
	{
		rv = BitReader_ReadBitsNoRefill(bits, 24) << (n - 24);
		BitReader_Refill(bits);
		rv += BitReader_ReadBitsNoRefill(bits, n - 24);
	}
	BitReader_Refill(bits);
	return rv;
}

uint32_t BitReader_ReadMoreThan24BitsB(BitReader *bits, int n)
{
	uint32_t rv;
	if (n <= 24)
	{
		rv = BitReader_ReadBitsNoRefillZero(bits, n);
	}
	else
	{
		rv = BitReader_ReadBitsNoRefill(bits, 24) << (n - 24);
		BitReader_RefillBackwards(bits);
		rv += BitReader_ReadBitsNoRefill(bits, n - 24);
	}
	BitReader_RefillBackwards(bits);
	return rv;
}

// Reads a gamma value.
// Assumes bitreader is already filled with at least 23 bits
int BitReader_ReadGamma(BitReader *bits)
{
	unsigned long bitresult;
	int n;
	int r;
	if (bits->bits != 0)
	{
		bit_scan_reverse(&bitresult, bits->bits);
		n = (int)(31 - bitresult);
	}
	else
	{
		n = 32;
	}
	n = 2 * n + 2;
	assert(n < 24);
	bits->bitpos += n;
	r = bits->bits >> (32 - n);
	bits->bits <<= n;
	return r - 2;
}

int CountLeadingZeros(uint32_t bits)
{
	unsigned long x = 32;
	bit_scan_reverse(&x, bits);
	return (int)(31 - x);
}

// Reads a gamma value with |forced| number of forced bits.
int BitReader_ReadGammaX(BitReader *bits, int forced)
{
	unsigned long bitresult;
	int r;
	if (bits->bits != 0)
	{
		bit_scan_reverse(&bitresult, bits->bits);
		int lz = (int)(31 - bitresult);
		assert(lz < 24);
		r = (bits->bits >> (31 - lz - forced)) + ((lz - 1) << forced);
		bits->bits <<= lz + forced + 1;
		bits->bitpos += lz + forced + 1;
		return r;
	}
	return 0;
}

// Reads a offset code parametrized by |v|.
uint32_t BitReader_ReadDistance(BitReader *bits, uint32_t v)
{
	uint32_t w, m, n, rv;
	if (v < 0xF0)
	{
		n = (v >> 4) + 4;
		w = rotl(bits->bits | 1, n);
		bits->bitpos += n;
		m = (2 << n) - 1;
		bits->bits = w & ~m;
		rv = ((w & m) << 4) + (v & 0xF) - 248;
	}
	else
	{
		n = v - 0xF0 + 4;
		w = rotl(bits->bits | 1, n);
		bits->bitpos += n;
		m = (2 << n) - 1;
		bits->bits = w & ~m;
		rv = 8322816 + ((w & m) << 12);
		BitReader_Refill(bits);
		rv += (bits->bits >> 20);
		bits->bitpos += 12;
		bits->bits <<= 12;
	}
	BitReader_Refill(bits);
	return rv;
}

// Reads a offset code parametrized by |v|, backwards.
uint32_t BitReader_ReadDistanceB(BitReader *bits, uint32_t v)
{
	uint32_t w, m, n, rv;
	if (v < 0xF0)
	{
		n = (v >> 4) + 4;
		w = rotl(bits->bits | 1, n);
		bits->bitpos += n;
		m = (2 << n) - 1;
		bits->bits = w & ~m;
		rv = ((w & m) << 4) + (v & 0xF) - 248;
	}
	else
	{
		n = v - 0xF0 + 4;
		w = rotl(bits->bits | 1, n);
		bits->bitpos += n;
		m = (2 << n) - 1;
		bits->bits = w & ~m;
		rv = 8322816 + ((w & m) << 12);
		BitReader_RefillBackwards(bits);
		rv += (bits->bits >> (32 - 12));
		bits->bitpos += 12;
		bits->bits <<= 12;
	}
	BitReader_RefillBackwards(bits);
	return rv;
}

// Reads a length code.
bool BitReader_ReadLength(BitReader *bits, uint32_t *v)
{
	unsigned long bitresult = 32;
	int n;
	uint32_t rv;
	bit_scan_reverse(&bitresult, bits->bits);
	n = (int)(31 - bitresult);
	if (n > 12)
		return false;
	bits->bitpos += n;
	bits->bits <<= n;
	BitReader_Refill(bits);
	n += 7;
	bits->bitpos += n;
	rv = (bits->bits >> (32 - n)) - 64;
	bits->bits <<= n;
	*v = rv;
	BitReader_Refill(bits);
	return true;
}

// Reads a length code, backwards.
bool BitReader_ReadLengthB(BitReader *bits, uint32_t *v)
{
	unsigned long bitresult = 32;
	uint32_t rv;
	bit_scan_reverse(&bitresult, bits->bits);
	int n = (int)(31 - bitresult);
	if (n > 12)
		return false;
	bits->bitpos += n;
	bits->bits <<= n;
	BitReader_RefillBackwards(bits);
	n += 7;
	bits->bitpos += n;
	rv = (bits->bits >> (32 - n)) - 64;
	bits->bits <<= n;
	*v = rv;
	BitReader_RefillBackwards(bits);
	return true;
}

int Log2RoundUp(uint32_t v)
{
	if (v > 1)
	{
		unsigned long idx;
		bit_scan_reverse(&idx, v - 1);
		return (int)(idx + 1);
	}
	else
	{
		return 0;
	}
}

#define ALIGN_16(x) (((x) + 15) & ~15)
// This used to be "{*(uint64_t*)(d) = *(uint64_t*)(s); }" but that breaks GCC
// auto-vectorization
#define COPY_64(d, s)                                                         \
	{                                                                         \
		memcpy(d, s, 8);                                                      \
	}
static inline void COPY_64_BYTES(uint8_t *d, const uint8_t *s)
{
	memcpy(d, s, 64);
}

static inline void COPY_64_ADD(uint8_t *d, const uint8_t *s, const uint8_t *t)
{
	for (int i = 0; i < 8; ++i)
	{
		d[i] = (uint8_t)(s[i] + t[i]);
	}
}

KrakenDecoder *Kraken_Create(void)
{
	size_t scratch_size = 0x6C000;
	size_t memory_needed = sizeof(KrakenDecoder) + scratch_size;
	KrakenDecoder *dec = (KrakenDecoder *)MallocAligned(memory_needed, 16);
	memset(dec, 0, sizeof(KrakenDecoder));
	dec->scratch_size = scratch_size;
	dec->scratch = (uint8_t *)(dec + 1);
	return dec;
}

void Kraken_Destroy(KrakenDecoder *kraken)
{
	FreeAligned(kraken);
}

const uint8_t *Kraken_ParseHeader(KrakenHeader *hdr, const uint8_t *p)
{
	int b = p[0];
	if ((b & 0xF) == 0xC)
	{
		if (((b >> 4) & 3) != 0)
			return NULL;
		hdr->restart_decoder = (b >> 7) & 1;
		hdr->uncompressed = (b >> 6) & 1;
		b = p[1];
		hdr->decoder_type = b & 0x7F;
		hdr->use_checksums = !!(b >> 7);
		if (hdr->decoder_type != 6 && hdr->decoder_type != 10 &&
			hdr->decoder_type != 5 && hdr->decoder_type != 11 &&
			hdr->decoder_type != 12)
			return NULL;
		return p + 2;
	}

	return NULL;
}

const uint8_t *Kraken_ParseQuantumHeader(
	KrakenQuantumHeader *hdr, const uint8_t *p, bool use_checksum)
{
	uint32_t v = (p[0] << 16) | (p[1] << 8) | p[2];
	uint32_t size = v & 0x3FFFF;
	if (size != 0x3ffff)
	{
		hdr->compressed_size = size + 1;
		hdr->flag1 = (v >> 18) & 1;
		hdr->flag2 = (v >> 19) & 1;
		if (use_checksum)
		{
			hdr->checksum = (p[3] << 16) | (p[4] << 8) | p[5];
			return p + 6;
		}
		else
		{
			return p + 3;
		}
	}
	v >>= 18;
	if (v == 1)
	{
		// memset
		hdr->checksum = p[3];
		hdr->compressed_size = 0;
		hdr->whole_match_distance = 0;
		return p + 4;
	}
	return NULL;
}

const uint8_t *LZNA_ParseWholeMatchInfo(const uint8_t *p, uint32_t *dist)
{
	uint32_t v = bswap_16(*(const uint16_t *)p);

	if (v < 0x8000)
	{
		uint32_t x = 0, b, pos = 0;
		for (;;)
		{
			b = p[2];
			p += 1;
			if (b & 0x80)
				break;
			x += (b + 0x80) << pos;
			pos += 7;
		}
		x += (b - 128) << pos;
		*dist = 0x8000 + v + (x << 15) + 1;
		return p + 2;
	}
	else
	{
		*dist = v - 0x8000 + 1;
		return p + 2;
	}
}

const uint8_t *LZNA_ParseQuantumHeader(
	KrakenQuantumHeader *hdr, const uint8_t *p, bool use_checksum, int raw_len)
{
	uint32_t v = (p[0] << 8) | p[1];
	uint32_t size = v & 0x3FFF;
	if (size != 0x3fff)
	{
		hdr->compressed_size = size + 1;
		hdr->flag1 = (v >> 14) & 1;
		hdr->flag2 = (v >> 15) & 1;
		if (use_checksum)
		{
			hdr->checksum = (p[2] << 16) | (p[3] << 8) | p[4];
			return p + 5;
		}
		else
		{
			return p + 2;
		}
	}
	v >>= 14;
	if (v == 0)
	{
		p = LZNA_ParseWholeMatchInfo(p + 2, &hdr->whole_match_distance);
		hdr->compressed_size = 0;
		return p;
	}
	if (v == 1)
	{
		// memset
		hdr->checksum = p[2];
		hdr->compressed_size = 0;
		hdr->whole_match_distance = 0;
		return p + 3;
	}
	if (v == 2)
	{
		// uncompressed
		hdr->compressed_size = raw_len;
		return p + 2;
	}
	return NULL;
}

uint32_t Kraken_GetCrc(const uint8_t *p, size_t p_size)
{
	// TODO: implement
	(void)p;
	(void)p_size;
	return 0;
}

// Rearranges elements in the input array so that bits in the index
// get flipped.
static void ReverseBitsArray2048(const uint8_t *input, uint8_t *output)
{
	for (int i = 0; i < 2048; i++)
	{
		int rev = 0;
		int x = i;
		for (int b = 0; b < 11; b++)
		{
			rev = (rev << 1) | (x & 1);
			x >>= 1;
		}
		output[rev] = input[i];
	}
}

bool Kraken_DecodeBytesCore(HuffReader *hr, HuffRevLut *lut)
{
	const uint8_t *src = hr->src;
	uint32_t src_bits = hr->src_bits;
	int src_bitpos = hr->src_bitpos;

	const uint8_t *src_mid = hr->src_mid;
	uint32_t src_mid_bits = hr->src_mid_bits;
	int src_mid_bitpos = hr->src_mid_bitpos;

	const uint8_t *src_end = hr->src_end;
	uint32_t src_end_bits = hr->src_end_bits;
	int src_end_bitpos = hr->src_end_bitpos;

	int k, n;

	uint8_t *dst = hr->output;
	uint8_t *dst_end = hr->output_end;

	if (src > src_mid)
		return false;

	if (hr->src_end - src_mid >= 4 && dst_end - dst >= 6)
	{
		dst_end -= 5;
		src_end -= 4;

		while (dst < dst_end && src <= src_mid && src_mid <= src_end)
		{
			src_bits |= *(const uint32_t *)src << src_bitpos;
			src += (31 - src_bitpos) >> 3;

			src_end_bits |= bswap_32(*(const uint32_t *)src_end)
							<< src_end_bitpos;
			src_end -= (31 - src_end_bitpos) >> 3;

			src_mid_bits |= *(const uint32_t *)src_mid << src_mid_bitpos;
			src_mid += (31 - src_mid_bitpos) >> 3;

			src_bitpos |= 0x18;
			src_end_bitpos |= 0x18;
			src_mid_bitpos |= 0x18;

			k = src_bits & 0x7FF;
			n = lut->bits2len[k];
			src_bits >>= n;
			src_bitpos -= n;
			dst[0] = lut->bits2sym[k];

			k = src_end_bits & 0x7FF;
			n = lut->bits2len[k];
			src_end_bits >>= n;
			src_end_bitpos -= n;
			dst[1] = lut->bits2sym[k];

			k = src_mid_bits & 0x7FF;
			n = lut->bits2len[k];
			src_mid_bits >>= n;
			src_mid_bitpos -= n;
			dst[2] = lut->bits2sym[k];

			k = src_bits & 0x7FF;
			n = lut->bits2len[k];
			src_bits >>= n;
			src_bitpos -= n;
			dst[3] = lut->bits2sym[k];

			k = src_end_bits & 0x7FF;
			n = lut->bits2len[k];
			src_end_bits >>= n;
			src_end_bitpos -= n;
			dst[4] = lut->bits2sym[k];

			k = src_mid_bits & 0x7FF;
			n = lut->bits2len[k];
			src_mid_bits >>= n;
			src_mid_bitpos -= n;
			dst[5] = lut->bits2sym[k];
			dst += 6;
		}
		dst_end += 5;

		src -= src_bitpos >> 3;
		src_bitpos &= 7;

		src_end += 4 + (src_end_bitpos >> 3);
		src_end_bitpos &= 7;

		src_mid -= src_mid_bitpos >> 3;
		src_mid_bitpos &= 7;
	}
	for (;;)
	{
		if (dst >= dst_end)
			break;

		if (src_mid - src <= 1)
		{
			if (src_mid - src == 1)
				src_bits |= *src << src_bitpos;
		}
		else
		{
			src_bits |= *(const uint16_t *)src << src_bitpos;
		}
		k = src_bits & 0x7FF;
		n = lut->bits2len[k];
		src_bitpos -= n;
		src_bits >>= n;
		*dst++ = lut->bits2sym[k];
		src += (7 - src_bitpos) >> 3;
		src_bitpos &= 7;

		if (dst < dst_end)
		{
			if (src_end - src_mid <= 1)
			{
				if (src_end - src_mid == 1)
				{
					src_end_bits |= *src_mid << src_end_bitpos;
					src_mid_bits |= *src_mid << src_mid_bitpos;
				}
			}
			else
			{
				unsigned int v = *(const uint16_t *)(src_end - 2);
				src_end_bits |= (((v >> 8) | (v << 8)) & 0xffff)
								<< src_end_bitpos;
				src_mid_bits |= *(const uint16_t *)src_mid << src_mid_bitpos;
			}
			n = lut->bits2len[src_end_bits & 0x7FF];
			*dst++ = lut->bits2sym[src_end_bits & 0x7FF];
			src_end_bitpos -= n;
			src_end_bits >>= n;
			src_end -= (7 - src_end_bitpos) >> 3;
			src_end_bitpos &= 7;
			if (dst < dst_end)
			{
				n = lut->bits2len[src_mid_bits & 0x7FF];
				*dst++ = lut->bits2sym[src_mid_bits & 0x7FF];
				src_mid_bitpos -= n;
				src_mid_bits >>= n;
				src_mid += (7 - src_mid_bitpos) >> 3;
				src_mid_bitpos &= 7;
			}
		}
		if (src > src_mid || src_mid > src_end)
			return false;
	}
	if (src != hr->src_mid_org || src_end != src_mid)
		return false;
	return true;
}

int Huff_ReadCodeLengthsOld(
	BitReader *bits, uint8_t *syms, uint32_t *code_prefix)
{
	if (BitReader_ReadBitNoRefill(bits))
	{
		int n, sym = 0, codelen, num_symbols = 0;
		int avg_bits_x4 = 32;
		int forced_bits = BitReader_ReadBitsNoRefill(bits, 2);

		uint32_t thres_for_valid_gamma_bits = 1 << (31 - (20u >> forced_bits));
		if (BitReader_ReadBit(bits))
			goto SKIP_INITIAL_ZEROS;
		do
		{
			// Run of zeros
			if (!(bits->bits & 0xff000000))
				return -1;
			sym += BitReader_ReadBitsNoRefill(
					   bits, 2 * (CountLeadingZeros(bits->bits) + 1)) -
				   2 + 1;
			if (sym >= 256)
				break;
		SKIP_INITIAL_ZEROS:
			BitReader_Refill(bits);
			// Read out the gamma value for the # of symbols
			if (!(bits->bits & 0xff000000))
				return -1;
			n = BitReader_ReadBitsNoRefill(
					bits, 2 * (CountLeadingZeros(bits->bits) + 1)) -
				2 + 1;
			// Overflow?
			if (sym + n > 256)
				return -1;
			BitReader_Refill(bits);
			num_symbols += n;
			do
			{
				if (bits->bits < thres_for_valid_gamma_bits)
					return -1; // too big gamma value?

				int lz = CountLeadingZeros(bits->bits);
				int v =
					BitReader_ReadBitsNoRefill(bits, lz + forced_bits + 1) +
					((lz - 1) << forced_bits);
				codelen =
					(-(int)(v & 1) ^ (v >> 1)) + ((avg_bits_x4 + 2) >> 2);
				if (codelen < 1 || codelen > 11)
					return -1;
				avg_bits_x4 = codelen + ((3 * avg_bits_x4 + 2) >> 2);
				BitReader_Refill(bits);
				syms[code_prefix[codelen]++] = (uint8_t)sym++;
			} while (--n);
		} while (sym != 256);
		return (sym == 256) && (num_symbols >= 2) ? num_symbols : -1;
	}
	else
	{
		// Sparse symbol encoding
		int num_symbols = BitReader_ReadBitsNoRefill(bits, 8);
		if (num_symbols == 0)
			return -1;
		if (num_symbols == 1)
		{
			syms[0] = (uint8_t)BitReader_ReadBitsNoRefill(bits, 8);
		}
		else
		{
			int codelen_bits = BitReader_ReadBitsNoRefill(bits, 3);
			if (codelen_bits > 4)
				return -1;
			for (int i = 0; i < num_symbols; i++)
			{
				BitReader_Refill(bits);
				int sym = BitReader_ReadBitsNoRefill(bits, 8);
				int codelen =
					BitReader_ReadBitsNoRefillZero(bits, codelen_bits) + 1;
				if (codelen > 11)
					return -1;
				syms[code_prefix[codelen]++] = (uint8_t)sym;
			}
		}
		return num_symbols;
	}
}

int BitReader_ReadFluff(BitReader *bits, int num_symbols)
{
	unsigned long y;

	if (num_symbols == 256)
		return 0;

	int x = 257 - num_symbols;
	if (x > num_symbols)
		x = num_symbols;

	x *= 2;

	bit_scan_reverse(&y, x - 1);
	y += 1;

	uint32_t v = bits->bits >> (32 - y);
	uint32_t z = (1 << y) - x;

	if ((v >> 1) >= z)
	{
		bits->bits <<= y;
		bits->bitpos += y;
		return v - z;
	}
	else
	{
		bits->bits <<= (y - 1);
		bits->bitpos += (y - 1);
		return (v >> 1);
	}
}

typedef struct
{
	const uint8_t *p, *p_end;
	uint32_t bitpos;
} BitReader2;

static const uint32_t kRiceCodeBits2Value[256] = {
	0x80000000, 0x00000007, 0x10000006, 0x00000006, 0x20000005, 0x00000105,
	0x10000005, 0x00000005, 0x30000004, 0x00000204, 0x10000104, 0x00000104,
	0x20000004, 0x00010004, 0x10000004, 0x00000004, 0x40000003, 0x00000303,
	0x10000203, 0x00000203, 0x20000103, 0x00010103, 0x10000103, 0x00000103,
	0x30000003, 0x00020003, 0x10010003, 0x00010003, 0x20000003, 0x01000003,
	0x10000003, 0x00000003, 0x50000002, 0x00000402, 0x10000302, 0x00000302,
	0x20000202, 0x00010202, 0x10000202, 0x00000202, 0x30000102, 0x00020102,
	0x10010102, 0x00010102, 0x20000102, 0x01000102, 0x10000102, 0x00000102,
	0x40000002, 0x00030002, 0x10020002, 0x00020002, 0x20010002, 0x01010002,
	0x10010002, 0x00010002, 0x30000002, 0x02000002, 0x11000002, 0x01000002,
	0x20000002, 0x00000012, 0x10000002, 0x00000002, 0x60000001, 0x00000501,
	0x10000401, 0x00000401, 0x20000301, 0x00010301, 0x10000301, 0x00000301,
	0x30000201, 0x00020201, 0x10010201, 0x00010201, 0x20000201, 0x01000201,
	0x10000201, 0x00000201, 0x40000101, 0x00030101, 0x10020101, 0x00020101,
	0x20010101, 0x01010101, 0x10010101, 0x00010101, 0x30000101, 0x02000101,
	0x11000101, 0x01000101, 0x20000101, 0x00000111, 0x10000101, 0x00000101,
	0x50000001, 0x00040001, 0x10030001, 0x00030001, 0x20020001, 0x01020001,
	0x10020001, 0x00020001, 0x30010001, 0x02010001, 0x11010001, 0x01010001,
	0x20010001, 0x00010011, 0x10010001, 0x00010001, 0x40000001, 0x03000001,
	0x12000001, 0x02000001, 0x21000001, 0x01000011, 0x11000001, 0x01000001,
	0x30000001, 0x00000021, 0x10000011, 0x00000011, 0x20000001, 0x00001001,
	0x10000001, 0x00000001, 0x70000000, 0x00000600, 0x10000500, 0x00000500,
	0x20000400, 0x00010400, 0x10000400, 0x00000400, 0x30000300, 0x00020300,
	0x10010300, 0x00010300, 0x20000300, 0x01000300, 0x10000300, 0x00000300,
	0x40000200, 0x00030200, 0x10020200, 0x00020200, 0x20010200, 0x01010200,
	0x10010200, 0x00010200, 0x30000200, 0x02000200, 0x11000200, 0x01000200,
	0x20000200, 0x00000210, 0x10000200, 0x00000200, 0x50000100, 0x00040100,
	0x10030100, 0x00030100, 0x20020100, 0x01020100, 0x10020100, 0x00020100,
	0x30010100, 0x02010100, 0x11010100, 0x01010100, 0x20010100, 0x00010110,
	0x10010100, 0x00010100, 0x40000100, 0x03000100, 0x12000100, 0x02000100,
	0x21000100, 0x01000110, 0x11000100, 0x01000100, 0x30000100, 0x00000120,
	0x10000110, 0x00000110, 0x20000100, 0x00001100, 0x10000100, 0x00000100,
	0x60000000, 0x00050000, 0x10040000, 0x00040000, 0x20030000, 0x01030000,
	0x10030000, 0x00030000, 0x30020000, 0x02020000, 0x11020000, 0x01020000,
	0x20020000, 0x00020010, 0x10020000, 0x00020000, 0x40010000, 0x03010000,
	0x12010000, 0x02010000, 0x21010000, 0x01010010, 0x11010000, 0x01010000,
	0x30010000, 0x00010020, 0x10010010, 0x00010010, 0x20010000, 0x00011000,
	0x10010000, 0x00010000, 0x50000000, 0x04000000, 0x13000000, 0x03000000,
	0x22000000, 0x02000010, 0x12000000, 0x02000000, 0x31000000, 0x01000020,
	0x11000010, 0x01000010, 0x21000000, 0x01001000, 0x11000000, 0x01000000,
	0x40000000, 0x00000030, 0x10000020, 0x00000020, 0x20000010, 0x00001010,
	0x10000010, 0x00000010, 0x30000000, 0x00002000, 0x10001000, 0x00001000,
	0x20000000, 0x00100000, 0x10000000, 0x00000000,
};

static const uint8_t kRiceCodeBits2Len[256] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4,
	2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4,
	2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
	4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5,
	3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
	4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
};

bool DecodeGolombRiceLengths(uint8_t *dst, size_t size, BitReader2 *br)
{
	const uint8_t *p = br->p, *p_end = br->p_end;
	uint8_t *dst_end = dst + size;
	if (p >= p_end)
		return false;

	int count = -(int)br->bitpos;
	uint32_t v = *p++ & (255 >> br->bitpos);
	for (;;)
	{
		if (v == 0)
		{
			count += 8;
		}
		else
		{
			uint32_t x = kRiceCodeBits2Value[v];
			*(uint32_t *)&dst[0] = count + (x & 0x0f0f0f0f);
			*(uint32_t *)&dst[4] = (x >> 4) & 0x0f0f0f0f;
			dst += kRiceCodeBits2Len[v];
			if (dst >= dst_end)
				break;
			count = x >> 28;
		}
		if (p >= p_end)
			return false;
		v = *p++;
	}
	// went too far, step back
	if (dst > dst_end)
	{
		int n = (int)(dst - dst_end);
		do
			v &= (v - 1);
		while (--n);
	}
	// step back if uint8_t not finished
	int bitpos = 0;
	if (!(v & 1))
	{
		p--;
		unsigned long q = 9;
		bit_scan_forward(&q, v);
		bitpos = (int)(8 - q);
	}
	br->p = p;
	br->bitpos = bitpos;
	return true;
}

bool DecodeGolombRiceBits(
	uint8_t *dst, unsigned int size, unsigned int bitcount, BitReader2 *br)
{
	if (bitcount == 0)
		return true;
	uint8_t *dst_end = dst + size;
	const uint8_t *p = br->p;
	int bitpos = br->bitpos;

	unsigned int bits_required = bitpos + bitcount * size;
	unsigned int bytes_required = (bits_required + 7) >> 3;
	if (bytes_required > br->p_end - p)
		return false;

	br->p = p + (bits_required >> 3);
	br->bitpos = bits_required & 7;

	// todo. handle r/w outside of range
	uint64_t bak = *(uint64_t *)dst_end;

	if (bitcount < 2)
	{
		assert(bitcount == 1);
		do
		{
			// Read the next uint8_t
			uint64_t bits =
				(uint8_t)(bswap_32(*(const uint32_t *)p) >> (24 - bitpos));
			p += 1;
			// Expand each bit into each uint8_t of the uint64_t.
			bits = (bits | (bits << 28)) & 0xF0000000Full;
			bits = (bits | (bits << 14)) & 0x3000300030003ull;
			bits = (bits | (bits << 7)) & 0x0101010101010101ull;
			*(uint64_t *)dst = *(uint64_t *)dst * 2 + bswap_64(bits);
			dst += 8;
		} while (dst < dst_end);
	}
	else if (bitcount == 2)
	{
		do
		{
			// Read the next 2 bytes
			uint64_t bits =
				(uint16_t)(bswap_32(*(const uint32_t *)p) >> (16 - bitpos));
			p += 2;
			// Expand each bit into each uint8_t of the uint64_t.
			bits = (bits | (bits << 24)) & 0xFF000000FFull;
			bits = (bits | (bits << 12)) & 0xF000F000F000Full;
			bits = (bits | (bits << 6)) & 0x0303030303030303ull;
			*(uint64_t *)dst = *(uint64_t *)dst * 4 + bswap_64(bits);
			dst += 8;
		} while (dst < dst_end);
	}
	else
	{
		assert(bitcount == 3);
		do
		{
			// Read the next 3 bytes
			uint64_t bits =
				(bswap_32(*(const uint32_t *)p) >> (8 - bitpos)) & 0xffffff;
			p += 3;
			// Expand each bit into each uint8_t of the uint64_t.
			bits = (bits | (bits << 20)) & 0xFFF00000FFFull;
			bits = (bits | (bits << 10)) & 0x3F003F003F003Full;
			bits = (bits | (bits << 5)) & 0x0707070707070707ull;
			*(uint64_t *)dst = *(uint64_t *)dst * 8 + bswap_64(bits);
			dst += 8;
		} while (dst < dst_end);
	}
	*(uint64_t *)dst_end = bak;
	return true;
}

int Huff_ConvertToRanges(
	HuffRange *range, int num_symbols, int P, const uint8_t *symlen,
	BitReader *bits)
{
	int num_ranges = P >> 1, v, sym_idx = 0;

	// Start with space?
	if (P & 1)
	{
		BitReader_Refill(bits);
		v = *symlen++;
		if (v >= 8)
			return -1;
		sym_idx = BitReader_ReadBitsNoRefill(bits, v + 1) + (1 << (v + 1)) - 1;
	}
	int syms_used = 0;

	for (int i = 0; i < num_ranges; i++)
	{
		BitReader_Refill(bits);
		v = symlen[0];
		if (v >= 9)
			return -1;
		int num = BitReader_ReadBitsNoRefillZero(bits, v) + (1 << v);
		v = symlen[1];
		if (v >= 8)
			return -1;
		int space =
			BitReader_ReadBitsNoRefill(bits, v + 1) + (1 << (v + 1)) - 1;
		range[i].symbol = (uint16_t)sym_idx;
		range[i].num = (uint16_t)num;
		syms_used += num;
		sym_idx += num + space;
		symlen += 2;
	}

	if (sym_idx >= 256 || syms_used >= num_symbols ||
		sym_idx + num_symbols - syms_used > 256)
		return -1;

	range[num_ranges].symbol = (uint16_t)sym_idx;
	range[num_ranges].num = (uint16_t)(num_symbols - syms_used);

	return num_ranges + 1;
}

int Huff_ReadCodeLengthsNew(
	BitReader *bits, uint8_t *syms, uint32_t *code_prefix)
{
	int forced_bits = BitReader_ReadBitsNoRefill(bits, 2);

	unsigned int num_symbols = BitReader_ReadBitsNoRefill(bits, 8) + 1;

	int fluff = BitReader_ReadFluff(bits, num_symbols);

	uint8_t code_len[512];
	BitReader2 br2;
	br2.bitpos = (bits->bitpos - 24) & 7;
	br2.p_end = bits->p_end;
	br2.p = bits->p - (unsigned)((24 - bits->bitpos + 7) >> 3);

	if (!DecodeGolombRiceLengths(code_len, num_symbols + fluff, &br2))
		return -1;
	memset(code_len + (num_symbols + fluff), 0, 16);
	if (!DecodeGolombRiceBits(code_len, num_symbols, forced_bits, &br2))
		return -1;

	// Reset the bits decoder.
	bits->bitpos = 24;
	bits->p = br2.p;
	bits->bits = 0;
	BitReader_Refill(bits);
	bits->bits <<= br2.bitpos;
	bits->bitpos += br2.bitpos;

	if (1)
	{
		unsigned int running_sum = 0x1e;
		for (unsigned int i = 0; i < num_symbols; i++)
		{
			int v = code_len[i];
			v = -(int)(v & 1) ^ (v >> 1);
			code_len[i] = (uint8_t)(v + (running_sum >> 2) + 1);
			if (code_len[i] < 1 || code_len[i] > 11)
				return -1;
			running_sum += v;
		}
	}

	HuffRange range[128];
	int ranges = Huff_ConvertToRanges(
		range, num_symbols, fluff, &code_len[num_symbols], bits);
	if (ranges <= 0)
		return -1;

	uint8_t *cp = code_len;
	for (int i = 0; i < ranges; i++)
	{
		int sym = range[i].symbol;
		int n = range[i].num;
		do
		{
			syms[code_prefix[*cp++]++] = (uint8_t)sym++;
		} while (--n);
	}

	return num_symbols;
}

typedef struct
{
	// Mapping that maps a bit pattern to a code length.
	uint8_t bits2len[2048 + 16];
	// Mapping that maps a bit pattern to a symbol.
	uint8_t bits2sym[2048 + 16];
} NewHuffLut;

// May overflow 16 bytes past the end
void FillByteOverflow16(uint8_t *dst, uint8_t v, size_t n)
{
	memset(dst, v, n);
}

bool Huff_MakeLut(
	const uint32_t *prefix_org, const uint32_t *prefix_cur,
	NewHuffLut *hufflut, uint8_t *syms)
{
	uint32_t currslot = 0;
	for (uint32_t i = 1; i < 11; i++)
	{
		uint32_t start = prefix_org[i];
		uint32_t count = prefix_cur[i] - start;
		if (count)
		{
			uint32_t stepsize = 1 << (11 - i);
			uint32_t num_to_set = count << (11 - i);
			if (currslot + num_to_set > 2048)
				return false;
			FillByteOverflow16(
				&hufflut->bits2len[currslot], (uint8_t)i, num_to_set);

			uint8_t *p = &hufflut->bits2sym[currslot];
			for (uint32_t j = 0; j != count; j++, p += stepsize)
				FillByteOverflow16(p, syms[start + j], stepsize);
			currslot += num_to_set;
		}
	}
	if (prefix_cur[11] - prefix_org[11] != 0)
	{
		uint32_t num_to_set = prefix_cur[11] - prefix_org[11];
		if (currslot + num_to_set > 2048)
			return false;
		FillByteOverflow16(&hufflut->bits2len[currslot], 11, num_to_set);
		memcpy(
			&hufflut->bits2sym[currslot], &syms[prefix_org[11]], num_to_set);
		currslot += num_to_set;
	}
	return currslot == 2048;
}

int Kraken_DecodeBytes_Type12(
	const uint8_t *src, size_t src_size, uint8_t *output, int output_size,
	int type)
{
	BitReader bits;
	int half_output_size;
	uint32_t split_left, split_mid, split_right;
	const uint8_t *src_mid;
	NewHuffLut huff_lut;
	HuffReader hr;
	HuffRevLut rev_lut;
	const uint8_t *src_end = src + src_size;

	bits.bitpos = 24;
	bits.bits = 0;
	bits.p = src;
	bits.p_end = src_end;
	BitReader_Refill(&bits);

	static const uint32_t code_prefix_org[12] = {
		0x0, 0x0, 0x2, 0x6, 0xE, 0x1E, 0x3E, 0x7E, 0xFE, 0x1FE, 0x2FE, 0x3FE};
	uint32_t code_prefix[12] = {0x0,  0x0,	0x2,  0x6,	 0xE,	0x1E,
								0x3E, 0x7E, 0xFE, 0x1FE, 0x2FE, 0x3FE};
	uint8_t syms[1280];
	int num_syms;
	if (!BitReader_ReadBitNoRefill(&bits))
	{
		num_syms = Huff_ReadCodeLengthsOld(&bits, syms, code_prefix);
	}
	else if (!BitReader_ReadBitNoRefill(&bits))
	{
		num_syms = Huff_ReadCodeLengthsNew(&bits, syms, code_prefix);
	}
	else
	{
		return -1;
	}

	if (num_syms < 1)
		return -1;
	src = bits.p - ((24 - bits.bitpos) / 8);

	if (num_syms == 1)
	{
		memset(output, syms[0], output_size);
		return (int)(src - src_end);
	}

	if (!Huff_MakeLut(code_prefix_org, code_prefix, &huff_lut, syms))
		return -1;

	ReverseBitsArray2048(huff_lut.bits2len, rev_lut.bits2len);
	ReverseBitsArray2048(huff_lut.bits2sym, rev_lut.bits2sym);

	if (type == 1)
	{
		if (src + 3 > src_end)
			return -1;
		split_mid = *(const uint16_t *)src;
		src += 2;
		hr.output = output;
		hr.output_end = output + output_size;
		hr.src = src;
		hr.src_end = src_end;
		hr.src_mid_org = hr.src_mid = src + split_mid;
		hr.src_bitpos = 0;
		hr.src_bits = 0;
		hr.src_mid_bitpos = 0;
		hr.src_mid_bits = 0;
		hr.src_end_bitpos = 0;
		hr.src_end_bits = 0;
		if (!Kraken_DecodeBytesCore(&hr, &rev_lut))
			return -1;
	}
	else
	{
		if (src + 6 > src_end)
			return -1;

		half_output_size = (output_size + 1) >> 1;
		split_mid = *(const uint32_t *)src & 0xFFFFFF;
		src += 3;
		if (split_mid > (src_end - src))
			return -1;
		src_mid = src + split_mid;
		split_left = *(const uint16_t *)src;
		src += 2;
		if (src_mid - src < split_left + 2 || src_end - src_mid < 3)
			return -1;
		split_right = *(const uint16_t *)src_mid;
		if (src_end - (src_mid + 2) < split_right + 2)
			return -1;

		hr.output = output;
		hr.output_end = output + half_output_size;
		hr.src = src;
		hr.src_end = src_mid;
		hr.src_mid_org = hr.src_mid = src + split_left;
		hr.src_bitpos = 0;
		hr.src_bits = 0;
		hr.src_mid_bitpos = 0;
		hr.src_mid_bits = 0;
		hr.src_end_bitpos = 0;
		hr.src_end_bits = 0;
		if (!Kraken_DecodeBytesCore(&hr, &rev_lut))
			return -1;

		hr.output = output + half_output_size;
		hr.output_end = output + output_size;
		hr.src = src_mid + 2;
		hr.src_end = src_end;
		hr.src_mid_org = hr.src_mid = src_mid + 2 + split_right;
		hr.src_bitpos = 0;
		hr.src_bits = 0;
		hr.src_mid_bitpos = 0;
		hr.src_mid_bits = 0;
		hr.src_end_bitpos = 0;
		hr.src_end_bits = 0;
		if (!Kraken_DecodeBytesCore(&hr, &rev_lut))
			return -1;
	}
	return (int)src_size;
}

static uint32_t bitmasks[32] = {
	0x1,		0x3,	   0x7,		  0xf,		 0x1f,		 0x3f,
	0x7f,		0xff,	   0x1ff,	  0x3ff,	 0x7ff,		 0xfff,
	0x1fff,		0x3fff,	   0x7fff,	  0xffff,	 0x1ffff,	 0x3ffff,
	0x7ffff,	0xfffff,   0x1fffff,  0x3fffff,	 0x7fffff,	 0xffffff,
	0x1ffffff,	0x3ffffff, 0x7ffffff, 0xfffffff, 0x1fffffff, 0x3fffffff,
	0x7fffffff, 0xffffffff};

int Kraken_DecodeMultiArray(
	const uint8_t *src, const uint8_t *src_end, uint8_t *dst, uint8_t *dst_end,
	uint8_t **array_data, int *array_lens, int array_count,
	int *total_size_out, bool force_memmove, uint8_t *scratch,
	uint8_t *scratch_end)
{
	const uint8_t *src_org = src;

	if (src_end - src < 4)
		return -1;

	int decoded_size;
	int num_arrays_in_file = *src++;
	if (!(num_arrays_in_file & 0x80))
		return -1;
	num_arrays_in_file &= 0x3f;

	if (dst == scratch)
	{
		// todo: ensure scratch space first?
		scratch += (scratch_end - scratch - 0xc000) >> 1;
		dst_end = scratch;
	}

	int total_size = 0;

	if (num_arrays_in_file == 0)
	{
		for (int i = 0; i < array_count; i++)
		{
			uint8_t *chunk_dst = dst;
			int dec = Kraken_DecodeBytes(
				&chunk_dst, src, src_end, &decoded_size, dst_end - dst,
				force_memmove, scratch, scratch_end);
			if (dec < 0)
				return -1;
			dst += decoded_size;
			array_lens[i] = decoded_size;
			array_data[i] = chunk_dst;
			src += dec;
			total_size += decoded_size;
		}
		*total_size_out = total_size;
		return (int)(src - src_org); // not supported yet
	}

	uint8_t *entropy_array_data[32];
	uint32_t entropy_array_size[32];

	// First loop just decodes everything to scratch
	uint8_t *scratch_cur = scratch;

	for (int i = 0; i < num_arrays_in_file; i++)
	{
		uint8_t *chunk_dst = scratch_cur;
		int dec = Kraken_DecodeBytes(
			&chunk_dst, src, src_end, &decoded_size, scratch_end - scratch_cur,
			force_memmove, scratch_cur, scratch_end);
		if (dec < 0)
			return -1;
		entropy_array_data[i] = chunk_dst;
		entropy_array_size[i] = decoded_size;
		scratch_cur += decoded_size;
		total_size += decoded_size;
		src += dec;
	}
	*total_size_out = total_size;

	if (src_end - src < 3)
		return -1;

	int Q = *(const uint16_t *)src;
	src += 2;

	int out_size;
	if (Kraken_GetBlockSize(src, src_end, &out_size, total_size) < 0)
		return -1;
	int num_indexes = out_size;

	int num_lens = num_indexes - array_count;
	if (num_lens < 1)
		return -1;

	if (scratch_end - scratch_cur < num_indexes)
		return -1;
	uint8_t *interval_lenlog2 = scratch_cur;
	scratch_cur += num_indexes;

	if (scratch_end - scratch_cur < num_indexes)
		return -1;
	uint8_t *interval_indexes = scratch_cur;
	scratch_cur += num_indexes;

	if (Q & 0x8000)
	{
		int size_out;
		int n = Kraken_DecodeBytes(
			&interval_indexes, src, src_end, &size_out, num_indexes, false,
			scratch_cur, scratch_end);
		if (n < 0 || size_out != num_indexes)
			return -1;
		src += n;

		for (int i = 0; i < num_indexes; i++)
		{
			int t = interval_indexes[i];
			interval_lenlog2[i] = (uint8_t)(t >> 4);
			interval_indexes[i] = t & 0xF;
		}

		num_lens = num_indexes;
	}
	else
	{
		int lenlog2_chunksize = num_indexes - array_count;

		int size_out;
		int n = Kraken_DecodeBytes(
			&interval_indexes, src, src_end, &size_out, num_indexes, false,
			scratch_cur, scratch_end);
		if (n < 0 || size_out != num_indexes)
			return -1;
		src += n;

		n = Kraken_DecodeBytes(
			&interval_lenlog2, src, src_end, &size_out, lenlog2_chunksize,
			false, scratch_cur, scratch_end);
		if (n < 0 || size_out != lenlog2_chunksize)
			return -1;
		src += n;

		for (int i = 0; i < lenlog2_chunksize; i++)
			if (interval_lenlog2[i] > 16)
				return -1;
	}

	if (scratch_end - scratch_cur < 4)
		return -1;

	scratch_cur = ALIGN_POINTER(scratch_cur, 4);
	if (scratch_end - scratch_cur < num_lens * 4)
		return -1;
	uint32_t *decoded_intervals = (uint32_t *)scratch_cur;

	int varbits_complen = Q & 0x3FFF;
	if (src_end - src < varbits_complen)
		return -1;

	const uint8_t *f = src;
	uint32_t bits_f = 0;
	int bitpos_f = 24;

	const uint8_t *src_end_actual = src + varbits_complen;

	const uint8_t *b = src_end_actual;
	uint32_t bits_b = 0;
	int bitpos_b = 24;

	int i;
	for (i = 0; i + 2 <= num_lens; i += 2)
	{
		bits_f |= bswap_32(*(const uint32_t *)f) >> (24 - bitpos_f);
		f += (bitpos_f + 7) >> 3;

		bits_b |= ((const uint32_t *)b)[-1] >> (24 - bitpos_b);
		b -= (bitpos_b + 7) >> 3;

		int numbits_f = interval_lenlog2[i + 0];
		int numbits_b = interval_lenlog2[i + 1];

		bits_f = rotl(bits_f | 1, numbits_f);
		bitpos_f += numbits_f - 8 * ((bitpos_f + 7) >> 3);

		bits_b = rotl(bits_b | 1, numbits_b);
		bitpos_b += numbits_b - 8 * ((bitpos_b + 7) >> 3);

		int value_f = bits_f & bitmasks[numbits_f];
		bits_f &= ~bitmasks[numbits_f];

		int value_b = bits_b & bitmasks[numbits_b];
		bits_b &= ~bitmasks[numbits_b];

		decoded_intervals[i + 0] = value_f;
		decoded_intervals[i + 1] = value_b;
	}

	// read final one since above loop reads 2
	if (i < num_lens)
	{
		bits_f |= bswap_32(*(const uint32_t *)f) >> (24 - bitpos_f);
		int numbits_f = interval_lenlog2[i];
		bits_f = rotl(bits_f | 1, numbits_f);
		int value_f = bits_f & bitmasks[numbits_f];
		decoded_intervals[i + 0] = value_f;
	}

	if (interval_indexes[num_indexes - 1])
		return -1;

	int indi = 0, leni = 0, source;
	int increment_leni = (Q & 0x8000) != 0;

	for (int arri = 0; arri < array_count; arri++)
	{
		array_data[arri] = dst;
		if (indi >= num_indexes)
			return -1;

		while ((source = interval_indexes[indi++]) != 0)
		{
			if (source > num_arrays_in_file)
				return -1;
			if (leni >= num_lens)
				return -1;
			int cur_len = decoded_intervals[leni++];
			int bytes_left = entropy_array_size[source - 1];
			if (cur_len > bytes_left || cur_len > dst_end - dst)
				return -1;
			uint8_t *blksrc = entropy_array_data[source - 1];
			entropy_array_size[source - 1] -= cur_len;
			entropy_array_data[source - 1] += cur_len;
			uint8_t *dstx = dst;
			dst += cur_len;
			memcpy(dstx, blksrc, cur_len);
		}
		leni += increment_leni;
		array_lens[arri] = (int)(dst - array_data[arri]);
	}

	if (indi != num_indexes || leni != num_lens)
		return -1;

	for (i = 0; i < num_arrays_in_file; i++)
	{
		if (entropy_array_size[i])
			return -1;
	}
	return (int)(src_end_actual - src_org);
}

int Krak_DecodeRecursive(
	const uint8_t *src, size_t src_size, uint8_t *output, int output_size,
	uint8_t *scratch, uint8_t *scratch_end)
{
	const uint8_t *src_org = src;
	uint8_t *output_end = output + output_size;
	const uint8_t *src_end = src + src_size;

	if (src_size < 6)
		return -1;

	int n = src[0] & 0x7f;
	if (n < 2)
		return -1;

	if (!(src[0] & 0x80))
	{
		src++;
		do
		{
			int decoded_size;
			int dec = Kraken_DecodeBytes(
				&output, src, src_end, &decoded_size, output_end - output,
				true, scratch, scratch_end);
			if (dec < 0)
				return -1;
			output += decoded_size;
			src += dec;
		} while (--n);
		if (output != output_end)
			return -1;
		return (int)(src - src_org);
	}
	else
	{
		uint8_t *array_data;
		int array_len, decoded_size;
		int dec = Kraken_DecodeMultiArray(
			src, src_end, output, output_end, &array_data, &array_len, 1,
			&decoded_size, true, scratch, scratch_end);
		if (dec < 0)
			return -1;
		output += decoded_size;
		if (output != output_end)
			return -1;
		return dec;
	}
}

int Krak_DecodeRLE(
	const uint8_t *src, size_t src_size, uint8_t *dst, int dst_size,
	uint8_t *scratch, uint8_t *scratch_end)
{
	if (src_size <= 1)
	{
		if (src_size != 1)
			return -1;
		memset(dst, src[0], dst_size);
		return 1;
	}
	uint8_t *dst_end = dst + dst_size;
	const uint8_t *cmd_ptr = src + 1, *cmd_ptr_end = src + src_size;
	// Unpack the first X bytes of the command buffer?
	if (src[0])
	{
		uint8_t *dst_ptr = scratch;
		int dec_size;
		int n = Kraken_DecodeBytes(
			&dst_ptr, src, src + src_size, &dec_size, scratch_end - scratch,
			true, scratch, scratch_end);
		if (n <= 0)
			return -1;
		int cmd_len = (int)(src_size - n + dec_size);
		if (cmd_len > scratch_end - scratch)
			return -1;
		memcpy(dst_ptr + dec_size, src + n, src_size - n);
		cmd_ptr = dst_ptr;
		cmd_ptr_end = &dst_ptr[cmd_len];
	}

	int rle_byte = 0;

	while (cmd_ptr < cmd_ptr_end)
	{
		uint32_t cmd = cmd_ptr_end[-1];
		if (cmd - 1 >= 0x2f)
		{
			cmd_ptr_end--;
			uint32_t bytes_to_copy = (-1 - cmd) & 0xF;
			uint32_t bytes_to_rle = cmd >> 4;
			if (dst_end - dst < bytes_to_copy + bytes_to_rle ||
				cmd_ptr_end - cmd_ptr < bytes_to_copy)
				return -1;
			memcpy(dst, cmd_ptr, bytes_to_copy);
			cmd_ptr += bytes_to_copy;
			dst += bytes_to_copy;
			memset(dst, rle_byte, bytes_to_rle);
			dst += bytes_to_rle;
		}
		else if (cmd >= 0x10)
		{
			uint32_t data = *(const uint16_t *)(cmd_ptr_end - 2) - 4096;
			cmd_ptr_end -= 2;
			uint32_t bytes_to_copy = data & 0x3F;
			uint32_t bytes_to_rle = data >> 6;
			if (dst_end - dst < bytes_to_copy + bytes_to_rle ||
				cmd_ptr_end - cmd_ptr < bytes_to_copy)
				return -1;
			memcpy(dst, cmd_ptr, bytes_to_copy);
			cmd_ptr += bytes_to_copy;
			dst += bytes_to_copy;
			memset(dst, rle_byte, bytes_to_rle);
			dst += bytes_to_rle;
		}
		else if (cmd == 1)
		{
			rle_byte = *cmd_ptr++;
			cmd_ptr_end--;
		}
		else if (cmd >= 9)
		{
			uint32_t bytes_to_rle =
				(*(const uint16_t *)(cmd_ptr_end - 2) - 0x8ff) * 128;
			cmd_ptr_end -= 2;
			if (dst_end - dst < bytes_to_rle)
				return -1;
			memset(dst, rle_byte, bytes_to_rle);
			dst += bytes_to_rle;
		}
		else
		{
			uint32_t bytes_to_copy =
				(*(const uint16_t *)(cmd_ptr_end - 2) - 511) * 64;
			cmd_ptr_end -= 2;
			if (cmd_ptr_end - cmd_ptr < bytes_to_copy ||
				dst_end - dst < bytes_to_copy)
				return -1;
			memcpy(dst, cmd_ptr, bytes_to_copy);
			dst += bytes_to_copy;
			cmd_ptr += bytes_to_copy;
		}
	}
	if (cmd_ptr_end != cmd_ptr)
		return -1;

	if (dst != dst_end)
		return -1;

	return (int)src_size;
}

int Kraken_GetBlockSize(
	const uint8_t *src, const uint8_t *src_end, int *dest_size,
	int dest_capacity)
{
	const uint8_t *src_org = src;
	int src_size, dst_size;

	if (src_end - src < 2)
		return -1; // too few bytes

	int chunk_type = (src[0] >> 4) & 0x7;
	if (chunk_type == 0)
	{
		if (src[0] >= 0x80)
		{
			// In this mode, memcopy stores the length in the bottom 12 bits.
			src_size = ((src[0] << 8) | src[1]) & 0xFFF;
			src += 2;
		}
		else
		{
			if (src_end - src < 3)
				return -1; // too few bytes
			src_size = ((src[0] << 16) | (src[1] << 8) | src[2]);
			if (src_size & ~0x3ffff)
				return -1; // reserved bits must not be set
			src += 3;
		}
		if (src_size > dest_capacity || src_end - src < src_size)
			return -1;
		*dest_size = src_size;
		return (int)(src + src_size - src_org);
	}

	if (chunk_type >= 6)
		return -1;

	// In all the other modes, the initial bytes encode
	// the src_size and the dst_size
	if (src[0] >= 0x80)
	{
		if (src_end - src < 3)
			return -1; // too few bytes

		// short mode, 10 bit sizes
		uint32_t bits = ((src[0] << 16) | (src[1] << 8) | src[2]);
		src_size = bits & 0x3ff;
		dst_size = src_size + ((bits >> 10) & 0x3ff) + 1;
		src += 3;
	}
	else
	{
		// long mode, 18 bit sizes
		if (src_end - src < 5)
			return -1; // too few bytes
		uint32_t bits =
			((src[1] << 24) | (src[2] << 16) | (src[3] << 8) | src[4]);
		src_size = bits & 0x3ffff;
		dst_size = (((bits >> 18) | (src[0] << 14)) & 0x3FFFF) + 1;
		if (src_size >= dst_size)
			return -1;
		src += 5;
	}
	if (src_end - src < src_size || dst_size > dest_capacity)
		return -1;
	*dest_size = dst_size;
	return src_size;
}

int Kraken_DecodeBytes(
	uint8_t **output, const uint8_t *src, const uint8_t *src_end,
	int *decoded_size, size_t output_size, bool force_memmove,
	uint8_t *scratch, uint8_t *scratch_end)
{
	const uint8_t *src_org = src;
	size_t src_size, dst_size;

	if (src_end - src < 2)
		return -1; // too few bytes

	int chunk_type = (src[0] >> 4) & 0x7;
	if (chunk_type == 0)
	{
		if (src[0] >= 0x80)
		{
			// In this mode, memcopy stores the length in the bottom 12 bits.
			src_size = ((src[0] << 8) | src[1]) & 0xFFF;
			src += 2;
		}
		else
		{
			if (src_end - src < 3)
				return -1; // too few bytes
			src_size = ((src[0] << 16) | (src[1] << 8) | src[2]);
			if (src_size & ~0x3ffff)
				return -1; // reserved bits must not be set
			src += 3;
		}
		if (src_size > output_size || (size_t)(src_end - src) < src_size)
			return -1;
		*decoded_size = (int)src_size;
		if (force_memmove)
			memmove(*output, src, src_size);
		else
			*output = src;
		return (int)(src + src_size - src_org);
	}

	// In all the other modes, the initial bytes encode
	// the src_size and the dst_size
	if (src[0] >= 0x80)
	{
		if (src_end - src < 3)
			return -1; // too few bytes

		// short mode, 10 bit sizes
		uint32_t bits = ((src[0] << 16) | (src[1] << 8) | src[2]);
		src_size = bits & 0x3ff;
		dst_size = src_size + ((bits >> 10) & 0x3ff) + 1;
		src += 3;
	}
	else
	{
		// long mode, 18 bit sizes
		if (src_end - src < 5)
			return -1; // too few bytes
		uint32_t bits =
			((src[1] << 24) | (src[2] << 16) | (src[3] << 8) | src[4]);
		src_size = bits & 0x3ffff;
		dst_size = (((bits >> 18) | (src[0] << 14)) & 0x3FFFF) + 1;
		if (src_size >= dst_size)
			return -1;
		src += 5;
	}
	if ((size_t)(src_end - src) < src_size || dst_size > output_size)
		return -1;

	uint8_t *dst = *output;
	if (dst == scratch)
	{
		if ((size_t)(scratch_end - scratch) < dst_size)
			return -1;
		scratch += dst_size;
	}

	//  printf("%d -> %d (%d)\n", src_size, dst_size, chunk_type);

	size_t src_used = (size_t)-1;
	switch (chunk_type)
	{
	case 2:
	case 4:
		src_used = Kraken_DecodeBytes_Type12(
			src, src_size, dst, (int)dst_size, chunk_type >> 1);
		break;
	case 5:
		src_used = Krak_DecodeRecursive(
			src, src_size, dst, (int)dst_size, scratch, scratch_end);
		break;
	case 3:
		src_used = Krak_DecodeRLE(
			src, src_size, dst, (int)dst_size, scratch, scratch_end);
		break;
	}
	if (src_used != src_size)
		return -1;
	*decoded_size = (int)dst_size;
	return (int)(src + src_size - src_org);
}

void CombineScaledOffsetArrays(
	int *offs_stream, size_t offs_stream_size, int scale,
	const uint8_t *low_bits)
{
	for (size_t i = 0; i != offs_stream_size; i++)
		offs_stream[i] = scale * offs_stream[i] - low_bits[i];
}

// Unpacks the packed 8 bit offset and lengths into 32 bit.
bool Kraken_UnpackOffsets(
	const uint8_t *src, const uint8_t *src_end,
	const uint8_t *packed_offs_stream, const uint8_t *packed_offs_stream_extra,
	int packed_offs_stream_size, int multi_dist_scale,
	const uint8_t *packed_litlen_stream, int packed_litlen_stream_size,
	int *offs_stream, int *len_stream, bool excess_flag, int excess_bytes)
{
	(void)excess_bytes;
	BitReader bits_a, bits_b;
	int n, i;
	int u32_len_stream_size = 0;

	bits_a.bitpos = 24;
	bits_a.bits = 0;
	bits_a.p = src;
	bits_a.p_end = src_end;
	BitReader_Refill(&bits_a);

	bits_b.bitpos = 24;
	bits_b.bits = 0;
	bits_b.p = src_end;
	bits_b.p_end = src;
	BitReader_RefillBackwards(&bits_b);

	if (!excess_flag)
	{
		if (bits_b.bits < 0x2000)
			return false;
		n = 31 - BSR(bits_b.bits);
		bits_b.bitpos += n;
		bits_b.bits <<= n;
		BitReader_RefillBackwards(&bits_b);
		n++;
		u32_len_stream_size = (bits_b.bits >> (32 - n)) - 1;
		bits_b.bitpos += n;
		bits_b.bits <<= n;
		BitReader_RefillBackwards(&bits_b);
	}

	if (multi_dist_scale == 0)
	{
		// Traditional way of coding offsets
		const uint8_t *packed_offs_stream_end =
			packed_offs_stream + packed_offs_stream_size;
		while (packed_offs_stream != packed_offs_stream_end)
		{
			*offs_stream++ = -(int32_t)BitReader_ReadDistance(
				&bits_a, *packed_offs_stream++);
			if (packed_offs_stream == packed_offs_stream_end)
				break;
			*offs_stream++ = -(int32_t)BitReader_ReadDistanceB(
				&bits_b, *packed_offs_stream++);
		}
	}
	else
	{
		// New way of coding offsets
		int *offs_stream_org = offs_stream;
		const uint8_t *packed_offs_stream_end =
			packed_offs_stream + packed_offs_stream_size;
		uint32_t cmd, offs;
		while (packed_offs_stream != packed_offs_stream_end)
		{
			cmd = *packed_offs_stream++;
			if ((cmd >> 3) > 26)
				return 0;
			offs = ((8 + (cmd & 7)) << (cmd >> 3)) |
				   BitReader_ReadMoreThan24Bits(&bits_a, (cmd >> 3));
			*offs_stream++ = 8 - (int32_t)offs;
			if (packed_offs_stream == packed_offs_stream_end)
				break;
			cmd = *packed_offs_stream++;
			if ((cmd >> 3) > 26)
				return 0;
			offs = ((8 + (cmd & 7)) << (cmd >> 3)) |
				   BitReader_ReadMoreThan24BitsB(&bits_b, (cmd >> 3));
			*offs_stream++ = 8 - (int32_t)offs;
		}
		if (multi_dist_scale != 1)
		{
			CombineScaledOffsetArrays(
				offs_stream_org, offs_stream - offs_stream_org,
				multi_dist_scale, packed_offs_stream_extra);
		}
	}
	uint32_t u32_len_stream_buf[512]; // max count is 128kb / 256 = 512
	if (u32_len_stream_size > 512)
		return false;

	uint32_t *u32_len_stream = u32_len_stream_buf,
			 *u32_len_stream_end = u32_len_stream_buf + u32_len_stream_size;
	for (i = 0; i + 1 < u32_len_stream_size; i += 2)
	{
		if (!BitReader_ReadLength(&bits_a, &u32_len_stream[i + 0]))
			return false;
		if (!BitReader_ReadLengthB(&bits_b, &u32_len_stream[i + 1]))
			return false;
	}
	if (i < u32_len_stream_size)
	{
		if (!BitReader_ReadLength(&bits_a, &u32_len_stream[i + 0]))
			return false;
	}

	bits_a.p -= (24 - bits_a.bitpos) >> 3;
	bits_b.p += (24 - bits_b.bitpos) >> 3;

	if (bits_a.p != bits_b.p)
		return false;

	for (i = 0; i < packed_litlen_stream_size; i++)
	{
		uint32_t v = packed_litlen_stream[i];
		if (v == 255)
			v = *u32_len_stream++ + 255;
		len_stream[i] = v + 3;
	}
	if (u32_len_stream != u32_len_stream_end)
		return false;

	return true;
}
bool Kraken_ReadLzTable(
	int mode, const uint8_t *src, const uint8_t *src_end, uint8_t *dst,
	int dst_size, int offset, uint8_t *scratch, uint8_t *scratch_end,
	KrakenLzTable *lztable)
{
	uint8_t *out;
	int decode_count, n;
	uint8_t *packed_offs_stream, *packed_len_stream;

	if (mode > 1)
		return false;

	if (src_end - src < 13)
		return false;

	if (offset == 0)
	{
		COPY_64(dst, src);
		dst += 8;
		src += 8;
	}

	if (*src & 0x80)
	{
		uint8_t flag = *src++;
		if ((flag & 0xc0) != 0x80)
			return false; // reserved flag set

		return false; // excess bytes not supported
	}

	// Disable no copy optimization if source and dest overlap
	bool force_copy = dst <= src_end && src <= dst + dst_size;

	// Decode lit stream, bounded by dst_size
	out = scratch;
	n = Kraken_DecodeBytes(
		&out, src, src_end, &decode_count,
		Min(scratch_end - scratch, dst_size), force_copy, scratch,
		scratch_end);
	if (n < 0)
		return false;
	src += n;
	lztable->lit_stream = out;
	lztable->lit_stream_size = decode_count;
	scratch += decode_count;

	// Decode command stream, bounded by dst_size
	out = scratch;
	n = Kraken_DecodeBytes(
		&out, src, src_end, &decode_count,
		Min(scratch_end - scratch, dst_size), force_copy, scratch,
		scratch_end);
	if (n < 0)
		return false;
	src += n;
	lztable->cmd_stream = out;
	lztable->cmd_stream_size = decode_count;
	scratch += decode_count;

	// Check if to decode the multistuff crap
	if (src_end - src < 3)
		return false;

	int offs_scaling = 0;
	uint8_t *packed_offs_stream_extra = NULL;

	if (src[0] & 0x80)
	{
		// uses the mode where distances are coded with 2 tables
		offs_scaling = src[0] - 127;
		src++;

		packed_offs_stream = scratch;
		n = Kraken_DecodeBytes(
			&packed_offs_stream, src, src_end, &lztable->offs_stream_size,
			Min(scratch_end - scratch, lztable->cmd_stream_size), false,
			scratch, scratch_end);
		if (n < 0)
			return false;
		src += n;
		scratch += lztable->offs_stream_size;

		if (offs_scaling != 1)
		{
			packed_offs_stream_extra = scratch;
			n = Kraken_DecodeBytes(
				&packed_offs_stream_extra, src, src_end, &decode_count,
				Min(scratch_end - scratch, lztable->offs_stream_size), false,
				scratch, scratch_end);
			if (n < 0 || decode_count != lztable->offs_stream_size)
				return false;
			src += n;
			scratch += decode_count;
		}
	}
	else
	{
		// Decode packed offset stream, it's bounded by the command length.
		packed_offs_stream = scratch;
		n = Kraken_DecodeBytes(
			&packed_offs_stream, src, src_end, &lztable->offs_stream_size,
			Min(scratch_end - scratch, lztable->cmd_stream_size), false,
			scratch, scratch_end);
		if (n < 0)
			return false;
		src += n;
		scratch += lztable->offs_stream_size;
	}

	// Decode packed litlen stream. It's bounded by 1/4 of dst_size.
	packed_len_stream = scratch;
	n = Kraken_DecodeBytes(
		&packed_len_stream, src, src_end, &lztable->len_stream_size,
		Min(scratch_end - scratch, dst_size >> 2), false, scratch,
		scratch_end);
	if (n < 0)
		return false;
	src += n;
	scratch += lztable->len_stream_size;

	// Reserve memory for final dist stream
	scratch = ALIGN_POINTER(scratch, 16);
	lztable->offs_stream = (int *)scratch;
	scratch += lztable->offs_stream_size * 4;

	// Reserve memory for final len stream
	scratch = ALIGN_POINTER(scratch, 16);
	lztable->len_stream = (int *)scratch;
	scratch += lztable->len_stream_size * 4;

	if (scratch + 64 > scratch_end)
		return false;

	return Kraken_UnpackOffsets(
		src, src_end, packed_offs_stream, packed_offs_stream_extra,
		lztable->offs_stream_size, offs_scaling, packed_len_stream,
		lztable->len_stream_size, lztable->offs_stream, lztable->len_stream, 0,
		0);
}

// Note: may access memory out of bounds on invalid input.
bool Kraken_ProcessLzRuns_Type0(
	KrakenLzTable *lzt, uint8_t *dst, uint8_t *dst_end, uint8_t *dst_start)
{
	const uint8_t *cmd_stream = lzt->cmd_stream,
				  *cmd_stream_end = cmd_stream + lzt->cmd_stream_size;
	const int *len_stream = lzt->len_stream;
	const int *len_stream_end = lzt->len_stream + lzt->len_stream_size;
	const uint8_t *lit_stream = lzt->lit_stream;
	const uint8_t *lit_stream_end = lzt->lit_stream + lzt->lit_stream_size;
	const int *offs_stream = lzt->offs_stream;
	const int *offs_stream_end = lzt->offs_stream + lzt->offs_stream_size;
	const uint8_t *copyfrom;
	uint32_t final_len;
	int32_t offset;
	int32_t recent_offs[7];
	int32_t last_offset;

	recent_offs[3] = -8;
	recent_offs[4] = -8;
	recent_offs[5] = -8;
	last_offset = -8;

	while (cmd_stream < cmd_stream_end)
	{
		uint32_t f = *cmd_stream++;
		uint32_t litlen = f & 3;
		uint32_t offs_index = f >> 6;
		uint32_t matchlen = (f >> 2) & 0xF;

		// use cmov
		uint32_t next_long_length = *len_stream;
		const int *next_len_stream = len_stream + 1;

		len_stream = (litlen == 3) ? next_len_stream : len_stream;
		litlen = (litlen == 3) ? next_long_length : litlen;
		recent_offs[6] = *offs_stream;

		COPY_64_ADD(dst, lit_stream, &dst[last_offset]);
		if (litlen > 8)
		{
			COPY_64_ADD(dst + 8, lit_stream + 8, &dst[last_offset + 8]);
			if (litlen > 16)
			{
				COPY_64_ADD(dst + 16, lit_stream + 16, &dst[last_offset + 16]);
				if (litlen > 24)
				{
					do
					{
						COPY_64_ADD(
							dst + 24, lit_stream + 24, &dst[last_offset + 24]);
						litlen -= 8;
						dst += 8;
						lit_stream += 8;
					} while (litlen > 24);
				}
			}
		}
		dst += litlen;
		lit_stream += litlen;

		offset = recent_offs[offs_index + 3];
		recent_offs[offs_index + 3] = recent_offs[offs_index + 2];
		recent_offs[offs_index + 2] = recent_offs[offs_index + 1];
		recent_offs[offs_index + 1] = recent_offs[offs_index + 0];
		recent_offs[3] = offset;
		last_offset = offset;

		offs_stream = (int *)((intptr_t)offs_stream + ((offs_index + 1) & 4));

		if ((uintptr_t)offset < (uintptr_t)(dst_start - dst))
			return false; // offset out of bounds

		copyfrom = dst + offset;
		if (matchlen != 15)
		{
			COPY_64(dst, copyfrom);
			COPY_64(dst + 8, copyfrom + 8);
			dst += matchlen + 2;
		}
		else
		{
			matchlen = 14 + *len_stream++; // why is the value not 16 here, the
										   // above case copies up to 16 bytes.
			if ((uintptr_t)matchlen > (uintptr_t)(dst_end - dst))
				return false; // copy length out of bounds
			COPY_64(dst, copyfrom);
			COPY_64(dst + 8, copyfrom + 8);
			COPY_64(dst + 16, copyfrom + 16);
			do
			{
				COPY_64(dst + 24, copyfrom + 24);
				matchlen -= 8;
				dst += 8;
				copyfrom += 8;
			} while (matchlen > 24);
			dst += matchlen;
		}
	}

	// check for incorrect input
	if (offs_stream != offs_stream_end || len_stream != len_stream_end)
		return false;

	final_len = (uint32_t)(dst_end - dst);
	if (final_len != lit_stream_end - lit_stream)
		return false;

	if (final_len >= 8)
	{
		do
		{
			COPY_64_ADD(dst, lit_stream, &dst[last_offset]);
			dst += 8, lit_stream += 8, final_len -= 8;
		} while (final_len >= 8);
	}
	if (final_len > 0)
	{
		do
		{
			*dst = *lit_stream++ + dst[last_offset];
		} while (dst++, --final_len);
	}
	return true;
}

// Note: may access memory out of bounds on invalid input.
bool Kraken_ProcessLzRuns_Type1(
	KrakenLzTable *lzt, uint8_t *dst, uint8_t *dst_end, uint8_t *dst_start)
{
	const uint8_t *cmd_stream = lzt->cmd_stream,
				  *cmd_stream_end = cmd_stream + lzt->cmd_stream_size;
	const int *len_stream = lzt->len_stream;
	const int *len_stream_end = lzt->len_stream + lzt->len_stream_size;
	const uint8_t *lit_stream = lzt->lit_stream;
	const uint8_t *lit_stream_end = lzt->lit_stream + lzt->lit_stream_size;
	const int *offs_stream = lzt->offs_stream;
	const int *offs_stream_end = lzt->offs_stream + lzt->offs_stream_size;
	const uint8_t *copyfrom;
	uint32_t final_len;
	int32_t offset;
	int32_t recent_offs[7];

	recent_offs[3] = -8;
	recent_offs[4] = -8;
	recent_offs[5] = -8;

	while (cmd_stream < cmd_stream_end)
	{
		uint32_t f = *cmd_stream++;
		uint32_t litlen = f & 3;
		uint32_t offs_index = f >> 6;
		uint32_t matchlen = (f >> 2) & 0xF;

		// use cmov
		uint32_t next_long_length = *len_stream;
		const int *next_len_stream = len_stream + 1;

		len_stream = (litlen == 3) ? next_len_stream : len_stream;
		litlen = (litlen == 3) ? next_long_length : litlen;
		recent_offs[6] = *offs_stream;

		COPY_64(dst, lit_stream);
		if (litlen > 8)
		{
			COPY_64(dst + 8, lit_stream + 8);
			if (litlen > 16)
			{
				COPY_64(dst + 16, lit_stream + 16);
				if (litlen > 24)
				{
					do
					{
						COPY_64(dst + 24, lit_stream + 24);
						litlen -= 8;
						dst += 8;
						lit_stream += 8;
					} while (litlen > 24);
				}
			}
		}
		dst += litlen;
		lit_stream += litlen;

		offset = recent_offs[offs_index + 3];
		recent_offs[offs_index + 3] = recent_offs[offs_index + 2];
		recent_offs[offs_index + 2] = recent_offs[offs_index + 1];
		recent_offs[offs_index + 1] = recent_offs[offs_index + 0];
		recent_offs[3] = offset;

		offs_stream = (int *)((intptr_t)offs_stream + ((offs_index + 1) & 4));

		if ((uintptr_t)offset < (uintptr_t)(dst_start - dst))
			return false; // offset out of bounds

		copyfrom = dst + offset;
		if (matchlen != 15)
		{
			COPY_64(dst, copyfrom);
			COPY_64(dst + 8, copyfrom + 8);
			dst += matchlen + 2;
		}
		else
		{
			matchlen = 14 + *len_stream++; // why is the value not 16 here, the
										   // above case copies up to 16 bytes.
			if ((uintptr_t)matchlen > (uintptr_t)(dst_end - dst))
				return false; // copy length out of bounds
			COPY_64(dst, copyfrom);
			COPY_64(dst + 8, copyfrom + 8);
			COPY_64(dst + 16, copyfrom + 16);
			do
			{
				COPY_64(dst + 24, copyfrom + 24);
				matchlen -= 8;
				dst += 8;
				copyfrom += 8;
			} while (matchlen > 24);
			dst += matchlen;
		}
	}

	// check for incorrect input
	if (offs_stream != offs_stream_end || len_stream != len_stream_end)
		return false;

	final_len = (uint32_t)(dst_end - dst);
	if (final_len != lit_stream_end - lit_stream)
		return false;

	if (final_len >= 64)
	{
		do
		{
			COPY_64_BYTES(dst, lit_stream);
			dst += 64, lit_stream += 64, final_len -= 64;
		} while (final_len >= 64);
	}
	if (final_len >= 8)
	{
		do
		{
			COPY_64(dst, lit_stream);
			dst += 8, lit_stream += 8, final_len -= 8;
		} while (final_len >= 8);
	}
	if (final_len > 0)
	{
		do
		{
			*dst++ = *lit_stream++;
		} while (--final_len);
	}
	return true;
}

bool Kraken_ProcessLzRuns(
	int mode, uint8_t *dst, int dst_size, int offset, KrakenLzTable *lztable)
{
	uint8_t *dst_end = dst + dst_size;

	if (mode == 1)
		return Kraken_ProcessLzRuns_Type1(
			lztable, dst + (offset == 0 ? 8 : 0), dst_end, dst - offset);

	if (mode == 0)
		return Kraken_ProcessLzRuns_Type0(
			lztable, dst + (offset == 0 ? 8 : 0), dst_end, dst - offset);

	return false;
}

// Decode one 256kb big quantum block. It's divided into two 128k blocks
// internally that are compressed separately but with a shared history.
int Kraken_DecodeQuantum(
	uint8_t *dst, uint8_t *dst_end, uint8_t *dst_start, const uint8_t *src,
	const uint8_t *src_end, uint8_t *scratch, uint8_t *scratch_end)
{
	const uint8_t *src_in = src;
	int mode, chunkhdr, dst_count, src_used, written_bytes;

	while (dst_end - dst != 0)
	{
		dst_count = (int)(dst_end - dst);
		if (dst_count > 0x20000)
			dst_count = 0x20000;
		if (src_end - src < 4)
			return -1;
		chunkhdr = src[2] | src[1] << 8 | src[0] << 16;
		if (!(chunkhdr & 0x800000))
		{
			// Stored as entropy without any match copying.
			uint8_t *out = dst;
			src_used = Kraken_DecodeBytes(
				&out, src, src_end, &written_bytes, dst_count, false, scratch,
				scratch_end);
			if (src_used < 0 || written_bytes != dst_count)
				return -1;
		}
		else
		{
			src += 3;
			src_used = chunkhdr & 0x7FFFF;
			mode = (chunkhdr >> 19) & 0xF;
			if (src_end - src < src_used)
				return -1;
			if (src_used < dst_count)
			{
				size_t scratch_usage =
					Min(Min(3 * dst_count + 32 + 0xd000, 0x6C000),
						scratch_end - scratch);
				if (scratch_usage < sizeof(KrakenLzTable))
					return -1;
				if (!Kraken_ReadLzTable(
						mode, src, src + src_used, dst, dst_count,
						(int)(dst - dst_start),
						scratch + sizeof(KrakenLzTable),
						scratch + scratch_usage, (KrakenLzTable *)scratch))
					return -1;
				if (!Kraken_ProcessLzRuns(
						mode, dst, dst_count, (int)(dst - dst_start),
						(KrakenLzTable *)scratch))
					return -1;
			}
			else if (src_used > dst_count || mode != 0)
			{
				return -1;
			}
			else
			{
				memmove(dst, src, dst_count);
			}
		}
		src += src_used;
		dst += dst_count;
	}
	return (int)(src - src_in);
}

void Kraken_CopyWholeMatch(uint8_t *dst, uint32_t offset, size_t length)
{
	size_t i = 0;
	uint8_t *src = dst - offset;
	if (offset >= 8)
	{
		for (; i + 8 <= length; i += 8)
			*(uint64_t *)(dst + i) = *(uint64_t *)(src + i);
	}
	for (; i < length; i++)
		dst[i] = src[i];
}

bool Kraken_DecodeStep(
	struct KrakenDecoder *dec, uint8_t *dst_start, int offset,
	size_t dst_bytes_left_in, const uint8_t *src, size_t src_bytes_left)
{
	const uint8_t *src_in = src;
	const uint8_t *src_end = src + src_bytes_left;
	KrakenQuantumHeader qhdr;
	uint32_t n;

	if ((offset & 0x3FFFF) == 0)
	{
		src = Kraken_ParseHeader(&dec->hdr, src);
		if (!src)
			return false;
	}

	bool is_kraken_decoder =
		(dec->hdr.decoder_type == 6 || dec->hdr.decoder_type == 10 ||
		 dec->hdr.decoder_type == 12);

	size_t dst_bytes_left =
		Min(is_kraken_decoder ? 0x40000 : 0x4000, dst_bytes_left_in);

	if (dec->hdr.uncompressed)
	{
		if ((size_t)(src_end - src) < dst_bytes_left)
		{
			dec->src_used = dec->dst_used = 0;
			return true;
		}
		memmove(dst_start + offset, src, dst_bytes_left);
		dec->src_used = (int)((src - src_in) + dst_bytes_left);
		dec->dst_used = (int)dst_bytes_left;
		return true;
	}

	if (is_kraken_decoder)
	{
		src = Kraken_ParseQuantumHeader(&qhdr, src, dec->hdr.use_checksums);
	}
	else
	{
		src = LZNA_ParseQuantumHeader(
			&qhdr, src, dec->hdr.use_checksums, (int)dst_bytes_left);
	}

	if (!src || src > src_end)
		return false;

	// Too few bytes in buffer to make any progress?
	if ((uintptr_t)(src_end - src) < qhdr.compressed_size)
	{
		dec->src_used = dec->dst_used = 0;
		return true;
	}

	if (qhdr.compressed_size > (uint32_t)dst_bytes_left)
		return false;

	if (qhdr.compressed_size == 0)
	{
		if (qhdr.whole_match_distance != 0)
		{
			if (qhdr.whole_match_distance > (uint32_t)offset)
				return false;
			Kraken_CopyWholeMatch(
				dst_start + offset, qhdr.whole_match_distance, dst_bytes_left);
		}
		else
		{
			memset(dst_start + offset, qhdr.checksum, dst_bytes_left);
		}
		dec->src_used = (int)(src - src_in);
		dec->dst_used = (int)dst_bytes_left;
		return true;
	}

	if (dec->hdr.use_checksums &&
		(Kraken_GetCrc(src, qhdr.compressed_size) & 0xFFFFFF) != qhdr.checksum)
		return false;

	if (qhdr.compressed_size == dst_bytes_left)
	{
		memmove(dst_start + offset, src, dst_bytes_left);
		dec->src_used = (int)((src - src_in) + dst_bytes_left);
		dec->dst_used = (int)dst_bytes_left;
		return true;
	}

	if (dec->hdr.decoder_type == 6)
	{
		n = Kraken_DecodeQuantum(
			dst_start + offset, dst_start + offset + dst_bytes_left, dst_start,
			src, src + qhdr.compressed_size, dec->scratch,
			dec->scratch + dec->scratch_size);
	}
	else
	{
		return false;
	}

	if (n != qhdr.compressed_size)
		return false;

	dec->src_used = (int)((src - src_in) + n);
	dec->dst_used = (int)dst_bytes_left;
	return true;
}

int Kraken_Decompress(
	const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_len)
{
	KrakenDecoder *dec = Kraken_Create();
	int offset = 0;
	while (dst_len != 0)
	{
		if (!Kraken_DecodeStep(dec, dst, offset, dst_len, src, src_len))
			goto FAIL;
		if (dec->src_used == 0)
			goto FAIL;
		src += dec->src_used;
		src_len -= dec->src_used;
		dst_len -= dec->dst_used;
		offset += dec->dst_used;
	}
	if (src_len != 0)
		goto FAIL;
	Kraken_Destroy(dec);
	return offset;
FAIL:
	Kraken_Destroy(dec);
	return -1;
}
