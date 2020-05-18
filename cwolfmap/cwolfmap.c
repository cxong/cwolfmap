#include "cwolfmap.h"
#include <stdio.h>

#define MAGIC 0xABCD

#define PATH_MAX 4096

static int LoadMapHead(CWolfMap *map, const char *path);
int CWLoad(CWolfMap *map, const char *path)
{
	char pathBuf[PATH_MAX];
	sprintf(pathBuf, "%s/MAPHEAD.WL1", path);
	int err = LoadMapHead(map, pathBuf);
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

void CWFree(CWolfMap *map)
{
}