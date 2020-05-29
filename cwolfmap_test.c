#include "cwolfmap/cwolfmap.h"
#include <stdio.h>
#include "rlutil.h"

static void PrintCh(const CWLevel *level, const int x, const int y)
{
	uint16_t ch = CWLevelGetCh(level, 0, x, y);
	// Structural
	const CWTile tile = CWChToTile(ch);
	const CWWall wall = CWChToWall(ch);
	char c = ' ';
	switch (tile)
	{
	case CWTILE_WALL:
		switch (wall)
		{
		case CWWALL_GREY_BRICK_1:
			setBackgroundColor(GREY);
			break;
		case CWWALL_GREY_BRICK_2:
			setBackgroundColor(GREY);
			setColor(BLACK);
			c = '#';
			break;
		case CWWALL_GREY_BRICK_FLAG:
			setBackgroundColor(GREY);
			setColor(RED);
			c = '=';
			break;
		case CWWALL_GREY_BRICK_HITLER:
			setBackgroundColor(GREY);
			setColor(YELLOW);
			c = '=';
			break;
		case CWWALL_CELL:
			setBackgroundColor(LIGHTBLUE);
			setColor(BLACK);
			c = '=';
			break;
		case CWWALL_GREY_BRICK_EAGLE:
			setBackgroundColor(GREY);
			setColor(LIGHTCYAN);
			c = '=';
			break;
		case CWWALL_CELL_SKELETON:
			setBackgroundColor(LIGHTBLUE);
			setColor(WHITE);
			c = '=';
			break;
		case CWWALL_BLUE_BRICK_1:
			setBackgroundColor(LIGHTBLUE);
			break;
		case CWWALL_BLUE_BRICK_2:
			setBackgroundColor(LIGHTBLUE);
			setColor(BLACK);
			c = '#';
			break;
		case CWWALL_WOOD_EAGLE:
			setBackgroundColor(RED);
			setColor(LIGHTCYAN);
			c = '=';
			break;
		case CWWALL_WOOD_HITLER:
			setBackgroundColor(RED);
			setColor(YELLOW);
			c = '=';
			break;
		case CWWALL_WOOD:
			setBackgroundColor(RED);
			break;
		case CWWALL_ENTRANCE:
			setBackgroundColor(LIGHTGREEN);
			break;
		case CWWALL_STEEL:
			setBackgroundColor(LIGHTCYAN);
			break;
		case CWWALL_LANDSCAPE:
			setBackgroundColor(LIGHTCYAN);
			setColor(LIGHTGREEN);
			c = '_';
			break;
		case CWWALL_RED_BRICK:
			setBackgroundColor(LIGHTRED);
			break;
		case CWWALL_RED_BRICK_SWASTIKA:
			setBackgroundColor(LIGHTRED);
			setColor(YELLOW);
			c = '=';
			break;
		case CWWALL_PURPLE:
			setBackgroundColor(LIGHTMAGENTA);
			break;
		case CWWALL_RED_BRICK_FLAG:
			setBackgroundColor(LIGHTRED);
			setColor(LIGHTCYAN);
			c = '=';
			break;
		case CWWALL_ELEVATOR:
			setBackgroundColor(YELLOW);
			break;
		case CWWALL_PURPLE_BLOOD:
			setBackgroundColor(LIGHTMAGENTA);
			setColor(LIGHTRED);
			c = '#';
			break;
		default:
			c = '?';
			break;
		}
		break;
	case CWTILE_DOOR_H:
		setColor(GREY);
		c = '-';
		break;
	case CWTILE_DOOR_V:
		setColor(GREY);
		c = '|';
		break;
	case CWTILE_DOOR_GOLD_H:
		setColor(YELLOW);
		c = '-';
		break;
	case CWTILE_DOOR_GOLD_V:
		setColor(YELLOW);
		c = '|';
		break;
	case CWTILE_DOOR_SILVER_H:
		setColor(CYAN);
		c = '-';
		break;
	case CWTILE_DOOR_SILVER_V:
		setColor(CYAN);
		c = '|';
		break;
	case CWTILE_ELEVATOR_H:
		// TODO: confirm H elevator doors
		setColor(BROWN);
		c = '-';
		break;
	case CWTILE_ELEVATOR_V:
		setColor(BROWN);
		c = '|';
		break;
	case CWTILE_AREA:
		break;
	default:
		c = '?';
		break;
	}
	// Entity
	ch = CWLevelGetCh(level, 1, x, y);
	const CWEntity entity = CWChToEntity(ch);
	switch (entity)
	{
	case CWENT_NONE:
		break;
	case CWENT_PLAYER_SPAWN_N:
		setColor(LIGHTGREEN);
		c = '^';
		break;
	case CWENT_PLAYER_SPAWN_E:
		setColor(LIGHTGREEN);
		c = '>';
		break;
	case CWENT_WATER:
		setColor(LIGHTBLUE);
		c = 'O';
		break;
	case CWENT_OIL_DRUM:
		setColor(LIGHTGREEN);
		c = '|';
		break;
	case CWENT_TABLE_WITH_CHAIRS:
		setColor(RED);
		c = '#';
		break;
	case CWENT_FLOOR_LAMP:
		setColor(MAGENTA);
		c = '|';
		break;
	case CWENT_CHANDELIER:
		setBackgroundColor(GREY);
		setColor(YELLOW);
		c = '+';
		break;
	case CWENT_HANGING_SKELETON:
		setColor(LIGHTCYAN);
		c = '#';
		break;
	case CWENT_WHITE_COLUMN:
		setColor(WHITE);
		c = '|';
		break;
	case CWENT_DOG_FOOD:
		setColor(RED);
		c = '=';
		break;
	case CWENT_GREEN_PLANT:
		setBackgroundColor(GREEN);
		setColor(LIGHTGREEN);
		c = '+';
		break;
	case CWENT_SKELETON:
		setColor(WHITE);
		c = '#';
		break;
	case CWENT_SINK:
		setBackgroundColor(WHITE);
		setColor(BLACK);
		c = '+';
		break;
	case CWENT_BROWN_PLANT:
		setBackgroundColor(GREEN);
		setColor(RED);
		c = '+';
		break;
	case CWENT_VASE:
		setColor(BLUE);
		c = 'O';
		break;
	case CWENT_TABLE:
		setColor(BROWN);
		c = '#';
		break;
	case CWENT_CEILING_LIGHT_GREEN:
		setBackgroundColor(GREY);
		setColor(GREEN);
		c = '+';
		break;
	case CWENT_UTENSILS_BROWN:
		setBackgroundColor(CYAN);
		setColor(RED);
		c = '+';
		break;
	case CWENT_ARMOR:
		setColor(BLUE);
		c = '|';
		break;
	case CWENT_BONES1:
		setColor(BROWN);
		c = 'o';
		break;
	case CWENT_KEY_GOLD:
		setColor(YELLOW);
		c = 'X';
		break;
	case CWENT_BASKET:
		setBackgroundColor(RED);
		setColor(BLACK);
		c = '+';
		break;
	case CWENT_FOOD:
		setColor(LIGHTGREEN);
		c = '=';
		break;
	case CWENT_MEDKIT:
		setColor(LIGHTRED);
		c = '=';
		break;
	case CWENT_AMMO:
		setColor(CYAN);
		c = '=';
		break;
	case CWENT_MACHINE_GUN:
		setColor(CYAN);
		c = 'O';
		break;
	case CWENT_CHAIN_GUN:
		setColor(LIGHTCYAN);
		c = 'O';
		break;
	case CWENT_CROSS:
		setColor(LIGHTBLUE);
		c = '*';
		break;
	case CWENT_CHALICE:
		setColor(LIGHTMAGENTA);
		c = '*';
		break;
	case CWENT_CHEST:
		setColor(LIGHTCYAN);
		c = '*';
		break;
	case CWENT_LIFE:
		setColor(LIGHTBLUE);
		c = '=';
		break;
	case CWENT_BONES_BLOOD:
		setColor(LIGHTRED);
		c = 'O';
		break;
	case CWENT_BARREL:
		setColor(RED);
		c = '|';
		break;
	case CWENT_WELL_WATER:
		setBackgroundColor(GREY);
		setColor(LIGHTBLUE);
		c = '+';
		break;
	case CWENT_WELL:
		setBackgroundColor(GREY);
		setColor(BLACK);
		c = '+';
		break;
	case CWENT_FLAG:
		setColor(LIGHTRED);
		c = '|';
		break;
	case CWENT_PUSHWALL:
		setBackgroundColor(WHITE);
		break;
	case CWENT_DEAD_GUARD:
		setColor(MAGENTA);
		c = 'X';
		break;
	case CWENT_DOG_E:
		setColor(BROWN);
		c = '>';
		break;
	case CWENT_DOG_N:
		setColor(BROWN);
		c = '^';
		break;
	case CWENT_DOG_W:
		setColor(BROWN);
		c = '<';
		break;
	case CWENT_DOG_S:
		setColor(BROWN);
		c = 'v';
		break;
	case CWENT_GUARD_E:
		setColor(RED);
		c = '>';
		break;
	case CWENT_GUARD_N:
		setColor(RED);
		c = '^';
		break;
	case CWENT_GUARD_W:
		setColor(RED);
		c = '<';
		break;
	case CWENT_GUARD_S:
		setColor(RED);
		c = 'v';
		break;
	case CWENT_SS_E:
		setColor(BLUE);
		c = '>';
		break;
	case CWENT_SS_N:
		setColor(BLUE);
		c = '^';
		break;
	case CWENT_SS_W:
		setColor(BLUE);
		c = '<';
		break;
	case CWENT_SS_S:
		setColor(BLUE);
		c = 'v';
		break;
	case CWENT_TURN_E:
	case CWENT_TURN_NE:
	case CWENT_TURN_N:
	case CWENT_TURN_NW:
	case CWENT_TURN_W:
	case CWENT_TURN_SW:
	case CWENT_TURN_S:
	case CWENT_TURN_SE:
		break;
	default:
		printf("");
		break;
	}
	printf("%c", c);
	resetColor();
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
	saveDefaultColor();
	CWLevel *level = map.levels;
	for (int i = 0; i < map.nLevels; i++, level++)
	{
		printf(
			"Level %d: %s (%dx%d)\n", i + 1, level->header.name,
			level->header.width, level->header.height);
		for (int x = 0; x < level->header.width; x++)
		{
			for (int y = 0; y < level->header.height; y++)
			{
				PrintCh(level, x, y);
			}
			printf(" \n");
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
