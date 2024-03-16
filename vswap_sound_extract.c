#include "cwolfmap/cwolfmap.h"
#include <SDL_mixer.h>
#include <stdio.h>

#define SAMPLE_RATE 44100
#define AUDIO_FMT AUDIO_S16
#define AUDIO_CHANNELS 2

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

static int extract_sound(const CWolfMap *map, const int i)
{
	const char *data;
	size_t len;
	FILE *f = NULL;
	SDL_AudioCVT cvt;
	SDL_BuildAudioCVT(
		&cvt, AUDIO_U8, 1, CWGetAudioSampleRate(map), AUDIO_FMT,
		AUDIO_CHANNELS, SAMPLE_RATE);

	int err = CWVSwapGetSound(&map->vswap, i, &data, &len);
	if (err != 0)
	{
		// Ignore
		err = 0;
		goto bail;
	}
	if (len == 0)
	{
		goto bail;
	}

	cvt.len = (int)len;
	cvt.buf = (Uint8 *)SDL_malloc(cvt.len * cvt.len_mult);
	memcpy(cvt.buf, data, len);
	SDL_ConvertAudio(&cvt);

	char buf[256];
	sprintf(buf, "SND%05d.wav", i);
	f = fopen(buf, "wb");
	if (f == NULL)
	{
		printf("Failed to open file for writing %s\n", buf);
		goto bail;
	}

	const int subchunk1Size = 16;
	const int32_t chunkSize = 4 + (8 + subchunk1Size) + (8 + cvt.len_cvt);
	wavfile_header_t header = {
		{'R', 'I', 'F', 'F'},
		chunkSize,
		{'W', 'A', 'V', 'E'},
		{'f', 'm', 't', ' '},
		subchunk1Size,
		1,
		AUDIO_CHANNELS,
		SAMPLE_RATE,
		SAMPLE_RATE * AUDIO_CHANNELS * 2,
		AUDIO_CHANNELS * 2,
		16,
		{'d', 'a', 't', 'a'},
		cvt.len_cvt};
	if (fwrite(&header, sizeof header, 1, f) != 1)
	{
		printf("Failed to write sound header %s\n", buf);
		goto bail;
	}
	if ((int)fwrite(cvt.buf, 1, cvt.len_cvt, f) != cvt.len_cvt)
	{
		printf("Failed to write sound data %s\n", buf);
		goto bail;
	}
	printf("Wrote file %s (len %d)\n", buf, cvt.len_cvt);

bail:
	if (f)
	{
		fclose(f);
	}
	SDL_free(cvt.buf);
	return err;
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;
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
	printf("Loaded %d VSWAP sounds\n", map.vswap.nSounds);
	for (int i = 0; i < map.vswap.nSounds; i++)
	{
		err = extract_sound(&map, i);
		if (err != 0)
		{
			goto bail;
		}
	}

bail:
	CWFree(&map);
	return err;
}
