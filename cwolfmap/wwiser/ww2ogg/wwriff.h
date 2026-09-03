#ifndef _WWRIFF_H
#define _WWRIFF_H

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif
#include <stdbool.h>
#include <stdint.h>
#include "Bit_stream.h"
#include "errors.h"

#define VERSION "0.24"

typedef enum {
    kNoForcePacketFormat,
    kForceModPackets,
    kForceNoModPackets
}ForcePacketFormat;

typedef struct
{
    const uint8_t* _codebooks_data;
    unsigned long _codebooks_length;
    char *_data;
    char *_data_ptr;
    long _length;

    bool _little_endian;

    long _riff_size;
    long _fmt_offset, _cue_offset, _LIST_offset, _smpl_offset, _vorb_offset, _data_offset;
    long _fmt_size, _cue_size, _LIST_size, _smpl_size, _vorb_size, _data_size;

    // RIFF fmt
    uint16_t _channels;
    uint32_t _sample_rate;
    uint32_t _avg_bytes_per_second;

    // RIFF extended fmt
    uint16_t _ext_unk;
    uint32_t _subtype;

    // cue info
    uint32_t _cue_count;

    // smpl info
    uint32_t _loop_count, _loop_start, _loop_end;

    // vorbis info
    uint32_t _sample_count;
    uint32_t _setup_packet_offset;
    uint32_t _first_audio_packet_offset;
    uint32_t _uid;
    uint8_t _blocksize_0_pow;
    uint8_t _blocksize_1_pow;

    bool _inline_codebooks, _full_setup;
    bool _header_triad_present, _old_packet_headers;
    bool _no_granule, _mod_packets;

    uint16_t (*_read_16)(unsigned char *);
    uint32_t (*_read_32)(unsigned char*);
} Wwise_RIFF_Vorbis;

int Wwise_RIFF_Vorbis_init(
    Wwise_RIFF_Vorbis *decoder,
    char *data,
    long length,
    const uint8_t* codebooks_data,
    unsigned long codebooks_length,
    bool inline_codebooks,
    bool full_setup,
    ForcePacketFormat force_packet_format
    );

char *Wwise_RIFF_Vorbis_generate_ogg(Wwise_RIFF_Vorbis *decoder, long *out_length);

#endif
