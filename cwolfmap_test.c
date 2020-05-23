#include "cwolfmap/cwolfmap.h"
#include <stdio.h>
#include <Windows.h>

static void PrintCh(const CWLevel *level, const int plane, const int x, const int y)
{
	if (plane == 0)
	{
		const CWTile tile = CWLevelGetTile(level, x, y);
		switch (tile)
		{
		case CWTILE_WALL:
			printf("#");
			break;
		case CWTILE_DOOR_H:
			printf("-");
			break;
		case CWTILE_DOOR_V:
			printf("|");
			break;
		case CWTILE_DOOR_GOLD_H:
			printf("-");
			break;
		case CWTILE_DOOR_GOLD_V:
			printf("|");
			break;
		case CWTILE_DOOR_SILVER_H:
			printf("-");
			break;
		case CWTILE_DOOR_SILVER_V:
			printf("|");
			break;
		case CWTILE_ELEVATOR_H:
			printf("-");
			break;
		case CWTILE_ELEVATOR_V:
			printf("|");
			break;
		case CWTILE_AREA:
			printf(" ");
			break;
		default:
			printf("?");
			break;
		}
	}
	else
	{
		const CWEntity entity = CWLevelGetEntity(level, x, y);
		//printf("%c", (unsigned char)(entity % (255 - 31)) + 31);
		printf(" ");
	}
}

int main(int argc, char *argv[])
{
	SetConsoleOutputCP(CP_UTF8);
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
		// Only care about the first two planes
		for (int j = 0; j < 2; j++, plane++)
		{
			printf("- Plane %d\n", j);
			for (int x = 0; x < level->header.width; x++)
			{
				for (int y = 0; y < level->header.height; y++)
				{
					PrintCh(level, j, x, y);
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
