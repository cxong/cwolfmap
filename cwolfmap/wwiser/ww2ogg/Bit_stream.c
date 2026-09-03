#include "Bit_stream.h"

// host-endian-neutral integer reading
uint32_t read_32_le_buf(unsigned char b[4])
{
    uint32_t v = 0;
    for (int i = 3; i >= 0; i--)
    {
        v <<= 8;
        v |= b[i];
    }

    return v;
}

uint32_t read_32_le(FILE *f)
{
    char b[4];
    fread(b, 1, 4, f);
    return read_32_le_buf((unsigned char *)b);
}

void write_32_le_buf(unsigned char b[4], uint32_t v)
{
    for (int i = 0; i < 4; i++)
    {
        b[i] = v & 0xFF;
        v >>= 8;
    }
}

void write_32_le(FILE *f, uint32_t v)
{
    char b[4];
    write_32_le_buf((unsigned char *)b, v);
    fwrite(b, 1, 4, f);
}

uint16_t read_16_le_buf(unsigned char b[2])
{
    uint16_t v = 0;
    for (int i = 1; i >= 0; i--)
    {
        v <<= 8;
        v |= b[i];
    }

    return v;
}

uint16_t read_16_le(FILE *f)
{
    char b[2];
    fread(b, 1, 2, f);
    return read_16_le_buf((unsigned char *)b);
}

void write_16_le_buf(unsigned char b[2], uint16_t v)
{
    for (int i = 0; i < 2; i++)
    {
        b[i] = v & 0xFF;
        v >>= 8;
    }
}

void write_16_le(FILE *f, uint16_t v)
{
    char b[2];
    write_16_le_buf((unsigned char *)b, v);
    fwrite(b, 1, 2, f);
}

uint32_t read_32_be_buf(unsigned char b[4])
{
    uint32_t v = 0;
    for (int i = 0; i < 4; i++)
    {
        v <<= 8;
        v |= b[i];
    }

    return v;
}

uint32_t read_32_be(FILE *f)
{
    char b[4];
    fread(b, 1, 4, f);
    return read_32_be_buf((unsigned char *)b);
}

void write_32_be_buf(unsigned char b[4], uint32_t v)
{
    for (int i = 3; i >= 0; i--)
    {
        b[i] = v & 0xFF;
        v >>= 8;
    }
}

void write_32_be(FILE *f, uint32_t v)
{
    char b[4];
    write_32_be_buf((unsigned char *)b, v);
    fwrite(b, 1, 4, f);
}

uint16_t read_16_be_buf(unsigned char b[2])
{
    uint16_t v = 0;
    for (int i = 0; i < 2; i++)
    {
        v <<= 8;
        v |= b[i];
    }

    return v;
}

uint16_t read_16_be(FILE *f)
{
    char b[2];
    fread(b, 1, 2, f);
    return read_16_be_buf((unsigned char *)b);
}

void write_16_be_buf(unsigned char b[2], uint16_t v)
{
    for (int i = 1; i >= 0; i--)
    {
        b[i] = v & 0xFF;
        v >>= 8;
    }
}

void write_16_be(FILE *f, uint16_t v)
{
    char b[2];
    write_16_be_buf((unsigned char *)b, v);
    fwrite(b, 1, 2, f);
}

void Bit_stream_init(Bit_stream *bstream, char **data_ptr) {
    bstream->data_ptr = data_ptr;
    bstream->bit_buffer = 0;
    bstream->bits_left = 0;
    bstream->total_bits_read = 0;
}

void Bit_oggstream_init(Bit_oggstream *bos, CArray *os)
{
    bos->os = os;
    bos->bit_buffer = 0;
    bos->bits_stored = 0;
    bos->payload_bytes = 0;
    bos->first = true;
    bos->continued = false;
    bos->granule = 0;
    bos->seqno = 0;
}

bool Bit_stream_get_bit(Bit_stream *bstream) {
    if (bstream->bits_left == 0) {

        int c = **bstream->data_ptr;
        (*bstream->data_ptr)++;
        bstream->bit_buffer = (unsigned char)c;
        bstream->bits_left = 8;

    }
    bstream->total_bits_read++;
    bstream->bits_left --;
    return ( ( bstream->bit_buffer & ( 0x80 >> bstream->bits_left ) ) != 0);
}

void Bit_oggstream_put_bit(Bit_oggstream *bos, bool bit) {
    if (bit)
        bos->bit_buffer |= 1<<bos->bits_stored;

    bos->bits_stored ++;
    if (bos->bits_stored == 8) {
        Bit_oggstream_flush_bits(bos);
    }
}

void Bit_oggstream_flush_bits(Bit_oggstream *bos) {
    if (bos->bits_stored != 0) {
        if (bos->payload_bytes == _SEGMENT_SIZE * _MAX_SEGMENTS)
        {
            fprintf(stderr, "Error: ran out of space in an Ogg packet\n");
            Bit_oggstream_flush_page(bos, true, false);
        }

        bos->page_buffer[_HEADER_BYTES + _MAX_SEGMENTS + bos->payload_bytes] = bos->bit_buffer;
        bos->payload_bytes ++;

        bos->bits_stored = 0;
        bos->bit_buffer = 0;
    }
}

void Bit_oggstream_flush_page(Bit_oggstream *bos, bool next_continued, bool last) {
    if (bos->payload_bytes != _SEGMENT_SIZE * _MAX_SEGMENTS)
    {
        Bit_oggstream_flush_bits(bos);
    }

    if (bos->payload_bytes != 0)
    {
        unsigned int segments = (bos->payload_bytes+_SEGMENT_SIZE)/_SEGMENT_SIZE;  // intentionally round up
        if (segments == _MAX_SEGMENTS+1) segments = _MAX_SEGMENTS; // at max eschews the final 0

        // move payload back
        for (unsigned int i = 0; i < bos->payload_bytes; i++)
        {
            bos->page_buffer[_HEADER_BYTES + segments + i] = bos->page_buffer[_HEADER_BYTES + _MAX_SEGMENTS + i];
        }

        bos->page_buffer[0] = 'O';
        bos->page_buffer[1] = 'g';
        bos->page_buffer[2] = 'g';
        bos->page_buffer[3] = 'S';
        bos->page_buffer[4] = 0; // stream_structure_version
        bos->page_buffer[5] = (bos->continued?1:0) | (bos->first?2:0) | (last?4:0); // header_type_flag
        write_32_le_buf(&bos->page_buffer[6], bos->granule);  // granule low bits
        write_32_le_buf(&bos->page_buffer[10], 0);       // granule high bits
        if (bos->granule == UINT32_C(0xFFFFFFFF))
            write_32_le_buf(&bos->page_buffer[10], UINT32_C(0xFFFFFFFF));
        write_32_le_buf(&bos->page_buffer[14], 1);       // stream serial number
        write_32_le_buf(&bos->page_buffer[18], bos->seqno);   // page sequence number
        write_32_le_buf(&bos->page_buffer[22], 0);       // checksum (0 for now)
		bos->page_buffer[26] = (unsigned char)segments; // segment count

        // lacing values
        for (unsigned int i = 0, bytes_left = bos->payload_bytes; i < segments; i++)
        {
            if (bytes_left >= _SEGMENT_SIZE)
            {
                bytes_left -= _SEGMENT_SIZE;
                bos->page_buffer[27 + i] = _SEGMENT_SIZE;
            }
            else
            {
				bos->page_buffer[27 + i] = (unsigned char)bytes_left;
            }
        }

        // checksum
        write_32_le_buf(&bos->page_buffer[22],
                checksum(bos->page_buffer, _HEADER_BYTES + segments + bos->payload_bytes)
                );

        // output to output stream
        for (unsigned int i = 0; i < _HEADER_BYTES + segments + bos->payload_bytes; i++)
        {
            CArrayPushBack(bos->os, &bos->page_buffer[i]);
        }

        bos->seqno++;
        bos->first = false;
        bos->continued = next_continued;
        bos->payload_bytes = 0;
    }
}

void Bit_stream_right_shift_bit_uint(Bit_stream* bstream, Bit_uint* bui)
{
    bui->total = 0;
    for ( unsigned int i = 0; i < bui->bit_size; i++) {
        if ( Bit_stream_get_bit(bstream) ) bui->total |= (1U << i);
    }
}

void Bit_oggstream_left_shift_bit_uint(Bit_oggstream* bstream, const Bit_uint* bui) {
    for ( unsigned int i = 0; i < bui->bit_size; i++) {
        Bit_oggstream_put_bit(bstream, (bui->total & (1U << i)) != 0);
    }
}

void Bit_uintv_init(Bit_uintv* buv, unsigned int s) {
    buv->size = s;
    buv->total = 0;
}

void Bit_uintv_init_with_value(Bit_uintv* buv, unsigned int s, unsigned int v) {
    buv->size = s;
    buv->total = v;
}

void Bit_stream_right_shift_bit_uintv(Bit_stream* bstream, Bit_uintv* bui) {
    bui->total = 0;
    for ( unsigned int i = 0; i < bui->size; i++) {
        if ( Bit_stream_get_bit(bstream) ) bui->total |= (1U << i);
    }
}

void Bit_oggstream_left_shift_bit_uintv(Bit_oggstream* bstream, const Bit_uintv* bui) {
    for ( unsigned int i = 0; i < bui->size; i++) {
        Bit_oggstream_put_bit(bstream, (bui->total & (1U << i)) != 0);
    }
}