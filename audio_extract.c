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
	printf("Loaded %d music tracks\n", map.audio.nMusic);
	for (int i = 0; i < map.audio.nMusic; i++)
	{
		const char *data;
		int len;
		err = CWAudioGetMusic(&map.audio, i, &data, &len);
		if (err != 0)
		{
			goto bail;
		}
		char buf[256];
		sprintf(buf, "%d.imf", i);
		FILE *f = fopen(buf, "wb");
		if (f == NULL)
		{
			printf("Failed to open file for writing %s\n", buf);
			goto bail;
		}
		if (fwrite(data, 1, len, f) != len)
		{
			printf("Failed to write audio data %s\n", buf);
			fclose(f);
			goto bail;
		}
		fclose(f);
		printf("Wrote file %s (len %d)\n", buf, len);
	}
	// The files can be played via adplug e.g. https://www.wothke.ch/AdLibido/

bail:
	CWFree(&map);
	return err;
}
