#include "wolf2.h"

#include <stdio.h>
#include <stdlib.h>

#include "idcl/idcl.h"
#include "idcl/kraken/kraken.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

int CWWolf2LoadResources(
	const char *path, Resource *mapHead, Resource *mapData, Resource *vswap)
{
	int err = 0;
	char pathBuf[PATH_MAX];
	FileLump *lumps;
	int numLumps;

	// chunk_4.resources contains .wl6 files
	snprintf(pathBuf, sizeof(pathBuf), "%s/base/chunk_4.resources", path);
	if (LoadWolf2Lumps(pathBuf, &lumps, &numLumps) != 0)
	{
		fprintf(stderr, "Error loading wl6 lumps %s\n", pathBuf);
		err = -1;
		goto bail;
	}
	for (int i = 0; i < numLumps; ++i)
	{
		const FileLump *lump = &lumps[i];
		Resource *resource = NULL;
		bool freeLump = true;
		if (strcmp(lump->name, "gamemaps.wl6") == 0)
		{
			resource = mapData;
		}
		else if (strcmp(lump->name, "maphead.wl6") == 0)
		{
			resource = mapHead;
		}
		else if (strcmp(lump->name, "vgadict.wl6") == 0)
		{
			// Ignore
		}
		else if (strcmp(lump->name, "vgagraph.wl6") == 0)
		{
			// Ignore
		}
		else if (strcmp(lump->name, "vgahead.wl6") == 0)
		{
			// Ignore
		}
		else if (strcmp(lump->name, "vswap.wl6") == 0)
		{
			resource = vswap;
		}
		if (resource)
		{
			if (lump->compressedSize != lump->size)
			{
				// Decompress
				resource->data = malloc(lump->size);
				int res = Kraken_Decompress(
					lump->data, lump->compressedSize, resource->data,
					lump->size);
				if (res < 0)
				{
					fprintf(
						stderr, "Decompression failed with error code %d\n",
						res);
					err = -1;
					goto bail;
				}
				if (res != (int)lump->size)
				{
					fprintf(
						stderr,
						"Decompressed size mismatch: expected %llu, got "
						"%d\n",
						lump->size, res);
					err = -1;
					goto bail;
				}
			}
			else
			{
				resource->data = lump->data;
				freeLump = false;
			}
		}
		if (freeLump)
		{
			free(lump->data);
		}
	}
	free(lumps);
	numLumps = 0;

bail:
	return err;
}