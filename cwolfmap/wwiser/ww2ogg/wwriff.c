#define __STDC_CONSTANT_MACROS
#include "wwriff.h"
#include "Bit_stream.h"
#include "errors.h"
#include <stdint.h>
#include <string.h>

#include "codebook.h"

/* Modern 2 or 6 byte header */
typedef struct
{
	long _offset;
	uint16_t _size;
	uint32_t _absolute_granule;
	bool _no_granule;
} Packet;

void Packet_init(
	Packet *packet, Wwise_RIFF_Vorbis *decoder, long o, bool little_endian,
	bool no_granule)
{
	packet->_offset = o;
	packet->_size = (uint16_t)-1;
	packet->_absolute_granule = 0;
	packet->_no_granule = no_granule;
	decoder->_data_ptr = decoder->_data + packet->_offset;

	if (little_endian)
	{
		packet->_size = read_16_le_buf((unsigned char *)decoder->_data_ptr);
		decoder->_data_ptr += 2;
		if (!packet->_no_granule)
		{
			packet->_absolute_granule =
				read_32_le_buf((unsigned char *)decoder->_data_ptr);
			decoder->_data_ptr += 4;
		}
	}
	else
	{
		packet->_size = read_16_be_buf((unsigned char *)decoder->_data_ptr);
		decoder->_data_ptr += 2;
		if (!packet->_no_granule)
		{
			packet->_absolute_granule =
				read_32_be_buf((unsigned char *)decoder->_data_ptr);
			decoder->_data_ptr += 4;
		}
	}
}

long Packet_header_size(Packet *packet)
{
	return packet->_no_granule ? 2 : 6;
}
long Packet_offset(Packet *packet)
{
	return packet->_offset + Packet_header_size(packet);
}
uint16_t Packet_size(Packet *packet)
{
	return packet->_size;
}
uint32_t Packet_granule(Packet *packet)
{
	return packet->_absolute_granule;
}
long Packet_next_offset(Packet *packet)
{
	return packet->_offset + Packet_header_size(packet) + packet->_size;
}

/* Old 8 byte header */
typedef struct
{
	long _offset;
	uint32_t _size;
	uint32_t _absolute_granule;
} Packet_8;

void Packet_8_init(
	Packet_8 *packet, Wwise_RIFF_Vorbis *decoder, long o, bool little_endian)
{
	packet->_offset = o;
	packet->_size = (uint32_t)-1;
	packet->_absolute_granule = 0;

	decoder->_data_ptr = decoder->_data + packet->_offset;

	if (little_endian)
	{
		packet->_size = read_32_le_buf((unsigned char *)decoder->_data_ptr);
		decoder->_data_ptr += 4;
		packet->_absolute_granule =
			read_32_le_buf((unsigned char *)decoder->_data_ptr);
		decoder->_data_ptr += 4;
	}
	else
	{
		packet->_size = read_32_be_buf((unsigned char *)decoder->_data_ptr);
		decoder->_data_ptr += 4;
		packet->_absolute_granule =
			read_32_be_buf((unsigned char *)decoder->_data_ptr);
		decoder->_data_ptr += 4;
	}
}

long Packet_8_header_size(Packet_8 *packet)
{
	(void)packet;
	return 8;
}
long Packet_8_offset(Packet_8 *packet)
{
	return packet->_offset + Packet_8_header_size(packet);
}
uint32_t Packet_8_size(Packet_8 *packet)
{
	return packet->_size;
}
uint32_t Packet_8_granule(Packet_8 *packet)
{
	return packet->_absolute_granule;
}
long Packet_8_next_offset(Packet_8 *packet)
{
	return packet->_offset + Packet_8_header_size(packet) + packet->_size;
}

typedef struct
{
	uint8_t type;
} Vorbis_packet_header;

void Bit_oggstream_left_shift_vorbis_packet_header(
	Bit_oggstream *bstream, const Vorbis_packet_header *vph)
{
	Bit_uint t = {vph->type, 8};
	Bit_oggstream_left_shift_bit_uint(bstream, &t);
	static const char vorbis_str[6] = {'v', 'o', 'r', 'b', 'i', 's'};

	for (unsigned int i = 0; i < 6; i++)
	{
		Bit_uint c = {vorbis_str[i], 8};
		Bit_oggstream_left_shift_bit_uint(bstream, &c);
	}
}

int Wwise_RIFF_Vorbis_init(
	Wwise_RIFF_Vorbis *decoder, char *data, long length,
	const uint8_t *codebooks_data, unsigned long codebooks_length,
	bool inline_codebooks, bool full_setup,
	ForcePacketFormat force_packet_format)
{
	decoder->_codebooks_data = codebooks_data;
	decoder->_codebooks_length = codebooks_length;
	decoder->_data = decoder->_data_ptr = data;
	decoder->_length = length;
	decoder->_little_endian = true;
	decoder->_riff_size = -1;
	decoder->_fmt_offset = -1;
	decoder->_cue_offset = -1;
	decoder->_LIST_offset = -1;
	decoder->_smpl_offset = -1;
	decoder->_vorb_offset = -1;
	decoder->_data_offset = -1;
	decoder->_fmt_size = -1;
	decoder->_cue_size = -1;
	decoder->_LIST_size = -1;
	decoder->_smpl_size = -1;
	decoder->_vorb_size = -1;
	decoder->_data_size = -1;
	decoder->_channels = 0;
	decoder->_sample_rate = 0;
	decoder->_avg_bytes_per_second = 0;
	decoder->_ext_unk = 0;
	decoder->_subtype = 0;
	decoder->_cue_count = 0;
	decoder->_loop_count = 0;
	decoder->_loop_start = 0;
	decoder->_loop_end = 0;
	decoder->_sample_count = 0;
	decoder->_setup_packet_offset = 0;
	decoder->_first_audio_packet_offset = 0;
	decoder->_uid = 0;
	decoder->_blocksize_0_pow = 0;
	decoder->_blocksize_1_pow = 0;
	decoder->_inline_codebooks = inline_codebooks;
	decoder->_full_setup = full_setup;
	decoder->_header_triad_present = false;
	decoder->_old_packet_headers = false;
	decoder->_no_granule = false;
	decoder->_mod_packets = false;
	decoder->_read_16 = NULL;
	decoder->_read_32 = NULL;

	// check RIFF header
	{
		unsigned char riff_head[4], wave_head[4];
		memcpy(riff_head, decoder->_data_ptr, 4);
		decoder->_data_ptr += 4;

		if (memcmp(&riff_head[0], "RIFX", 4))
		{
			if (memcmp(&riff_head[0], "RIFF", 4))
			{
				fprintf(stderr, "missing RIFF\n");
				return PARSE_ERROR;
			}
			else
			{
				decoder->_little_endian = true;
			}
		}
		else
		{
			decoder->_little_endian = false;
		}

		if (decoder->_little_endian)
		{
			decoder->_read_16 = read_16_le_buf;
			decoder->_read_32 = read_32_le_buf;
		}
		else
		{
			decoder->_read_16 = read_16_be_buf;
			decoder->_read_32 = read_32_be_buf;
		}

		decoder->_riff_size =
			decoder->_read_32((unsigned char *)decoder->_data_ptr) + 8;
		decoder->_data_ptr += 4;
		if (decoder->_riff_size > decoder->_length)
		{
			fprintf(stderr, "RIFF truncated\n");
			return PARSE_ERROR;
		}

		memcpy(wave_head, decoder->_data_ptr, 4);
		decoder->_data_ptr += 4;
		if (memcmp(&wave_head[0], "WAVE", 4))
		{
			fprintf(stderr, "missing WAVE\n");
			return PARSE_ERROR;
		}
	}

	// read chunks
	long chunk_offset = 12;
	while (chunk_offset < decoder->_riff_size)
	{
		decoder->_data_ptr = decoder->_data + chunk_offset;

		if (chunk_offset + 8 > decoder->_riff_size)
		{
			fprintf(stderr, "chunk truncated\n");
			return PARSE_ERROR;
		}
		char chunk_type[4];
		memcpy(chunk_type, decoder->_data_ptr, 4);
		decoder->_data_ptr += 4;
		uint32_t chunk_size;

		chunk_size = decoder->_read_32((unsigned char *)decoder->_data_ptr);
		decoder->_data_ptr += 4;

		if (!memcmp(chunk_type, "fmt ", 4))
		{
			decoder->_fmt_offset = chunk_offset + 8;
			decoder->_fmt_size = chunk_size;
		}
		else if (!memcmp(chunk_type, "cue ", 4))
		{
			decoder->_cue_offset = chunk_offset + 8;
			decoder->_cue_size = chunk_size;
		}
		else if (!memcmp(chunk_type, "LIST", 4))
		{
			decoder->_LIST_offset = chunk_offset + 8;
			decoder->_LIST_size = chunk_size;
		}
		else if (!memcmp(chunk_type, "smpl", 4))
		{
			decoder->_smpl_offset = chunk_offset + 8;
			decoder->_smpl_size = chunk_size;
		}
		else if (!memcmp(chunk_type, "vorb", 4))
		{
			decoder->_vorb_offset = chunk_offset + 8;
			decoder->_vorb_size = chunk_size;
		}
		else if (!memcmp(chunk_type, "data", 4))
		{
			decoder->_data_offset = chunk_offset + 8;
			decoder->_data_size = chunk_size;
		}

		chunk_offset = chunk_offset + 8 + chunk_size;
	}

	if (chunk_offset > decoder->_riff_size)
	{
		fprintf(stderr, "chunk truncated\n");
		return PARSE_ERROR;
	}

	// check that we have the chunks we're expecting
	if (-1 == decoder->_fmt_offset && -1 == decoder->_data_offset)
	{
		fprintf(stderr, "expected fmt, data chunks\n");
		return PARSE_ERROR;
	}

	// read fmt
	if (-1 == decoder->_vorb_offset && 0x42 != decoder->_fmt_size)
	{
		fprintf(stderr, "expected 0x42 fmt if vorb missing\n");
		return PARSE_ERROR;
	}
	if (-1 != decoder->_vorb_offset && 0x28 != decoder->_fmt_size &&
		0x18 != decoder->_fmt_size && 0x12 != decoder->_fmt_size)
	{
		fprintf(stderr, "bad fmt size\n");
		return PARSE_ERROR;
	}
	if (-1 == decoder->_vorb_offset && 0x42 == decoder->_fmt_size)
	{
		// fake it out
		decoder->_vorb_offset = decoder->_fmt_offset + 0x18;
	}

	decoder->_data_ptr = decoder->_data + decoder->_fmt_offset;
	if (UINT16_C(0xFFFF) !=
		decoder->_read_16((unsigned char *)decoder->_data_ptr))
	{
		fprintf(stderr, "bad codec id\n");
		return PARSE_ERROR;
	}
	decoder->_data_ptr += 2;
	decoder->_channels =
		decoder->_read_16((unsigned char *)decoder->_data_ptr);
	decoder->_data_ptr += 2;
	decoder->_sample_rate =
		decoder->_read_32((unsigned char *)decoder->_data_ptr);
	decoder->_data_ptr += 4;
	decoder->_avg_bytes_per_second =
		decoder->_read_32((unsigned char *)decoder->_data_ptr);
	decoder->_data_ptr += 4;
	if (0U != decoder->_read_16((unsigned char *)decoder->_data_ptr))
	{
		fprintf(stderr, "bad block align\n");
		return PARSE_ERROR;
	}
	decoder->_data_ptr += 2;
	if (0U != decoder->_read_16((unsigned char *)decoder->_data_ptr))
	{
		fprintf(stderr, "expected 0 bps\n");
		return PARSE_ERROR;
	}
	decoder->_data_ptr += 2;
	if (decoder->_fmt_size - 0x12 !=
		decoder->_read_16((unsigned char *)decoder->_data_ptr))
	{
		fprintf(stderr, "bad extra fmt length\n");
		return PARSE_ERROR;
	}
	decoder->_data_ptr += 2;

	if (decoder->_fmt_size - 0x12 >= 2)
	{
		// read extra fmt
		decoder->_ext_unk =
			decoder->_read_16((unsigned char *)decoder->_data_ptr);
		decoder->_data_ptr += 2;
		if (decoder->_fmt_size - 0x12 >= 6)
		{
			decoder->_subtype =
				decoder->_read_32((unsigned char *)decoder->_data_ptr);
			decoder->_data_ptr += 4;
		}
	}

	if (decoder->_fmt_size == 0x28)
	{
		const unsigned char whoknowsbuf_check[16] = {
			1, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9b, 0x71};
		if (memcmp(decoder->_data_ptr, whoknowsbuf_check, 16))
		{
			fprintf(stderr, "expected signature in extra fmt?\n");
			return PARSE_ERROR;
		}
	}

	// read cue
	if (-1 != decoder->_cue_offset)
	{
#if 0
        if (0x1c != decoder->_cue_size) throw Parse_error_str("bad cue size");
#endif
		decoder->_data_ptr = decoder->_data + decoder->_cue_offset;

		decoder->_cue_count =
			decoder->_read_32((unsigned char *)decoder->_data_ptr);
		decoder->_data_ptr += 4;
	}

	// read LIST
	if (-1 != decoder->_LIST_offset)
	{
#if 0
        if ( 4 != decoder->_LIST_size ) throw Parse_error_str("bad LIST size");
        char adtlbuf[4];
        const char adtlbuf_check[4] = {'a','d','t','l'};
        fseek(decoder->_infile, decoder->_LIST_offset, SEEK_SET);
        fread(adtlbuf, 1, 4, decoder->_infile);
        if (memcmp(adtlbuf, adtlbuf_check, 4)) throw Parse_error_str("expected only adtl in LIST");
#endif
	}

	// read smpl
	if (-1 != decoder->_smpl_offset)
	{
		decoder->_data_ptr = decoder->_data + decoder->_smpl_offset + 0x1C;
		decoder->_loop_count =
			decoder->_read_32((unsigned char *)decoder->_data_ptr);
		decoder->_data_ptr += 4;

		if (1 != decoder->_loop_count)
		{
			fprintf(stderr, "expected one loop\n");
			return PARSE_ERROR;
		}

		decoder->_data_ptr = decoder->_data + decoder->_smpl_offset + 0x2C;
		decoder->_loop_start =
			decoder->_read_32((unsigned char *)decoder->_data_ptr);
		decoder->_data_ptr += 4;
		decoder->_loop_end =
			decoder->_read_32((unsigned char *)decoder->_data_ptr);
		decoder->_data_ptr += 4;
	}

	// read vorb
	switch (decoder->_vorb_size)
	{
	case -1:
	case 0x28:
	case 0x2A:
	case 0x2C:
	case 0x32:
	case 0x34:
		decoder->_data_ptr = decoder->_data + decoder->_vorb_offset + 0x00;
		break;

	default:
		fprintf(stderr, "bad vorb size\n");
		return PARSE_ERROR;
		break;
	}

	decoder->_sample_count =
		decoder->_read_32((unsigned char *)decoder->_data_ptr);
	decoder->_data_ptr += 4;

	switch (decoder->_vorb_size)
	{
	case -1:
	case 0x2A: {
		decoder->_no_granule = true;

		decoder->_data_ptr = decoder->_data + decoder->_vorb_offset + 0x4;
		uint32_t mod_signal =
			decoder->_read_32((unsigned char *)decoder->_data_ptr);
		decoder->_data_ptr += 4;

		// set
		// D9     11011001
		// CB     11001011
		// BC     10111100
		// B2     10110010
		// unset
		// 4A     01001010
		// 4B     01001011
		// 69     01101001
		// 70     01110000
		// A7     10100111 !!!

		// seems to be 0xD9 when _mod_packets should be set
		// also seen 0xCB, 0xBC, 0xB2
		if (0x4A != mod_signal && 0x4B != mod_signal && 0x69 != mod_signal &&
			0x70 != mod_signal)
		{
			decoder->_mod_packets = true;
		}
		decoder->_data_ptr = decoder->_data + decoder->_vorb_offset + 0x10;
		break;
	}

	default:
		decoder->_data_ptr = decoder->_data + decoder->_vorb_offset + 0x18;
		break;
	}

	if (force_packet_format == kForceNoModPackets)
	{
		decoder->_mod_packets = false;
	}
	else if (force_packet_format == kForceModPackets)
	{
		decoder->_mod_packets = true;
	}

	decoder->_setup_packet_offset =
		decoder->_read_32((unsigned char *)decoder->_data_ptr);
	decoder->_data_ptr += 4;
	decoder->_first_audio_packet_offset =
		decoder->_read_32((unsigned char *)decoder->_data_ptr);
	decoder->_data_ptr += 4;
	switch (decoder->_vorb_size)
	{
	case -1:
	case 0x2A:
		decoder->_data_ptr = decoder->_data + decoder->_vorb_offset + 0x24;
		break;

	case 0x32:
	case 0x34:
		decoder->_data_ptr = decoder->_data + decoder->_vorb_offset + 0x2C;
		break;
	}

	switch (decoder->_vorb_size)
	{
	case 0x28:
	case 0x2C:
		// ok to leave _uid, _blocksize_0_pow and _blocksize_1_pow unset
		decoder->_header_triad_present = true;
		decoder->_old_packet_headers = true;
		break;

	case -1:
	case 0x2A:
	case 0x32:
	case 0x34:
		decoder->_uid = decoder->_read_32((unsigned char *)decoder->_data_ptr);
		decoder->_data_ptr += 4;
		decoder->_blocksize_0_pow = *((uint8_t *)decoder->_data_ptr);
		decoder->_data_ptr += 1;
		decoder->_blocksize_1_pow = *((uint8_t *)decoder->_data_ptr);
		decoder->_data_ptr += 1;
		break;
	}

	// check/set loops now that we know total sample count
	if (0 != decoder->_loop_count)
	{
		if (decoder->_loop_end == 0)
		{
			decoder->_loop_end = decoder->_sample_count;
		}
		else
		{
			decoder->_loop_end = decoder->_loop_end + 1;
		}

		if (decoder->_loop_start >= decoder->_sample_count ||
			decoder->_loop_end > decoder->_sample_count ||
			decoder->_loop_start > decoder->_loop_end)
		{
			fprintf(stderr, "loops out of range\n");
			return PARSE_ERROR;
		}
	}

	// check subtype now that we know the vorb info
	// this is clearly just the channel layout
	switch (decoder->_subtype)
	{
	case 4:	   /* 1 channel, no seek table */
	case 3:	   /* 2 channels */
	case 0x33: /* 4 channels */
	case 0x37: /* 5 channels, seek or not */
	case 0x3b: /* 5 channels, no seek table */
	case 0x3f: /* 6 channels, no seek table */
		break;
	default:
		// throw Parse_error_str("unknown subtype");
		break;
	}

	return 0;
}

void generate_ogg_header(
	Wwise_RIFF_Vorbis *decoder, Bit_oggstream *os, bool **mode_blockflag,
	int *mode_bits)
{
	// generate identification packet
	{
		Vorbis_packet_header vhead;
		vhead.type = 1;

		Bit_oggstream_left_shift_vorbis_packet_header(os, &vhead);

		Bit_uint version = {0, 32};
		Bit_oggstream_left_shift_bit_uint(os, &version);

		Bit_uint ch = {decoder->_channels, 8};
		Bit_oggstream_left_shift_bit_uint(os, &ch);

		Bit_uint srate = {decoder->_sample_rate, 32};
		Bit_oggstream_left_shift_bit_uint(os, &srate);

		Bit_uint bitrate_max = {0, 32};
		Bit_oggstream_left_shift_bit_uint(os, &bitrate_max);

		Bit_uint bitrate_nominal = {decoder->_avg_bytes_per_second * 8, 32};
		Bit_oggstream_left_shift_bit_uint(os, &bitrate_nominal);

		Bit_uint bitrate_minimum = {0, 32};
		Bit_oggstream_left_shift_bit_uint(os, &bitrate_minimum);

		Bit_uint blocksize_0 = {decoder->_blocksize_0_pow, 4};
		Bit_oggstream_left_shift_bit_uint(os, &blocksize_0);

		Bit_uint blocksize_1 = {decoder->_blocksize_1_pow, 4};
		Bit_oggstream_left_shift_bit_uint(os, &blocksize_1);

		Bit_uint framing = {1, 1};
		Bit_oggstream_left_shift_bit_uint(os, &framing);

		// identification packet on its own page
		Bit_oggstream_flush_page(os, false, false);
	}

	// generate comment packet
	{
		Vorbis_packet_header vhead;
		vhead.type = 3;

		Bit_oggstream_left_shift_vorbis_packet_header(os, &vhead);

		static const char vendor[] =
			"converted from Audiokinetic Wwise by ww2ogg " VERSION;
		Bit_uint vendor_size = {(unsigned int)strlen(vendor), 32};

		Bit_oggstream_left_shift_bit_uint(os, &vendor_size);
		for (unsigned int i = 0; i < vendor_size.total; i++)
		{
			Bit_uint c = {vendor[i], 8};
			Bit_oggstream_left_shift_bit_uint(os, &c);
		}

		if (0 == decoder->_loop_count)
		{
			// no user comments
			Bit_uint user_comment_count = {0, 32};
			Bit_oggstream_left_shift_bit_uint(os, &user_comment_count);
		}
		else
		{
			// two comments, loop start and end
			Bit_uint user_comment_count = {2, 32};
			Bit_oggstream_left_shift_bit_uint(os, &user_comment_count);

			char loop_start_str[64];
			char loop_end_str[64];

			sprintf(loop_start_str, "LoopStart=%u", decoder->_loop_start);
			sprintf(loop_end_str, "LoopEnd=%u", decoder->_loop_end);

			Bit_uint loop_start_comment_length = {
				(unsigned int)strlen(loop_start_str), 32};
			Bit_oggstream_left_shift_bit_uint(os, &loop_start_comment_length);
			for (unsigned int i = 0; i < loop_start_comment_length.total; i++)
			{
				Bit_uint c = {loop_start_str[i], 8};
				Bit_oggstream_left_shift_bit_uint(os, &c);
			}

			Bit_uint loop_end_comment_length = {
				(unsigned int)strlen(loop_end_str), 32};
			Bit_oggstream_left_shift_bit_uint(os, &loop_end_comment_length);
			for (unsigned int i = 0; i < loop_end_comment_length.total; i++)
			{
				Bit_uint c = {loop_end_str[i], 8};
				Bit_oggstream_left_shift_bit_uint(os, &c);
			}
		}

		Bit_uint framing = {1, 1};
		Bit_oggstream_left_shift_bit_uint(os, &framing);

		Bit_oggstream_flush_page(os, false, false);
	}

	// generate setup packet
	{
		Vorbis_packet_header vhead;
		vhead.type = 5;

		Bit_oggstream_left_shift_vorbis_packet_header(os, &vhead);

		Packet setup_packet;
		Packet_init(
			&setup_packet, decoder,
			decoder->_data_offset + decoder->_setup_packet_offset,
			decoder->_little_endian, decoder->_no_granule);

		decoder->_data_ptr = decoder->_data + Packet_offset(&setup_packet);
		if (Packet_granule(&setup_packet) != 0)
		{
			fprintf(stderr, "setup packet granule != 0\n");
		}
		Bit_stream ss;
		Bit_stream_init(&ss, &decoder->_data_ptr);

		// codebook count
		Bit_uint codebook_count_less1 = {0, 8};
		Bit_stream_right_shift_bit_uint(&ss, &codebook_count_less1);
		unsigned int codebook_count = codebook_count_less1.total + 1;
		Bit_oggstream_left_shift_bit_uint(os, &codebook_count_less1);

		// rebuild codebooks
		if (decoder->_inline_codebooks)
		{
			codebook_library cbl;
			codebook_library_init(&cbl);

			for (unsigned int i = 0; i < codebook_count; i++)
			{
				if (decoder->_full_setup)
				{
					codebook_library_copy(&cbl, &ss, os);
				}
				else
				{
					codebook_library_rebuild(&cbl, &ss, 0, os);
				}
			}
		}
		else
		{
			/* external codebooks */

			codebook_library cbl;
			codebook_library_init_from_data(
				&cbl, decoder->_codebooks_data, decoder->_codebooks_length);

			for (unsigned int i = 0; i < codebook_count; i++)
			{
				Bit_uint codebook_id = {0, 10};
				Bit_stream_right_shift_bit_uint(&ss, &codebook_id);
				codebook_library_rebuild_ogg(&cbl, codebook_id.total, os);
			}
		}

		// Time Domain transforms (placeholder)
		Bit_uint time_count_less1 = {0, 6};
		Bit_oggstream_left_shift_bit_uint(os, &time_count_less1);
		Bit_uint dummy_time_value = {0, 16};
		Bit_oggstream_left_shift_bit_uint(os, &dummy_time_value);

		// if (decoder->_full_setup)
		// {

		//     while (ss->total_bits_read < setup_packet.size()*8u)
		//     {
		//         Bit_uint bitly = {0, 1};
		//         Bit_stream_right_shift_bit_uint(ss, &bitly);
		//         Bit_oggstream_left_shift_bit_uint(os, &bitly);
		//     }
		// }
		// else    // _full_setup
		{
			// floor count
			Bit_uint floor_count_less1 = {0, 6};
			Bit_stream_right_shift_bit_uint(&ss, &floor_count_less1);
			unsigned int floor_count = floor_count_less1.total + 1;
			Bit_oggstream_left_shift_bit_uint(os, &floor_count_less1);

			// rebuild floors
			for (unsigned int i = 0; i < floor_count; i++)
			{
				// Always floor type 1
				Bit_uint floor_type = {1, 16};
				Bit_oggstream_left_shift_bit_uint(os, &floor_type);

				Bit_uint floor1_partitions = {0, 5};
				Bit_stream_right_shift_bit_uint(&ss, &floor1_partitions);
				Bit_oggstream_left_shift_bit_uint(os, &floor1_partitions);
				unsigned int *floor1_partition_class_list =
					malloc(sizeof(unsigned int) * floor1_partitions.total);

				unsigned int maximum_class = 0;
				for (unsigned int j = 0; j < floor1_partitions.total; j++)
				{
					Bit_uint floor1_partition_class = {0, 4};
					Bit_stream_right_shift_bit_uint(
						&ss, &floor1_partition_class);
					Bit_oggstream_left_shift_bit_uint(
						os, &floor1_partition_class);

					floor1_partition_class_list[j] =
						floor1_partition_class.total;

					if (floor1_partition_class.total > maximum_class)
						maximum_class = floor1_partition_class.total;
				}

				unsigned int *floor1_class_dimensions_list =
					malloc(sizeof(unsigned int) * (maximum_class + 1));

				for (unsigned int j = 0; j <= maximum_class; j++)
				{
					Bit_uint class_dimensions_less1 = {0, 3};
					Bit_stream_right_shift_bit_uint(
						&ss, &class_dimensions_less1);
					Bit_oggstream_left_shift_bit_uint(
						os, &class_dimensions_less1);

					floor1_class_dimensions_list[j] =
						class_dimensions_less1.total + 1;
					Bit_uint class_subclasses = {0, 2};
					Bit_stream_right_shift_bit_uint(&ss, &class_subclasses);
					Bit_oggstream_left_shift_bit_uint(os, &class_subclasses);

					if (0 != class_subclasses.total)
					{
						Bit_uint masterbook = {0, 8};
						Bit_stream_right_shift_bit_uint(&ss, &masterbook);
						Bit_oggstream_left_shift_bit_uint(os, &masterbook);

						if (masterbook.total >= codebook_count)
						{
							fprintf(stderr, "invalid floor1 masterbook\n");
						}
					}

					for (unsigned int k = 0;
						 k < (1U << class_subclasses.total); k++)
					{
						Bit_uint subclass_book_plus1 = {0, 8};
						Bit_stream_right_shift_bit_uint(
							&ss, &subclass_book_plus1);
						Bit_oggstream_left_shift_bit_uint(
							os, &subclass_book_plus1);

						int subclass_book =
							(int)(subclass_book_plus1.total) - 1;
						if (subclass_book >= 0 &&
							(unsigned int)(subclass_book) >= codebook_count)
						{
							fprintf(stderr, "invalid floor1 subclass book\n");
						}
					}
				}

				Bit_uint floor1_multiplier_less1 = {0, 2};
				Bit_stream_right_shift_bit_uint(&ss, &floor1_multiplier_less1);
				Bit_oggstream_left_shift_bit_uint(
					os, &floor1_multiplier_less1);

				Bit_uint rangebits = {0, 4};
				Bit_stream_right_shift_bit_uint(&ss, &rangebits);
				Bit_oggstream_left_shift_bit_uint(os, &rangebits);
				for (unsigned int j = 0; j < floor1_partitions.total; j++)
				{
					unsigned int current_class_number =
						floor1_partition_class_list[j];
					for (unsigned int k = 0;
						 k <
						 floor1_class_dimensions_list[current_class_number];
						 k++)
					{
						Bit_uintv X;
						Bit_uintv_init(&X, rangebits.total);
						Bit_stream_right_shift_bit_uintv(&ss, &X);
						Bit_oggstream_left_shift_bit_uintv(os, &X);
					}
				}

				free(floor1_class_dimensions_list);
				free(floor1_partition_class_list);
			}

			// residue count
			Bit_uint residue_count_less1 = {0, 6};
			Bit_stream_right_shift_bit_uint(&ss, &residue_count_less1);
			unsigned int residue_count = residue_count_less1.total + 1;
			Bit_oggstream_left_shift_bit_uint(os, &residue_count_less1);

			// rebuild residues
			for (unsigned int i = 0; i < residue_count; i++)
			{
				Bit_uint residue_type = {0, 2};
				Bit_stream_right_shift_bit_uint(&ss, &residue_type);
				Bit_uint residue_type_out = {residue_type.total, 16};
				Bit_oggstream_left_shift_bit_uint(os, &residue_type_out);

				if (residue_type.total > 2)
				{
					fprintf(stderr, "invalid residue type\n");
					return;
				}
				Bit_uint residue_begin = {0, 24}, residue_end = {0, 24},
						 residue_partition_size_less1 = {0, 24};
				Bit_uint residue_classifications_less1 = {0, 6};
				Bit_uint residue_classbook = {0, 8};

				Bit_stream_right_shift_bit_uint(&ss, &residue_begin);
				Bit_stream_right_shift_bit_uint(&ss, &residue_end);
				Bit_stream_right_shift_bit_uint(
					&ss, &residue_partition_size_less1);
				Bit_stream_right_shift_bit_uint(
					&ss, &residue_classifications_less1);
				Bit_stream_right_shift_bit_uint(&ss, &residue_classbook);
				unsigned int residue_classifications =
					residue_classifications_less1.total + 1;
				Bit_oggstream_left_shift_bit_uint(os, &residue_begin);
				Bit_oggstream_left_shift_bit_uint(os, &residue_end);
				Bit_oggstream_left_shift_bit_uint(
					os, &residue_partition_size_less1);
				Bit_oggstream_left_shift_bit_uint(
					os, &residue_classifications_less1);
				Bit_oggstream_left_shift_bit_uint(os, &residue_classbook);

				if (residue_classbook.total >= codebook_count)
				{
					fprintf(stderr, "invalid residue classbook\n");
					return;
				}

				unsigned int *residue_cascade = (unsigned int *)malloc(
					residue_classifications * sizeof(unsigned int));

				for (unsigned int j = 0; j < residue_classifications; j++)
				{
					Bit_uint high_bits = {0, 5};
					Bit_uint low_bits = {0, 3};

					Bit_stream_right_shift_bit_uint(&ss, &low_bits);
					Bit_oggstream_left_shift_bit_uint(os, &low_bits);

					Bit_uint bitflag = {0, 1};
					Bit_stream_right_shift_bit_uint(&ss, &bitflag);
					Bit_oggstream_left_shift_bit_uint(os, &bitflag);
					if (bitflag.total)
					{
						Bit_stream_right_shift_bit_uint(&ss, &high_bits);
						Bit_oggstream_left_shift_bit_uint(os, &high_bits);
					}

					residue_cascade[j] = high_bits.total * 8 + low_bits.total;
				}

				for (unsigned int j = 0; j < residue_classifications; j++)
				{
					for (unsigned int k = 0; k < 8; k++)
					{
						if (residue_cascade[j] & (1 << k))
						{
							Bit_uint residue_book = {0, 8};
							Bit_stream_right_shift_bit_uint(
								&ss, &residue_book);
							Bit_oggstream_left_shift_bit_uint(
								os, &residue_book);

							if (residue_book.total >= codebook_count)
							{
								fprintf(stderr, "invalid residue book\n");
								return;
							}
						}
					}
				}

				free(residue_cascade);
			}

			// mapping count
			Bit_uint mapping_count_less1 = {0, 6};
			Bit_stream_right_shift_bit_uint(&ss, &mapping_count_less1);
			unsigned int mapping_count = mapping_count_less1.total + 1;
			Bit_oggstream_left_shift_bit_uint(os, &mapping_count_less1);

			for (unsigned int i = 0; i < mapping_count; i++)
			{
				// always mapping type 0, the only one
				Bit_uint mapping_type = {0, 16};

				Bit_oggstream_left_shift_bit_uint(os, &mapping_type);

				Bit_uint submaps_flag = {0, 1};
				Bit_stream_right_shift_bit_uint(&ss, &submaps_flag);
				Bit_oggstream_left_shift_bit_uint(os, &submaps_flag);
				unsigned int submaps = 1;
				if (submaps_flag.total)
				{
					Bit_uint submaps_less1 = {0, 4};

					Bit_stream_right_shift_bit_uint(&ss, &submaps_less1);
					submaps = submaps_less1.total + 1;
					Bit_oggstream_left_shift_bit_uint(os, &submaps_less1);
				}

				Bit_uint square_polar_flag = {0, 1};
				Bit_stream_right_shift_bit_uint(&ss, &square_polar_flag);
				Bit_oggstream_left_shift_bit_uint(os, &square_polar_flag);

				if (square_polar_flag.total)
				{
					Bit_uint coupling_steps_less1 = {0, 8};
					Bit_stream_right_shift_bit_uint(
						&ss, &coupling_steps_less1);
					unsigned int coupling_steps =
						coupling_steps_less1.total + 1;
					Bit_oggstream_left_shift_bit_uint(
						os, &coupling_steps_less1);

					for (unsigned int j = 0; j < coupling_steps; j++)
					{
						Bit_uintv magnitude;
						Bit_uintv angle;
						Bit_uintv_init(
							&magnitude, ilog(decoder->_channels - 1));
						Bit_uintv_init(&angle, ilog(decoder->_channels - 1));

						Bit_stream_right_shift_bit_uintv(&ss, &magnitude);
						Bit_stream_right_shift_bit_uintv(&ss, &angle);
						Bit_oggstream_left_shift_bit_uintv(os, &magnitude);
						Bit_oggstream_left_shift_bit_uintv(os, &angle);

						if (angle.total == magnitude.total ||
							magnitude.total >= decoder->_channels ||
							angle.total >= decoder->_channels)
						{
							fprintf(stderr, "invalid coupling step\n");
							return;
						}
					}
				}

				// a rare reserved field not removed by Ak!
				Bit_uint mapping_reserved = {0, 2};
				Bit_stream_right_shift_bit_uint(&ss, &mapping_reserved);
				Bit_oggstream_left_shift_bit_uint(os, &mapping_reserved);
				if (0 != mapping_reserved.total)
				{
					fprintf(stderr, "mapping reserved bits not zero\n");
					return;
				}

				if (submaps > 1)
				{
					for (unsigned int j = 0; j < decoder->_channels; j++)
					{
						Bit_uint mapping_mux = {0, 4};
						Bit_stream_right_shift_bit_uint(&ss, &mapping_mux);
						Bit_oggstream_left_shift_bit_uint(os, &mapping_mux);

						if (mapping_mux.total >= submaps)
						{
							fprintf(stderr, "invalid mapping mux\n");
							return;
						}
					}
				}

				for (unsigned int j = 0; j < submaps; j++)
				{
					// Another! Unused time domain transform configuration
					// placeholder!
					Bit_uint time_config = {0, 8};
					Bit_stream_right_shift_bit_uint(&ss, &time_config);
					Bit_oggstream_left_shift_bit_uint(os, &time_config);

					Bit_uint floor_number = {0, 8};
					Bit_stream_right_shift_bit_uint(&ss, &floor_number);
					Bit_oggstream_left_shift_bit_uint(os, &floor_number);
					if (floor_number.total >= floor_count)
					{
						fprintf(stderr, "invalid floor mapping\n");
						return;
					}

					Bit_uint residue_number = {0, 8};
					Bit_stream_right_shift_bit_uint(&ss, &residue_number);
					Bit_oggstream_left_shift_bit_uint(os, &residue_number);
					if (residue_number.total >= residue_count)
					{
						fprintf(stderr, "invalid residue mapping\n");
						return;
					}
				}
			}

			// mode count
			Bit_uint mode_count_less1 = {0, 6};
			Bit_stream_right_shift_bit_uint(&ss, &mode_count_less1);
			unsigned int mode_count = mode_count_less1.total + 1;
			Bit_oggstream_left_shift_bit_uint(os, &mode_count_less1);

			*mode_blockflag = malloc(sizeof(bool) * mode_count);
			*mode_bits = ilog(mode_count - 1);

			for (unsigned int i = 0; i < mode_count; i++)
			{
				Bit_uint block_flag = {0, 1};
				Bit_stream_right_shift_bit_uint(&ss, &block_flag);
				Bit_oggstream_left_shift_bit_uint(os, &block_flag);

				(*mode_blockflag)[i] = (block_flag.total != 0);
				// only 0 valid for windowtype and transformtype
				Bit_uint windowtype = {0, 16}, transformtype = {0, 16};
				Bit_oggstream_left_shift_bit_uint(os, &windowtype);
				Bit_oggstream_left_shift_bit_uint(os, &transformtype);

				Bit_uint mapping = {0, 8};
				Bit_stream_right_shift_bit_uint(&ss, &mapping);
				Bit_oggstream_left_shift_bit_uint(os, &mapping);
				if (mapping.total >= mapping_count)
				{
					fprintf(stderr, "invalid mode mapping\n");
					return;
				}
			}

			Bit_uint framing = {1, 1};
			Bit_oggstream_left_shift_bit_uint(os, &framing);

		} // _full_setup

		Bit_oggstream_flush_page(os, false, false);

		if ((ss.total_bits_read + 7) / 8 != Packet_size(&setup_packet))
		{
			fprintf(stderr, "didn't read exactly setup packet\n");
			return;
		}

		if (Packet_next_offset(&setup_packet) !=
			decoder->_data_offset +
				(long)(decoder->_first_audio_packet_offset))
		{
			fprintf(
				stderr, "first audio packet doesn't follow setup packet\n");
			return;
		}
	}
}

void generate_ogg_header_with_triad(
	Wwise_RIFF_Vorbis *decoder, Bit_oggstream *os);

char *Wwise_RIFF_Vorbis_generate_ogg(
	Wwise_RIFF_Vorbis *decoder, long *out_length)
{
	CArray arr;
	CArrayInit(&arr, 1);
	Bit_oggstream os;
	Bit_oggstream_init(&os, &arr);

	bool *mode_blockflag = NULL;
	int mode_bits = 0;
	bool prev_blockflag = false;

	if (decoder->_header_triad_present)
	{
		generate_ogg_header_with_triad(decoder, &os);
	}
	else
	{
		generate_ogg_header(decoder, &os, &mode_blockflag, &mode_bits);
	}

	// Audio pages
	{
		long offset =
			decoder->_data_offset + decoder->_first_audio_packet_offset;

		while (offset < decoder->_data_offset + decoder->_data_size)
		{
			uint32_t size, granule;
			long packet_header_size, packet_payload_offset, next_offset;

			if (decoder->_old_packet_headers)
			{
				Packet_8 audio_packet;
				Packet_8_init(
					&audio_packet, decoder, offset, decoder->_little_endian);
				packet_header_size = Packet_8_header_size(&audio_packet);
				size = Packet_8_size(&audio_packet);
				packet_payload_offset = Packet_8_offset(&audio_packet);
				granule = Packet_8_granule(&audio_packet);
				next_offset = Packet_8_next_offset(&audio_packet);
			}
			else
			{
				Packet audio_packet;
				Packet_init(
					&audio_packet, decoder, offset, decoder->_little_endian,
					decoder->_no_granule);
				packet_header_size = Packet_header_size(&audio_packet);
				size = Packet_size(&audio_packet);
				packet_payload_offset = Packet_offset(&audio_packet);
				granule = Packet_granule(&audio_packet);
				next_offset = Packet_next_offset(&audio_packet);
			}

			if (offset + packet_header_size >
				decoder->_data_offset + decoder->_data_size)
			{
				fprintf(stderr, "packet header truncated\n");
				return NULL;
			}

			offset = packet_payload_offset;

			decoder->_data_ptr = decoder->_data + offset;
			// HACK: don't know what to do here
			if (granule == UINT32_C(0xFFFFFFFF))
			{
				os.granule = 1;
			}
			else
			{
				os.granule = granule;
			}

			// first byte
			if (decoder->_mod_packets)
			{
				// need to rebuild packet type and window info

				if (!mode_blockflag)
				{
					fprintf(stderr, "didn't load mode_blockflag\n");
					return NULL;
				}

				// OUT: 1 bit packet type (0 == audio)
				Bit_uint packet_type = {0, 1};
				Bit_oggstream_left_shift_bit_uint(&os, &packet_type);

				Bit_uintv *mode_number_p = 0;
				Bit_uintv *remainder_p = 0;

				{
					// collect mode number from first byte

					Bit_stream ss;
					Bit_stream_init(&ss, &decoder->_data_ptr);

					// IN/OUT: N bit mode number (max 6 bits)
					mode_number_p = malloc(sizeof(Bit_uintv));
					Bit_uintv_init(mode_number_p, mode_bits);
					Bit_stream_right_shift_bit_uintv(&ss, mode_number_p);
					Bit_oggstream_left_shift_bit_uintv(&os, mode_number_p);

					// IN: remaining bits of first (input) byte
					remainder_p = malloc(sizeof(Bit_uintv));
					Bit_uintv_init(remainder_p, 8 - mode_bits);
					Bit_stream_right_shift_bit_uintv(&ss, remainder_p);
				}

				if (mode_blockflag[mode_number_p->total])
				{
					// long window, peek at next frame

					decoder->_data_ptr = decoder->_data + next_offset;
					bool next_blockflag = false;
					if (next_offset + packet_header_size <=
						decoder->_data_offset + decoder->_data_size)
					{

						// mod_packets always goes with 6-byte headers
						Packet audio_packet;
						Packet_init(
							&audio_packet, decoder, next_offset,
							decoder->_little_endian, decoder->_no_granule);
						uint32_t next_packet_size = Packet_size(&audio_packet);
						if (next_packet_size > 0)
						{
							decoder->_data_ptr =
								decoder->_data + Packet_offset(&audio_packet);

							Bit_stream ss;
							Bit_stream_init(&ss, &decoder->_data_ptr);
							Bit_uintv next_mode_number;
							Bit_uintv_init(&next_mode_number, mode_bits);

							Bit_stream_right_shift_bit_uintv(
								&ss, &next_mode_number);

							next_blockflag =
								mode_blockflag[next_mode_number.total];
						}
					}

					// OUT: previous window type bit
					Bit_uint prev_window_type = {prev_blockflag, 1};
					Bit_oggstream_left_shift_bit_uint(&os, &prev_window_type);

					// OUT: next window type bit
					Bit_uint next_window_type = {next_blockflag, 1};
					Bit_oggstream_left_shift_bit_uint(&os, &next_window_type);
					// fix seek for rest of stream
					decoder->_data_ptr = decoder->_data + offset + 1;
				}

				prev_blockflag = mode_blockflag[mode_number_p->total];
				free(mode_number_p);

				// OUT: remaining bits of first (input) byte
				Bit_oggstream_left_shift_bit_uintv(&os, remainder_p);
				free(remainder_p);
			}
			else
			{
				// nothing unusual for first byte
				int v = decoder->_data_ptr[0];
				decoder->_data_ptr++;
				if (v < 0)
				{
					fprintf(stderr, "file truncated\n");
					return NULL;
				}
				Bit_uint c = {v, 8};
				Bit_oggstream_left_shift_bit_uint(&os, &c);
			}

			// remainder of packet
			for (unsigned int i = 1; i < size; i++)
			{
				int v = (unsigned char)decoder->_data_ptr[0];
				decoder->_data_ptr++;
				if (decoder->_data_ptr - decoder->_data > decoder->_length)
				{
					fprintf(stderr, "file truncated\n");
					return NULL;
				}
				Bit_uint c = {v, 8};
				Bit_oggstream_left_shift_bit_uint(&os, &c);
			}

			offset = next_offset;
			Bit_oggstream_flush_page(
				&os, false,
				(offset == decoder->_data_offset + decoder->_data_size));
		}
		if (offset > decoder->_data_offset + decoder->_data_size)
		{
			fprintf(stderr, "page truncated\n");
			return NULL;
		}
	}

	free(mode_blockflag);

	Bit_oggstream_flush_page(&os, false, false);
	*out_length = (long) arr.size;
	return (char *)arr.data;
}

void generate_ogg_header_with_triad(
	Wwise_RIFF_Vorbis *decoder, Bit_oggstream *os)
{
	(decoder);
	(void)os;
#if 0
    // Header page triad
    {
        long offset = _data_offset + _setup_packet_offset;

        // copy information packet
        {
            Packet_8 information_packet(_infile, offset, _little_endian);
            uint32_t size = information_packet.size();

            if (information_packet.granule() != 0)
            {
                throw Parse_error_str("information packet granule != 0");
            }

            _infile.seekg(information_packet.offset());

            Bit_uint c = { _infile.get(), 8 };
            if (1 != c.total)
            {
                throw Parse_error_str("wrong type for information packet");
            }

            Bit_oggstream_left_shift_bit_uint(os, &c);

            for (unsigned int i = 1; i < size; i++)
            {
                c.total = _infile.get();
                Bit_oggstream_left_shift_bit_uint(os, &c);
            }

            // identification packet on its own page
            Bit_oggstream_flush_page(&os, false, false);

            offset = information_packet.next_offset();
        }

        // copy comment packet 
        {
            Packet_8 comment_packet(_infile, offset, _little_endian);
            uint16_t size = comment_packet.size();

            if (comment_packet.granule() != 0)
            {
                throw Parse_error_str("comment packet granule != 0");
            }

            _infile.seekg(comment_packet.offset());

            Bit_uint c = { _infile.get(), 8 };
            if (3 != c.total)
            {
                throw Parse_error_str("wrong type for comment packet");
            }

            Bit_oggstream_left_shift_bit_uint(os, &c);

            for (unsigned int i = 1; i < size; i++)
            {
                c.total = _infile.get();
                Bit_oggstream_left_shift_bit_uint(os, &c);
            }

            // identification packet on its own page
            Bit_oggstream_flush_page(&os, false, false);

            offset = comment_packet.next_offset();
        }

        // copy setup packet
        {
            Packet_8 setup_packet(_infile, offset, _little_endian);

            _infile.seekg(setup_packet.offset());
            if (setup_packet.granule() != 0) throw Parse_error_str("setup packet granule != 0");
            Bit_stream ss(_infile);

            Bit_uint c = {0, 8};
            Bit_stream_right_shift_bit_uint(&ss, &c);

            // type
            if (5 != c.total)
            {
                throw Parse_error_str("wrong type for setup packet");
            }
            Bit_oggstream_left_shift_bit_uint(os, &c);

            // 'vorbis'
            for (unsigned int i = 0; i < 6; i++)
            {
                Bit_stream_right_shift_bit_uint(&ss, &c);
                Bit_oggstream_left_shift_bit_uint(os, &c);
            }

            // codebook count
            Bit_uint codebook_count_less1 = {0, 8};
            Bit_stream_right_shift_bit_uint(&ss, &codebook_count_less1);
            unsigned int codebook_count = codebook_count_less1.total + 1;
            Bit_oggstream_left_shift_bit_uint(os, &codebook_count_less1);

            codebook_library cbl;

            // rebuild codebooks
            for (unsigned int i = 0; i < codebook_count; i++)
            {
                cbl.copy(ss, os);
            }

            while (ss->total_bits_read < setup_packet.size()*8u)
            {
                Bit_uint bitly = {0, 1};
                Bit_stream_right_shift_bit_uint(&ss, &bitly);
                Bit_oggstream_left_shift_bit_uint(os, &bitly);
            }

            Bit_oggstream_flush_page(&os, false, false);

            offset = setup_packet.next_offset();
        }

        if (offset != _data_offset + static_cast<long>(_first_audio_packet_offset)) throw Parse_error_str("first audio packet doesn't follow setup packet");

    }
#endif
}
