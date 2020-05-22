#include "cwolfmap/cwolfmap.h"
#include <stdio.h>

typedef struct wavfile_header_s
{
	char ChunkID[4];   /*  4   */
	int32_t ChunkSize; /*  4   */
	char Format[4];	   /*  4   */

	char Subchunk1ID[4];   /*  4   */
	int32_t Subchunk1Size; /*  4   */
	int16_t AudioFormat;   /*  2   */
	int16_t NumChannels;   /*  2   */
	int32_t SampleRate;	   /*  4   */
	int32_t ByteRate;	   /*  4   */
	int16_t BlockAlign;	   /*  2   */
	int16_t BitsPerSample; /*  2   */

	char Subchunk2ID[4];
	int32_t Subchunk2Size;
} wavfile_header_t;

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
		sprintf(buf, "SND%05d.wav", i);
		FILE *f = fopen(buf, "wb");
		if (f == NULL)
		{
			printf("Failed to open file for writing %s\n", buf);
			goto bail;
		}
		const int subchunk1Size = 16;
		const int32_t chunkSize = 4 + (8 + subchunk1Size) + (8 + len);
		wavfile_header_t header = {
			"RIFF",	  chunkSize, "WAVE", "fmt ", subchunk1Size, 1,	1,
			SND_RATE, SND_RATE,	 1,		 8,		 "data",		len};
		if (fwrite(&header, sizeof header, 1, f) != 1)
		{
			printf("Failed to write sound header %s\n", buf);
			fclose(f);
			continue;
		}
		if (fwrite(data, 1, len, f) != len)
		{
			printf("Failed to write sound data %s\n", buf);
			fclose(f);
			continue;
		}
		printf("Wrote file %s (len %d)\n", buf, len);
		fclose(f);
	}

bail:
	CWFree(&map);
	return err;
}
