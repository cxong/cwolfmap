#include "cwolfmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static int LoadLevel(CWLevel *level, const char *data);
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
		fprintf(stderr, "Gamemaps does not contain magic string " GAMEMAPS_MAGIC);
		goto bail;
	}

	const int32_t *ptr = &map->mapHead.ptr[0];
	for (const int32_t *ptr = &map->mapHead.ptr[0]; *ptr > 0; ptr++)
	{
		map->nLevels++;
	}
	map->levels = malloc(sizeof(CWLevel) * map->nLevels);
	CWLevel *lPtr = map->levels;
	for (const int32_t *ptr = &map->mapHead.ptr[0]; *ptr > 0; ptr++, lPtr++)
	{
		err = LoadLevel(lPtr, buf + *ptr);
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
	return err;
}
static int LoadLevel(CWLevel *level, const char *data)
{
	int err = 0;
	memcpy(&level->header, data, sizeof(level->header));
	if (strncmp(level->header.signature, "!ID!", 4) != 0)
	{
		err = -1;
		fprintf(
			stderr, "Level does not contain signature");
		goto bail;
	}

bail:
	return err;
}

void CWFree(CWolfMap *map)
{
	free(map->levels);
	memset(map, 0, sizeof *map);
}