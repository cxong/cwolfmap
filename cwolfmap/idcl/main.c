#include <stdio.h>
#include <stdlib.h>

#include "idcl.h"
#include <kraken.h>

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Usage: %s <path_to_wolf2>\n", argv[0]);
		return 1; // Not enough arguments
	}

	char path[4096];
	FileLump *lumps;
	int numLumps;

	// chunk_4.resources contains .wl6 files
	snprintf(path, sizeof(path), "%s/base/chunk_4.resources", argv[1]);
	if (LoadWolf2Lumps(path, &lumps, &numLumps) != 0)
	{
		return 1; // Error loading lumps
	}
	printf("Found %d lumps in chunk_4.resources\n", numLumps);
	// Write out lumps to file
	for (int i = 0; i < numLumps; ++i)
	{
		const FileLump *lump = &lumps[i];
		if (lump->compressedSize != lump->size)
		{
			printf("Lump %s is compressed!\n", lump->name);
			// Decompress
			uint8_t *dst = malloc(lump->size);
			int res = Kraken_Decompress(
				lump->data, lump->compressedSize, dst, lump->size);
			if (res < 0)
			{
				printf("Decompression failed with error code %d\n", res);
				return 1;
			}
			if (res != lump->size)
			{
				printf(
					"Decompressed size mismatch: expected %llu, got %d\n",
					lump->size, res);
				return 1;
			}
			printf("Decompression successful, output size: %d bytes\n", res);
			FILE *f = fopen(lump->name, "wb");
			fwrite(dst, 1, lump->size, f);
			fclose(f);
			free(dst);
			printf(
				"Wrote out compressed lump %s, uncompressed size: %llu\n",
				lump->name, lump->size);
		}
		else
		{
			FILE *f = fopen(lump->name, "wb");
			fwrite(lump->data, 1, lump->size, f);
			fclose(f);
			printf("Wrote out lump %s\n", lump->name);
		}
	}
	// Free allocated memory for both sets of lumps
	for (int i = 0; i < numLumps; ++i)
	{
		free(lumps[i].data);
	}
	free(lumps);
	numLumps = 0;

	// sound/soundbanks/pc/sound.pack contains sb_wolfstone.bnk
	snprintf(
		path, sizeof(path), "%s/base/sound/soundbanks/pc/sound.pack", argv[1]);
	if (LoadWolf2Lumps(path, &lumps, &numLumps) != 0)
	{
		return 1; // Error loading lumps
	}
	printf("Found %d lumps in sound.pack\n", numLumps);
	for (int i = 0; i < numLumps; ++i)
	{
		const FileLump *lump = &lumps[i];
		FILE *f = fopen(lump->name, "wb");
		fwrite(lump->data, 1, lump->size, f);
		fclose(f);
		printf("Wrote out lump %s\n", lump->name);
		free(lump->data);
	}
	free(lumps);
	numLumps = 0;

	// patch_1_english(us).pack contains sb_vo_wolfstone.bnk (need to read
	// english(us).pack and patch_\d_english(us).pack)
	for (int i = 0;; i++)
	{
		if (i == 0)
		{
			snprintf(
				path, sizeof(path),
				"%s/base/sound/soundbanks/pc/english(us).pack", argv[1]);
		}
		else
		{

			snprintf(
				path, sizeof(path),
				"%s/base/sound/soundbanks/pc/patch_%d_english(us).pack",
				argv[1], i);
		}
		if (LoadWolf2Lumps(path, &lumps, &numLumps) != 0)
		{
			break;
		}
		printf("Found %d lumps in %s\n", numLumps, path);
		for (int j = 0; j < numLumps; ++j)
		{
			const FileLump *lump = &lumps[j];
			FILE *f = fopen(lump->name, "wb");
			fwrite(lump->data, 1, lump->size, f);
			fclose(f);
			printf("Wrote out lump %s\n", lump->name);
			free(lump->data);
		}
		free(lumps);
		numLumps = 0;
	}

	// gameresources.resources contains strings

	return 0;
}