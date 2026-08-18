#include "kraken.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	// Test file    | Uncompressed size
	// gamemaps.wl6 | 213502
	// vgagraph.wl6 | 280974
	// vswap.wl6    | 1378242
	(void)argc;
	(void)argv;
	FILE *f = fopen("gamemaps.wl6", "rb");
	fseek(f, 0, SEEK_END);
	size_t src_len = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t *src = malloc(src_len);
	fread(src, 1, src_len, f);
	fclose(f);
	size_t dst_len = 213502;
	uint8_t *dst = malloc(dst_len);
	int res = Kraken_Decompress(src, src_len, dst, dst_len);
	if (res < 0)
	{
		printf("Decompression failed with error code %d\n", res);
		goto bail;
	}
	if (res != dst_len)
	{
		printf(
			"Decompressed size mismatch: expected %zu, got %d\n", dst_len,
			res);
		goto bail;
	}
	printf("Decompression successful, output size: %d bytes\n", res);
	FILE *out = fopen("gamemaps_dec.wl6", "wb");
	fwrite(dst, 1, dst_len, out);
	fclose(out);

bail:
	free(src);
	free(dst);
	return 0;
}
