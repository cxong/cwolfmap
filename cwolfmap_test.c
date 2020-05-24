#include "cwolfmap/cwolfmap.h"
#include <stdio.h>
#include "rlutil.h"

static void PrintCh(const CWLevel *level, const int plane, const int x, const int y)
{
	const uint16_t ch = CWLevelGetCh(level, plane, x, y);
	if (plane == 0)
	{
		const CWTile tile = CWChToTile(ch);
		const CWWall wall = CWChToWall(ch);
		switch (tile)
		{
		case CWTILE_WALL:
			switch (wall)
			{
			case CWWALL_GREY_BRICK_1:
				setBackgroundColor(GREY);
				printf(" ");
				break;
			case CWWALL_GREY_BRICK_2:
				setBackgroundColor(GREY);
				setColor(BLACK);
				printf("#");
				break;
			case CWWALL_GREY_BRICK_FLAG:
				setBackgroundColor(GREY);
				setColor(RED);
				printf("=");
				break;
			case CWWALL_GREY_BRICK_HITLER:
				setBackgroundColor(GREY);
				setColor(YELLOW);
				printf("=");
				break;
			case CWWALL_CELL:
				setBackgroundColor(LIGHTBLUE);
				setColor(BLACK);
				printf("=");
				break;
			case CWWALL_GREY_BRICK_EAGLE:
				setBackgroundColor(GREY);
				setColor(LIGHTCYAN);
				printf("=");
				break;
			case CWWALL_CELL_SKELETON:
				setBackgroundColor(LIGHTBLUE);
				setColor(WHITE);
				printf("=");
				break;
			case CWWALL_BLUE_BRICK_1:
				setBackgroundColor(LIGHTBLUE);
				printf(" ");
				break;
			case CWWALL_BLUE_BRICK_2:
				setBackgroundColor(LIGHTBLUE);
				setColor(BLACK);
				printf("#");
				break;
			case CWWALL_WOOD_EAGLE:
				setBackgroundColor(RED);
				setColor(LIGHTCYAN);
				printf("=");
				break;
			case CWWALL_WOOD_HITLER:
				setBackgroundColor(RED);
				setColor(YELLOW);
				printf("=");
				break;
			case CWWALL_WOOD:
				setBackgroundColor(RED);
				printf(" ");
				break;
			case CWWALL_ENTRANCE:
				setBackgroundColor(LIGHTGREEN);
				printf(" ");
				break;
			case CWWALL_STEEL:
				setBackgroundColor(LIGHTCYAN);
				printf(" ");
				break;
			case CWWALL_LANDSCAPE:
				setBackgroundColor(LIGHTCYAN);
				setColor(LIGHTGREEN);
				printf("_");
				break;
			case CWWALL_RED_BRICK:
				setBackgroundColor(LIGHTRED);
				printf(" ");
				break;
			case CWWALL_RED_BRICK_SWASTIKA:
				setBackgroundColor(LIGHTRED);
				setColor(YELLOW);
				printf("=");
				break;
			case CWWALL_PURPLE:
				setBackgroundColor(LIGHTMAGENTA);
				printf(" ");
				break;
			case CWWALL_RED_BRICK_FLAG:
				setBackgroundColor(LIGHTRED);
				setColor(LIGHTCYAN);
				printf("=");
				break;
			case CWWALL_ELEVATOR:
				setBackgroundColor(YELLOW);
				printf(" ");
				break;
			case CWWALL_PURPLE_BLOOD:
				setBackgroundColor(LIGHTMAGENTA);
				setColor(LIGHTRED);
				printf("#");
				break;
			default:
				printf("#");
				break;
			}
			setBackgroundColor(BLACK);
			setColor(GREY);
			break;
		case CWTILE_DOOR_H:
			setColor(GREY);
			printf("-");
			setColor(GREY);
			break;
		case CWTILE_DOOR_V:
			setColor(GREY);
			printf("|");
			setColor(GREY);
			break;
		case CWTILE_DOOR_GOLD_H:
			setColor(YELLOW);
			printf("-");
			setColor(GREY);
			break;
		case CWTILE_DOOR_GOLD_V:
			setColor(YELLOW);
			printf("|");
			setColor(GREY);
			break;
		case CWTILE_DOOR_SILVER_H:
			setColor(CYAN);
			printf("-");
			setColor(GREY);
			break;
		case CWTILE_DOOR_SILVER_V:
			setColor(CYAN);
			printf("|");
			setColor(GREY);
			break;
		case CWTILE_ELEVATOR_H:
			setColor(BROWN);
			printf("-");
			setColor(GREY);
			break;
		case CWTILE_ELEVATOR_V:
			setColor(BROWN);
			printf("|");
			setColor(GREY);
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
		for (int j = 0; j < 1; j++, plane++)
		{
			printf("- Plane %d\n", j);
			for (int x = 0; x < level->header.width; x++)
			{
				for (int y = 0; y < level->header.height; y++)
				{
					PrintCh(level, j, x, y);
				}
				printf(" \n");
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
