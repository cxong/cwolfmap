#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "revorb.h"

int main(int argc, char *argv[]) {
  // Test files: *.ogg
  int err = 0;
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <file.ogg>\n", argv[0]);
    err = 1;
    goto bail;
  }
  FILE *f = fopen(argv[1], "rb");
  if (!f) {
    fprintf(stderr, "Failed to open file %s\n", argv[1]);
    err = 1;
    goto bail;
  }
  char *data = NULL;
  size_t size = 0;
  fseek(f, 0, SEEK_END);
  size = ftell(f);
  fseek(f, 0, SEEK_SET);
  data = malloc(size);
  if (!data) {
    fprintf(stderr, "Failed to allocate memory for file %s\n", argv[1]);
    err = 1;
    goto bail;
  }
  if (fread(data, 1, size, f) != size) {
    fprintf(stderr, "Failed to read file %s\n", argv[1]);
    err = 1;
    goto bail;
  }
  fclose(f);

  char *data_out = NULL;
  size_t size_out = 0;
  if (!revorb(data, size, &data_out, &size_out)) {
    fprintf(stderr, "Failed to revorb file %s\n", argv[1]);
    err = 1;
    goto bail;
  }
  // Write output file
  char out_filename[1024];
  snprintf(out_filename, sizeof(out_filename), "%s.revorb.ogg", argv[1]);
  FILE *f_out = fopen(out_filename, "wb");
  if (!f_out) {
    fprintf(stderr, "Failed to open output file %s\n", out_filename);
    err = 1;
    goto bail;
  }
  if (fwrite(data_out, 1, size_out, f_out) != size_out) {
    fprintf(stderr, "Failed to write output file %s\n", out_filename);
    err = 1;
    goto bail;
  }
  fclose(f_out);

bail:
  return err;
}
