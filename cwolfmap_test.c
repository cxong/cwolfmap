#include "cwolfmap/cwolfmap.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
	CWolfMap map;
	int err = 0;
	err = CWLoad(&map, "WOLF3D");
	if (err != 0)
	{
		goto bail;
	}
	printf("Loaded %d levels\n", map.nLevels);
	CWLevel *level = map.levels;
	for (int i = 0; i < map.nLevels; i++, level++)
	{
		printf(
			"Level %d: %s (%dx%d)\n", i + 1, level->header.name,
			level->header.width, level->header.height);
		const CWPlane *plane = level->planes;
		for (int j = 0; j < NUM_PLANES; j++, plane++)
		{
			printf("- Plane %d\n", j);
			for (int x = 0; x < level->header.width; x++)
			{
				for (int y = 0; y < level->header.height; y++)
				{
					const uint16_t ch = CWLevelGetTile(level, j, x, y);
					// Avoid non-printable characters
					printf("%c", (unsigned char)(ch % (255 - 31)) + 31);
				}
				printf("\n");
			}
		}
	}

	printf("Loaded %d sounds\n", map.audio.head.nOffsets);

	printf("Loaded %d VSWAP chunks\n", map.vswap.head.chunkCount);
	printf("- sprite: %d\n", map.vswap.head.firstSprite);
	printf("- sound: %d\n", map.vswap.head.firstSound);

bail:
	CWFree(&map);
	return err;
}
