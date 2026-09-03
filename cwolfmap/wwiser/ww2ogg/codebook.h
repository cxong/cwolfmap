#ifndef _CODEBOOK_H
#define _CODEBOOK_H

#include <stdint.h>
#include <stdlib.h>
#include "errors.h"
#include "Bit_stream.h"

/* stuff from Tremor (lowmem) */
int ilog(unsigned int v);

unsigned int _book_maptype1_quantvals(unsigned int entries, unsigned int dimensions);

typedef struct
{
    char * codebook_data;
    long * codebook_offsets;
    long codebook_count;
} codebook_library;

void codebook_library_init_from_data(codebook_library *cb_lib, const uint8_t* data, unsigned long length);
void codebook_library_init(codebook_library *cb_lib);

void codebook_library_destroy(codebook_library* cb_lib);

const char* codebook_library_get_codebook(codebook_library* cb_lib, int i);

long codebook_library_get_codebook_size(codebook_library* cb_lib, int i);

void codebook_library_rebuild_ogg(codebook_library *cb_lib, int i, Bit_oggstream* bos);

void codebook_library_rebuild(codebook_library *cb_lib, Bit_stream *bis, unsigned long cb_size, Bit_oggstream* bos);

void codebook_library_copy(codebook_library *cb_lib, Bit_stream *bis, Bit_oggstream *bos);

#endif
