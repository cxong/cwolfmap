#pragma once

#include <stdint.h>
#include <stdio.h>

#include "c_array.h"
#include "errors.h"
#include "crc.h"

// host-endian-neutral integer reading
uint32_t read_32_le_buf(unsigned char b[4]);
uint32_t read_32_le(FILE *f);
void write_32_le_buf(unsigned char b[4], uint32_t v);
void write_32_le(FILE *f, uint32_t v);
uint16_t read_16_le_buf(unsigned char b[2]);
uint16_t read_16_le(FILE *f);
void write_16_le_buf(unsigned char b[2], uint16_t v);
void write_16_le(FILE *f, uint16_t v);
uint32_t read_32_be_buf(unsigned char b[4]);
uint32_t read_32_be(FILE *f);
void write_32_be_buf(unsigned char b[4], uint32_t v);
void write_32_be(FILE *f, uint32_t v);
uint16_t read_16_be_buf(unsigned char b[2]);
uint16_t read_16_be(FILE *f);
void write_16_be_buf(unsigned char b[2], uint16_t v);
void write_16_be(FILE *f, uint16_t v);

// using an istream, pull off individual bits with get_bit (LSB first)
typedef struct {
    char **data_ptr;

    unsigned char bit_buffer;
    unsigned int bits_left;
    unsigned long total_bits_read;
} Bit_stream;

void Bit_stream_init(Bit_stream *bstream, char **data_ptr);

bool Bit_stream_get_bit(Bit_stream *bstream);

typedef struct {
    CArray *os;

    unsigned char bit_buffer;
    unsigned int bits_stored;

#define _HEADER_BYTES 27
#define _MAX_SEGMENTS 255
#define _SEGMENT_SIZE 255

    unsigned int payload_bytes;
    bool first, continued;
    unsigned char page_buffer[_HEADER_BYTES + _MAX_SEGMENTS + _SEGMENT_SIZE * _MAX_SEGMENTS];
    uint32_t granule;
    uint32_t seqno;
} Bit_oggstream;

void Bit_oggstream_init(Bit_oggstream *bos, CArray *os);

void Bit_oggstream_put_bit(Bit_oggstream *bos, bool bit);

void Bit_oggstream_flush_bits(Bit_oggstream *bos);

// Must flush at end
void Bit_oggstream_flush_page(Bit_oggstream *bos, bool next_continued, bool last);

// integer of a certain number of bits, to allow reading just that many
// bits from the Bit_stream
typedef struct{
    unsigned int total;
    unsigned int bit_size;
} Bit_uint;

void Bit_stream_right_shift_bit_uint(Bit_stream* bstream, Bit_uint* bui);

void Bit_oggstream_left_shift_bit_uint(Bit_oggstream* bstream, const Bit_uint* bui);

// integer of a run-time specified number of bits
// bits from the Bit_stream
typedef struct {
    unsigned int size;
    unsigned int total;
} Bit_uintv;


void Bit_uintv_init(Bit_uintv* buv, unsigned int s);

void Bit_uintv_init_with_value(Bit_uintv* buv, unsigned int s, unsigned int v);

void Bit_stream_right_shift_bit_uintv(Bit_stream* bstream, Bit_uintv* bui);

void Bit_oggstream_left_shift_bit_uintv(Bit_oggstream* bstream, const Bit_uintv* bui);
