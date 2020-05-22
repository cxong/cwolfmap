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
	printf("Loaded %d VSWAP sounds\n", map.vswap.nSounds);
	for (int i = 0; i < map.vswap.nSounds; i++)
	{
		const char *data;
		int len;
		err = CWVSwapGetSound(&map.vswap, i, &data, &len);
		if (err != 0)
		{
			continue;
		}
		if (len == 0)
		{
			continue;
		}
		char buf[256];
		sprintf(buf, "SND%05d.raw", i);
		FILE *f = fopen(buf, "wb");
		if (f == NULL)
		{
			printf("Failed to open file for writing %s\n", buf);
			goto bail;
		}
		if (fwrite(data, 1, len, f) != len)
		{
			printf("Failed to write sound data %s\n", buf);
		}
		else
		{
			printf("Wrote file %s (len %d)\n", buf, len);
		}
		fclose(f);
	}

bail:
	CWFree(&map);
	return err;
}
