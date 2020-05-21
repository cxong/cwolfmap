#include "vswap.h"

#include <stdio.h>
#include <stdlib.h>

// http://gaarabis.free.fr/_sites/specs/wlspec_index.html

#define PATH_MAX 4096

int CWVSwapLoad(CWVSwap *vswap, const char *path)
{
	int err = 0;
	FILE *f = fopen(path, "rb");
	if (!f)
	{
		err = -1;
		fprintf(stderr, "Failed to read %s", path);
		goto bail;
	}
	fseek(f, 0, SEEK_END);
	const long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (fread(&vswap->head, 1, sizeof vswap->head, f) != sizeof vswap->head)
	{
		err = -1;
		fprintf(stderr, "Failed to read vswap head");
		goto bail;
	}
	vswap->chunkOffset = malloc(sizeof(uint32_t) * vswap->head.chunkCount);
	if (fread(
			vswap->chunkOffset, sizeof(uint32_t), vswap->head.chunkCount, f) !=
		vswap->head.chunkCount)
	{
		err = -1;
		fprintf(stderr, "Failed to read chunk offsets");
		goto bail;
	}
	vswap->chunkLength = malloc(sizeof(uint16_t) * vswap->head.chunkCount);
	if (fread(
			vswap->chunkLength, sizeof(uint16_t), vswap->head.chunkCount, f) !=
		vswap->head.chunkCount)
	{
		err = -1;
		fprintf(stderr, "Failed to read chunk lengths");
		goto bail;
	}

bail:
	if (f)
	{
		fclose(f);
	}
	return err;
}

void CWVSwapFree(CWVSwap *vswap)
{
	free(vswap->chunkOffset);
	free(vswap->chunkLength);
}
