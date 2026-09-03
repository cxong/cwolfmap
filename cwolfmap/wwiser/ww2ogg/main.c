#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "packed_codebooks_aoTuV_603.h"
#include "wwriff.h"

// Based on ww2ogg https://github.com/hcs64/ww2ogg

int main(int argc, char* argv[])
{
    // Test files: *.wem
    int err = 0;
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <file.wem>\n", argv[0]);
        err = 1;
        goto bail;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f)
    {
        fprintf(stderr, "Failed to open file %s\n", argv[1]);
        err = 1;
        goto bail;
    }
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *data = (char*)malloc(length);
    if (!data)    {
        fprintf(stderr, "Failed to allocate memory\n");
        err = 1;
        goto bail;
    }
    if (fread(data, 1, length, f) != (size_t)length)
    {
        fprintf(stderr, "Failed to read file %s\n", argv[1]);
        err = 1;
        goto bail;
    }
    fclose(f);
    Wwise_RIFF_Vorbis decoder;
    err = Wwise_RIFF_Vorbis_init(&decoder, data, length, packed_codebooks_aoTuV_603, sizeof(packed_codebooks_aoTuV_603), false, false, kNoForcePacketFormat);
    if (err != 0) {
        fprintf(stderr, "Failed to read headers: %d\n", err);
        err = 1;
        goto bail;
    }

    long stream_length;
    char *generated_stream = Wwise_RIFF_Vorbis_generate_ogg(&decoder, &stream_length);
    if (!generated_stream)
    {
        fprintf(stderr, "Failed to generate Ogg stream\n");
        err = 1;
        goto bail;
    }
    printf("Generated Ogg stream of length %ld\n", stream_length);

    // TODO: revorb the generated stream to fix granule positions
    // Just write out the stream for now
    FILE *out = fopen("output.ogg", "wb");
    if (!out)    {
        fprintf(stderr, "Failed to open output file\n");
        err = 1;
        goto bail;
    }
    if (fwrite(generated_stream, 1, stream_length, out) != (size_t)stream_length)
    {
        fprintf(stderr, "Failed to write output file\n");
        err = 1;
        goto bail;
    }
    fclose(out);
    // std::ostringstream ostream;
    // if(!revorb(generatedStream, ostream))
    //     throw CRecoverableError("Failed to rebuild granules");

    // std::vector<uint8_t> out;
    // out.resize(ostream.str().length());
    // memcpy(out.data(), ostream.str().data(), ostream.str().length());
    // return out;

bail:
    return err;
}
