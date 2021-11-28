#include "cwolfmap/cwolfmap.h"
#include "rlutil.h"
#include <stdio.h>

static void PrintCh(
	const CWLevel *level, const int x, const int y, const CWMapType type)
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
			setBackgroundColor(DARKGREY);
			break;
		case CWWALL_GREY_BRICK_2:
			setBackgroundColor(DARKGREY);
			setColor(BLACK);
			c = '#';
			break;
		case CWWALL_GREY_BRICK_FLAG:
			setBackgroundColor(DARKGREY);
			setColor(RED);
			c = '=';
			break;
		case CWWALL_GREY_BRICK_HITLER:
			setBackgroundColor(DARKGREY);
			setColor(YELLOW);
			c = '=';
			break;
		case CWWALL_CELL:
			setBackgroundColor(BLUE);
			setColor(BLACK);
			c = '=';
			break;
		case CWWALL_GREY_BRICK_EAGLE:
			setBackgroundColor(DARKGREY);
			setColor(LIGHTCYAN);
			c = '=';
			break;
		case CWWALL_CELL_SKELETON:
			setBackgroundColor(BLUE);
			setColor(WHITE);
			c = '=';
			break;
		case CWWALL_BLUE_BRICK_1:
			setBackgroundColor(BLUE);
			break;
		case CWWALL_BLUE_BRICK_2:
			setBackgroundColor(BLUE);
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
		case CWWALL_STEEL_SIGN:
			setBackgroundColor(LIGHTCYAN);
			setColor(BLACK);
			c = '=';
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
		case CWWALL_DEAD_ELEVATOR:
			setBackgroundColor(YELLOW);
			setColor(LIGHTRED);
			c = '=';
			break;
		case CWWALL_WOOD_IRON_CROSS:
			setBackgroundColor(RED);
			setColor(DARKGREY);
			c = '=';
			break;
		case CWWALL_DIRTY_BRICK_1:
			setBackgroundColor(DARKGREY);
			setColor(GREEN);
			c = '=';
			break;
		case CWWALL_DIRTY_BRICK_2:
			setBackgroundColor(DARKGREY);
			setColor(GREEN);
			c = '#';
			break;
		case CWWALL_GREY_BRICK_3:
			setBackgroundColor(DARKGREY);
			setColor(GREY);
			c = '=';
			break;
		case CWWALL_PURPLE_BLOOD:
			setBackgroundColor(LIGHTMAGENTA);
			setColor(LIGHTRED);
			c = '#';
			break;
		case CWWALL_GREY_BRICK_SIGN:
			setBackgroundColor(GREY);
			setColor(BLACK);
			c = '#';
			break;
		case CWWALL_BROWN_WEAVE:
			setBackgroundColor(BROWN);
			break;
		case CWWALL_BROWN_WEAVE_BLOOD_2:
			setBackgroundColor(BROWN);
			setColor(MAGENTA);
			c = '=';
			break;
		case CWWALL_BROWN_WEAVE_BLOOD_3:
			setBackgroundColor(BROWN);
			setColor(LIGHTMAGENTA);
			c = '=';
			break;
		case CWWALL_BROWN_WEAVE_BLOOD_1:
			setBackgroundColor(BROWN);
			setColor(LIGHTRED);
			c = '=';
			break;
		case CWWALL_STAINED_GLASS:
			setBackgroundColor(GREY);
			setColor(LIGHTGREEN);
			c = '=';
			break;
		case CWWALL_BLUE_WALL_SKULL:
			setBackgroundColor(LIGHTBLUE);
			setColor(BLACK);
			c = '=';
			break;
		case CWWALL_GREY_WALL_1:
			setBackgroundColor(GREY);
			break;
		case CWWALL_BLUE_WALL_SWASTIKA:
			setBackgroundColor(LIGHTBLUE);
			setColor(LIGHTRED);
			c = '=';
			break;
		case CWWALL_GREY_WALL_VENT:
			setBackgroundColor(GREY);
			setColor(BLACK);
			c = '=';
			break;
		case CWWALL_MULTICOLOR_BRICK:
			setBackgroundColor(LIGHTRED);
			setColor(LIGHTBLUE);
			c = '=';
			break;
		case CWWALL_GREY_WALL_2:
			setBackgroundColor(GREY);
			setColor(BLACK);
			c = '#';
			break;
		case CWWALL_BLUE_WALL:
			setBackgroundColor(LIGHTBLUE);
			break;
		case CWWALL_BLUE_BRICK_SIGN:
			setBackgroundColor(BLUE);
			setColor(YELLOW);
			c = '=';
			break;
		case CWWALL_BROWN_MARBLE_1:
			setBackgroundColor(RED);
			setColor(BROWN);
			c = '_';
			break;
		case CWWALL_GREY_WALL_MAP:
			setBackgroundColor(GREY);
			setColor(LIGHTBLUE);
			c = '=';
			break;
		case CWWALL_BROWN_STONE_1:
			setBackgroundColor(BROWN);
			setColor(BLACK);
			c = '#';
			break;
		case CWWALL_BROWN_STONE_2:
			setBackgroundColor(BROWN);
			setColor(RED);
			c = '#';
			break;
		case CWWALL_BROWN_MARBLE_2:
			setBackgroundColor(GREEN);
			setColor(RED);
			c = '_';
			break;
		case CWWALL_BROWN_MARBLE_FLAG:
			setBackgroundColor(LIGHTCYAN);
			setColor(RED);
			c = '_';
			break;
		case CWWALL_WOOD_PANEL:
			setBackgroundColor(RED);
			setColor(YELLOW);
			c = '#';
			break;
		case CWWALL_GREY_WALL_HITLER:
			setBackgroundColor(GREY);
			setColor(YELLOW);
			c = '=';
			break;
		case CWWALL_STONE_WALL_1:
			setBackgroundColor(RED);
			setColor(GREEN);
			c = '#';
			break;
		case CWWALL_STONE_WALL_2:
			setBackgroundColor(RED);
			setColor(BROWN);
			c = '#';
			break;
		case CWWALL_STONE_WALL_FLAG:
			setBackgroundColor(RED);
			setColor(LIGHTGREEN);
			c = '#';
			break;
		case CWWALL_STONE_WALL_WREATH:
			setBackgroundColor(RED);
			setColor(LIGHTRED);
			c = '#';
			break;
		case CWWALL_GREY_CONCRETE_LIGHT:
			setBackgroundColor(DARKGREY);
			setColor(RED);
			c = '#';
			break;
		case CWWALL_GREY_CONCRETE_DARK:
			setBackgroundColor(DARKGREY);
			setColor(BROWN);
			c = '#';
			break;
		case CWWALL_BLOOD_WALL:
			setBackgroundColor(LIGHTRED);
			setColor(BLACK);
			c = '#';
			break;
		case CWWALL_CONCRETE:
			setBackgroundColor(GREY);
			setColor(DARKGREY);
			c = '#';
			break;
		case CWWALL_RAMPART_STONE_1:
			setBackgroundColor(CYAN);
			setColor(GREY);
			c = '#';
			break;
		case CWWALL_RAMPART_STONE_2:
			setBackgroundColor(CYAN);
			setColor(DARKGREY);
			c = '#';
			break;
		case CWWALL_ELEVATOR_WALL:
			setBackgroundColor(YELLOW);
			setColor(LIGHTMAGENTA);
			c = '#';
			break;
		case CWWALL_WHITE_PANEL:
			setBackgroundColor(WHITE);
			setColor(RED);
			c = '=';
			break;
		case CWWALL_BROWN_CONCRETE:
			setBackgroundColor(BROWN);
			setColor(GREY);
			c = '#';
			break;
		case CWWALL_PURPLE_BRICK:
			setBackgroundColor(LIGHTMAGENTA);
			setColor(BLACK);
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
		setColor(BROWN);
		c = '-';
		break;
	case CWTILE_ELEVATOR_V:
		setColor(BROWN);
		c = '|';
		break;
	case CWTILE_AREA:
		break;
	case CWTILE_SECRET_EXIT:
		setColor(LIGHTCYAN);
		c = '#';
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
	case CWENT_PLAYER_SPAWN_S:
		setColor(LIGHTGREEN);
		c = 'v';
		break;
	case CWENT_PLAYER_SPAWN_W:
		setColor(LIGHTGREEN);
		c = '<';
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
	case CWENT_SINK_SKULLS_ON_STICK:
		if (type == CWMAPTYPE_SOD)
		{
			setColor(LIGHTMAGENTA);
			c = '#';
		}
		else
		{
			setBackgroundColor(WHITE);
			setColor(BLACK);
			c = '+';
		}
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
	case CWENT_UTENSILS_BROWN_CAGE_BLOODY_BONES:
		if (type == CWMAPTYPE_SOD)
		{
			setBackgroundColor(LIGHTMAGENTA);
			setColor(LIGHTRED);
			c = '+';
		}
		else
		{
			setBackgroundColor(CYAN);
			setColor(RED);
			c = '+';
		}
		break;
	case CWENT_ARMOR:
		setColor(BLUE);
		c = '|';
		break;
	case CWENT_CAGE:
		setBackgroundColor(LIGHTMAGENTA);
		setColor(BLACK);
		c = '+';
		break;
	case CWENT_CAGE_SKELETON:
		setBackgroundColor(LIGHTMAGENTA);
		setColor(WHITE);
		c = '+';
		break;
	case CWENT_BONES1:
		setColor(BROWN);
		c = 'H';
		break;
	case CWENT_KEY_GOLD:
		setColor(YELLOW);
		c = 'X';
		break;
	case CWENT_KEY_SILVER:
		setColor(LIGHTCYAN);
		c = 'X';
		break;
	case CWENT_BED_CAGE_SKULLS:
		if (type == CWMAPTYPE_SOD)
		{
			setBackgroundColor(LIGHTMAGENTA);
			setColor(RED);
			c = '+';
		}
		else
		{
			setColor(RED);
			c = '~';
		}
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
	case CWENT_CROWN:
		setColor(WHITE);
		c = '*';
		break;
	case CWENT_LIFE:
		setColor(LIGHTBLUE);
		c = '=';
		break;
	case CWENT_BONES_BLOOD:
		setColor(LIGHTRED);
		c = 'H';
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
	case CWENT_POOL_OF_BLOOD:
		setColor(LIGHTRED);
		c = 'O';
		break;
	case CWENT_FLAG:
		setColor(LIGHTRED);
		c = '|';
		break;
	case CWENT_CEILING_LIGHT_RED_AARDWOLF:
		if (type == CWMAPTYPE_WL6)
		{
			setBackgroundColor(LIGHTGREEN);
		}
		else
		{
			setBackgroundColor(GREY);
			setColor(LIGHTRED);
			c = '+';
			break;
		}
		break;
	case CWENT_BONES2:
		setColor(GREY);
		c = 'H';
		break;
	case CWENT_BONES3:
		setColor(YELLOW);
		c = 'H';
		break;
	case CWENT_BONES4:
		setColor(WHITE);
		c = 'H';
		break;
	case CWENT_UTENSILS_BLUE_COW_SKULL:
		if (type == CWMAPTYPE_SOD)
		{
			setColor(MAGENTA);
			c = '#';
		}
		else
		{
			setBackgroundColor(CYAN);
			setColor(LIGHTBLUE);
			c = '+';
		}
		break;
	case CWENT_STOVE_WELL_BLOOD:
		if (type == CWMAPTYPE_SOD)
		{
			setBackgroundColor(GREY);
			setColor(RED);
			c = '+';
		}
		else
		{
			setColor(GREY);
			c = '#';
		}
		break;
	case CWENT_RACK_ANGEL_STATUE:
		if (type == CWMAPTYPE_SOD)
		{
			setColor(CYAN);
			c = '#';
		}
		else
		{
			setBackgroundColor(RED);
			setColor(BLACK);
			c = '#';
		}
		break;
	case CWENT_VINES:
		setColor(GREEN);
		c = '#';
		break;
	case CWENT_BROWN_COLUMN:
		setColor(CYAN);
		c = '|';
		break;
	case CWENT_AMMO_BOX:
		setColor(CYAN);
		c = 'H';
		break;
	case CWENT_TRUCK_REAR:
		setColor(BLUE);
		c = '#';
		break;
	case CWENT_SPEAR:
		setColor(YELLOW);
		c = '$';
		break;
	case CWENT_PUSHWALL:
		setBackgroundColor(WHITE);
		break;
	case CWENT_ENDGAME:
		setColor(RED);
		c = '$';
		break;
	case CWENT_GHOST:
		setColor(LIGHTCYAN);
		c = '$';
		break;
	case CWENT_ANGEL:
		setColor(LIGHTRED);
		c = '$';
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
	case CWENT_GUARD_MOVING_E:
		setColor(RED);
		c = '>';
		break;
	case CWENT_GUARD_N:
	case CWENT_GUARD_MOVING_N:
		setColor(RED);
		c = '^';
		break;
	case CWENT_GUARD_W:
	case CWENT_GUARD_MOVING_W:
		setColor(RED);
		c = '<';
		break;
	case CWENT_GUARD_S:
	case CWENT_GUARD_MOVING_S:
		setColor(RED);
		c = 'v';
		break;
	case CWENT_SS_E:
	case CWENT_SS_MOVING_E:
		setColor(BLUE);
		c = '>';
		break;
	case CWENT_SS_N:
	case CWENT_SS_MOVING_N:
		setColor(BLUE);
		c = '^';
		break;
	case CWENT_SS_W:
	case CWENT_SS_MOVING_W:
		setColor(BLUE);
		c = '<';
		break;
	case CWENT_SS_S:
	case CWENT_SS_MOVING_S:
		setColor(BLUE);
		c = 'v';
		break;
	case CWENT_MUTANT_E:
	case CWENT_MUTANT_MOVING_E:
		setColor(GREEN);
		c = '>';
		break;
	case CWENT_MUTANT_N:
	case CWENT_MUTANT_MOVING_N:
		setColor(GREEN);
		c = '^';
		break;
	case CWENT_MUTANT_W:
	case CWENT_MUTANT_MOVING_W:
		setColor(GREEN);
		c = '<';
		break;
	case CWENT_MUTANT_S:
	case CWENT_MUTANT_MOVING_S:
		setColor(GREEN);
		c = 'v';
		break;
	case CWENT_OFFICER_E:
	case CWENT_OFFICER_MOVING_E:
		setColor(GREY);
		c = '>';
		break;
	case CWENT_OFFICER_N:
	case CWENT_OFFICER_MOVING_N:
		setColor(GREY);
		c = '^';
		break;
	case CWENT_OFFICER_W:
	case CWENT_OFFICER_MOVING_W:
		setColor(GREY);
		c = '<';
		break;
	case CWENT_OFFICER_S:
	case CWENT_OFFICER_MOVING_S:
		setColor(GREY);
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
	case CWENT_TRANS:
		setColor(GREEN);
		c = '$';
		break;
	case CWENT_UBER_MUTANT:
		setColor(LIGHTGREEN);
		c = '$';
		break;
	case CWENT_BARNACLE_WILHELM:
		setColor(LIGHTBLUE);
		c = '$';
		break;
	case CWENT_ROBED_HITLER:
		setColor(DARKGREY);
		c = '$';
		break;
	case CWENT_DEATH_KNIGHT:
		setColor(DARKGREY);
		c = '$';
		break;
	case CWENT_HITLER:
		setColor(LIGHTCYAN);
		c = '$';
		break;
	case CWENT_FETTGESICHT:
		setColor(LIGHTGREEN);
		c = '$';
		break;
	case CWENT_SCHABBS:
		setColor(LIGHTMAGENTA);
		c = '$';
		break;
	case CWENT_GRETEL:
		setColor(YELLOW);
		c = '$';
		break;
	case CWENT_HANS:
		setColor(LIGHTBLUE);
		c = '$';
		break;
	case CWENT_OTTO:
		setColor(WHITE);
		c = '$';
		break;
	case CWENT_PACMAN_GHOST_RED:
		setColor(LIGHTRED);
		c = 'G';
		break;
	case CWENT_PACMAN_GHOST_YELLOW:
		setColor(YELLOW);
		c = 'G';
		break;
	case CWENT_PACMAN_GHOST_ROSE:
		setColor(LIGHTMAGENTA);
		c = 'G';
		break;
	case CWENT_PACMAN_GHOST_BLUE:
		setColor(LIGHTBLUE);
		c = 'G';
		break;
	default:
		c = '?';
		break;
	}
	printf("%c", c);
	resetColor();
}

int main(int argc, char *argv[])
{
	CWAudioInit();
	CWolfMap map;
	int err = 0;
	if (argc == 2)
	{
		err = CWLoad(&map, argv[1], 0);
	}
	else
	{
		err = CWLoad(&map, "WOLF3D", 0);
	}
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
		for (int y = 0; y < level->header.height; y++)
		{
			for (int x = 0; x < level->header.width; x++)
			{
				PrintCh(level, x, y, map.type);
			}
			printf(" \n");
		}
	}

	printf("Loaded %zu sounds\n", map.audio.head.nOffsets);

	printf("Loaded %d VSWAP chunks\n", map.vswap.head.chunkCount);
	printf("- sprite: %d\n", map.vswap.head.firstSprite);
	printf("- sound: %d\n", map.vswap.head.firstSound);

bail:
	CWFree(&map);
	CWAudioTerminate();
	return err;
}
