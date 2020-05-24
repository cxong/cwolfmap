#include "cwolfmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "expand.h"

#define MAGIC 0xABCD

#define PATH_MAX 4096

static int LoadMapHead(CWolfMap *map, const char *path);
static int LoadMapData(CWolfMap *map, const char *path);
int CWLoad(CWolfMap *map, const char *path)
{
	char pathBuf[PATH_MAX];
	int err;

	sprintf(pathBuf, "%s/MAPHEAD.WL1", path);
	err = LoadMapHead(map, pathBuf);
	if (err != 0)
	{
		goto bail;
	}

	sprintf(pathBuf, "%s/GAMEMAPS.WL1", path);
	err = LoadMapData(map, pathBuf);
	if (err != 0)
	{
		goto bail;
	}

	sprintf(pathBuf, "%s/AUDIOHED.WL1", path);
	err = CWAudioLoadHead(&map->audio.head, pathBuf);
	if (err != 0)
	{
		goto bail;
	}

	sprintf(pathBuf, "%s/AUDIOT.WL1", path);
	err = CWAudioLoadAudioT(&map->audio, pathBuf);
	if (err != 0)
	{
		goto bail;
	}

	sprintf(pathBuf, "%s/VSWAP.WL1", path);
	err = CWVSwapLoad(&map->vswap, pathBuf);
	if (err != 0)
	{
		goto bail;
	}

bail:
	return err;
}
static int LoadMapHead(CWolfMap *map, const char *path)
{
	int err = 0;
	memset(map, 0, sizeof *map);
	FILE *f = fopen(path, "rb");
	if (!f)
	{
		err = -1;
		fprintf(stderr, "Failed to read %s", path);
		goto bail;
	}
	const size_t size = sizeof map->mapHead;
	const size_t read = fread((void *)&map->mapHead, 1, size, f);
	if (read != size)
	{
		err = -1;
		fprintf(
			stderr, "Failed to read maphead; only read %d bytes of %d", read,
			size);
		goto bail;
	}
	if (map->mapHead.magic != MAGIC)
	{
		err = -1;
		fprintf(stderr, "Unpexpected magic value: %x", map->mapHead.magic);
		goto bail;
	}

bail:
	if (f)
	{
		fclose(f);
	}
	return err;
}

#define GAMEMAPS_MAGIC "TED5v1.0"

static int LoadLevel(CWLevel *level, const char *data, const int off);
static int LoadMapData(CWolfMap *map, const char *path)
{
	int err = 0;
	char *buf = NULL;
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
	buf = malloc(fsize);
	if (fread(buf, 1, fsize, f) != fsize)
	{
		err = -1;
		fprintf(stderr, "Failed to read file");
		goto bail;
	}
	if (strncmp(buf, GAMEMAPS_MAGIC, strlen(GAMEMAPS_MAGIC)) != 0)
	{
		err = -1;
		fprintf(
			stderr, "Gamemaps does not contain magic string " GAMEMAPS_MAGIC);
		goto bail;
	}

	const int32_t *ptr;
	for (const int32_t *ptr = &map->mapHead.ptr[0]; *ptr > 0; ptr++)
	{
		map->nLevels++;
	}
	map->levels = malloc(sizeof(CWLevel) * map->nLevels);
	CWLevel *lPtr = map->levels;
	for (const int32_t *ptr = &map->mapHead.ptr[0]; *ptr > 0; ptr++, lPtr++)
	{
		err = LoadLevel(lPtr, buf, *ptr);
		if (err != 0)
		{
			goto bail;
		}
	}

bail:
	if (f)
	{
		fclose(f);
	}
	free(buf);
	if (err != 0)
	{
		CWFree(map);
	}
	return err;
}
static int LoadPlane(
	CWPlane *plane, const char *data, const uint32_t off, const uint16_t len,
	char *buf, const int bufSize);
static int LoadLevel(CWLevel *level, const char *data, const int off)
{
	int err = 0;
	char *buf = NULL;
	memcpy(&level->header, data + off, sizeof(level->header));
	if (strncmp(level->header.signature, "!ID!", 4) != 0)
	{
		err = -1;
		fprintf(stderr, "Level does not contain signature");
		goto bail;
	}

	const int bufSize =
		level->header.width * level->header.height * sizeof(uint16_t);
	buf = malloc(bufSize);
	for (int i = 0; i < NUM_PLANES; i++)
	{
		const uint32_t off = *(&level->header.offPlane0 + i);
		const uint16_t len = *(&level->header.lenPlane0 + i);
		err = LoadPlane(&level->planes[i], data, off, len, buf, bufSize);
		if (err != 0)
		{
			goto bail;
		}
	}

bail:
	free(buf);
	return err;
}
static int LoadPlane(
	CWPlane *plane, const char *data, const uint32_t off, const uint16_t len,
	char *buf, const int bufSize)
{
	int err = 0;

	if (off == 0)
	{
		goto bail;
	}

	ExpandCarmack(data + off, buf);
	plane->len = bufSize;
	plane->plane = malloc(bufSize);
	ExpandRLEW(buf, plane->plane, MAGIC);

bail:
	return err;
}

static void LevelFree(CWLevel *level);
void CWFree(CWolfMap *map)
{
	for (int i = 0; i < map->nLevels; i++)
	{
		LevelFree(&map->levels[i]);
	}
	free(map->levels);
	CWAudioFree(&map->audio);
	CWVSwapFree(&map->vswap);
	memset(map, 0, sizeof *map);
}
static void LevelFree(CWLevel *level)
{
	for (int i = 0; i < NUM_PLANES; i++)
	{
		free(level->planes[i].plane);
	}
}

// http://gaarabis.free.fr/_sites/specs/wlspec_index.html

uint16_t CWLevelGetCh(
	const CWLevel *level, const int planeIndex, const int x, const int y)
{
	const CWPlane *plane = &level->planes[planeIndex];
	return plane->plane[x * level->header.height + y];
}

static const CWTile tileMap[] = {
	// 0-63
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	CWTILE_WALL,
	// 64-89
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	// 90
	CWTILE_DOOR_V,
	CWTILE_DOOR_H,
	CWTILE_DOOR_GOLD_V,
	CWTILE_DOOR_GOLD_H,
	CWTILE_DOOR_SILVER_V,
	CWTILE_DOOR_SILVER_H,
	// 96-99
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	// 100
	CWTILE_ELEVATOR_V,
	CWTILE_ELEVATOR_H,
	// 102-105
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	CWTILE_UNKNOWN,
	// 106-143
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
	CWTILE_AREA,
};

CWTile CWChToTile(const uint16_t ch)
{
	if (ch < sizeof tileMap / sizeof tileMap[0])
	{
		return tileMap[ch];
	}
	return CWTILE_UNKNOWN;
}

static const CWWall wallMap[] = {
	CWWALL_UNKNOWN,
	CWWALL_GREY_BRICK_1,
	CWWALL_GREY_BRICK_2,
	CWWALL_GREY_BRICK_FLAG,
	CWWALL_GREY_BRICK_HITLER,
	CWWALL_CELL,
	CWWALL_GREY_BRICK_EAGLE,
	CWWALL_CELL_SKELETON,
	CWWALL_BLUE_BRICK_1,
	CWWALL_BLUE_BRICK_2,
	CWWALL_WOOD_EAGLE,
	CWWALL_WOOD_HITLER,
	CWWALL_WOOD,
	CWWALL_ENTRANCE,
	CWWALL_UNKNOWN,
	CWWALL_STEEL,
	CWWALL_LANDSCAPE,
	CWWALL_RED_BRICK,
	CWWALL_RED_BRICK_SWASTIKA,
	CWWALL_PURPLE,
	CWWALL_RED_BRICK_FLAG,
	CWWALL_ELEVATOR,
	CWWALL_UNKNOWN,
	CWWALL_UNKNOWN,
	CWWALL_UNKNOWN,
	CWWALL_PURPLE_BLOOD,
};

CWWall CWChToWall(const uint16_t ch)
{
	if (ch < sizeof wallMap / sizeof wallMap[0])
	{
		return wallMap[ch];
	}
	return CWWALL_UNKNOWN;
}

CWEntity CWLevelGetEntity(const CWLevel *level, const int x, const int y)
{
	const uint16_t ch = CWLevelGetCh(level, 1, x, y);
	// TODO: map
	return CWENT_UNKNOWN;
}
