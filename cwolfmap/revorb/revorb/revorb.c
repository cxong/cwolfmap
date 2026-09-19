/*
 * REVORB - Recomputes page granule positions in Ogg Vorbis files.
 *   version 0.2 (2008/06/29)
 *
 * Copyright (c) 2008, Jiri Hruska <jiri.hruska@fud.cz>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*# INCLUDE=.\include #*/
/*# LIB=.\lib         #*/
/*# CFLAGS=/D_UNICODE #*/
/*# LFLAGS=/NODEFAULTLIB:MSVCRT /LTCG /OPT:REF /MANIFEST:NO #*/

#include "revorb.h"

#include <ogg/ogg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vorbis/codec.h>

static bool append_output(char **out, size_t *out_size, const void *data,
                          size_t data_size) {
  size_t new_size = *out_size + data_size;
  char *new_out = realloc(*out, new_size);
  if (new_size != 0 && new_out == NULL)
    return false;

  memcpy(new_out + *out_size, data, data_size);
  *out = new_out;
  *out_size = new_size;
  return true;
}

bool copy_headers(const char *in, size_t in_size, ogg_sync_state *si,
                  ogg_stream_state *is, char **out, size_t *out_size,
                  ogg_sync_state *so, ogg_stream_state *os, vorbis_info *vi,
                  size_t *in_pos) {
	(void)so;
  char *buffer = ogg_sync_buffer(si, 4096);
  size_t remaining = in_size - *in_pos;
  size_t numread = remaining < 4096 ? remaining : 4096;
  memcpy(buffer, in + *in_pos, numread);
  ogg_sync_wrote(si, (long)numread);
  *in_pos += numread;

  ogg_page page;
  if (ogg_sync_pageout(si, &page) != 1) {
    fprintf(stderr, "Input is not an Ogg.\n");
    return false;
  }

  ogg_stream_init(is, ogg_page_serialno(&page));
  ogg_stream_init(os, ogg_page_serialno(&page));

  if (ogg_stream_pagein(is, &page) < 0) {
    fprintf(stderr, "Error in the first page.\n");
    ogg_stream_clear(is);
    ogg_stream_clear(os);
    return false;
  }

  ogg_packet packet;
  if (ogg_stream_packetout(is, &packet) != 1) {
    fprintf(stderr, "Error in the first packet.\n");
    ogg_stream_clear(is);
    ogg_stream_clear(os);
    return false;
  }

  vorbis_comment vc;
  vorbis_comment_init(&vc);
  if (vorbis_synthesis_headerin(vi, &vc, &packet) < 0) {
    fprintf(stderr, "Error in header, probably not a Vorbis file.\n");
    vorbis_comment_clear(&vc);
    ogg_stream_clear(is);
    ogg_stream_clear(os);
    return false;
  }

  ogg_stream_packetin(os, &packet);

  int i = 0;
  while (i < 2) {
    int res = ogg_sync_pageout(si, &page);

    if (res == 0) {
      buffer = ogg_sync_buffer(si, 4096);
      remaining = in_size - *in_pos;
      numread = remaining < 4096 ? remaining : 4096;
      if (numread == 0) {
        fprintf(stderr, "Headers are damaged, file is probably truncated.\n");
        ogg_stream_clear(is);
        ogg_stream_clear(os);
        return false;
      }
      memcpy(buffer, in + *in_pos, numread);
      ogg_sync_wrote(si, (long)numread);
      *in_pos += numread;
      continue;
    }

    if (res == 1) {
      ogg_stream_pagein(is, &page);
      while (i < 2) {
        res = ogg_stream_packetout(is, &packet);
        if (res == 0)
          break;
        if (res < 0) {
          fprintf(stderr, "Secondary header is corrupted.\n");
          vorbis_comment_clear(&vc);
          ogg_stream_clear(is);
          ogg_stream_clear(os);
          return false;
        }
        vorbis_synthesis_headerin(vi, &vc, &packet);
        ogg_stream_packetin(os, &packet);
        i++;
      }
    }
  }

  vorbis_comment_clear(&vc);

  while (ogg_stream_flush(os, &page)) {
    if (!append_output(out, out_size, page.header, page.header_len) ||
        !append_output(out, out_size, page.body, page.body_len)) {
      fprintf(stderr, "Cannot write headers to output.\n");
      ogg_stream_clear(is);
      ogg_stream_clear(os);
      return false;
    }
  }

  return true;
}

bool revorb(const char *in, const size_t in_size, char **out,
            size_t *out_size) {
  bool g_failed = false;
  size_t in_pos = 0;

  *out = NULL;
  *out_size = 0;

  ogg_sync_state sync_in, sync_out;
  ogg_sync_init(&sync_in);
  ogg_sync_init(&sync_out);

  ogg_stream_state stream_in, stream_out;
  vorbis_info vi;
  vorbis_info_init(&vi);

  ogg_packet packet;
  ogg_page page;

  if (copy_headers(in, in_size, &sync_in, &stream_in, out, out_size, &sync_out,
                   &stream_out, &vi, &in_pos)) {
    ogg_int64_t granpos = 0, packetnum = 0;
    int lastbs = 0;

    while (1) {
      //    ogg_int64_t logstream_startgran = granpos;

      int eos = 0;
      while (!eos) {
        int res = ogg_sync_pageout(&sync_in, &page);
        if (res == 0) {
          char *buffer = ogg_sync_buffer(&sync_in, 4096);
          size_t remaining = in_size - in_pos;
          size_t numread = remaining < 4096 ? remaining : 4096;
          if (numread == 0) {
            eos = 2;
          } else {
            memcpy(buffer, in + in_pos, numread);
            ogg_sync_wrote(&sync_in, (long)numread);
            in_pos += numread;
          }
          continue;
        }

        if (res < 0) {
          fprintf(stderr, "Warning: Corrupted or missing data in bitstream.\n");
          g_failed = true;
        } else {
          if (ogg_page_eos(&page))
            eos = 1;
          ogg_stream_pagein(&stream_in, &page);

          while (1) {
            res = ogg_stream_packetout(&stream_in, &packet);
            if (res == 0)
              break;
            if (res < 0) {
              fprintf(stderr, "Warning: Bitstream error.\n");
              g_failed = true;
              continue;
            }

            /*
            if (packet.granulepos >= 0) {
              granpos = packet.granulepos + logstream_startgran;
              packet.granulepos = granpos;
            }
            */
            int bs = vorbis_packet_blocksize(&vi, &packet);
            if (lastbs)
              granpos += (lastbs + bs) / 4;
            lastbs = bs;

            packet.granulepos = granpos;
            packet.packetno = packetnum++;
            if (!packet.e_o_s) {
              ogg_stream_packetin(&stream_out, &packet);

              ogg_page opage;
              while (ogg_stream_pageout(&stream_out, &opage)) {
                if (!append_output(out, out_size, opage.header,
                                   opage.header_len) ||
                    !append_output(out, out_size, opage.body, opage.body_len)) {
                  fprintf(stderr, "Unable to write page to output.\n");
                  eos = 2;
                  g_failed = true;
                  break;
                }
              }
            }
          }
        }
      }

      if (eos == 2)
        break;

      {
        packet.e_o_s = 1;
        ogg_stream_packetin(&stream_out, &packet);
        ogg_page opage;
        while (ogg_stream_flush(&stream_out, &opage)) {
          if (!append_output(out, out_size, opage.header, opage.header_len) ||
              !append_output(out, out_size, opage.body, opage.body_len)) {
            fprintf(stderr, "Unable to write page to output.\n");
            g_failed = true;
            break;
          }
        }
        ogg_stream_clear(&stream_in);
        break;
      }
    }

    ogg_stream_clear(&stream_out);
  } else {
    g_failed = true;
  }

  vorbis_info_clear(&vi);

  ogg_sync_clear(&sync_in);
  ogg_sync_clear(&sync_out);
  return !g_failed;
}
