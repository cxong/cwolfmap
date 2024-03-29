#include "cwolfmap/cwolfmap.h"
#include "rlutil.h"
#include <SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>

#define SAMPLE_RATE 44100
#define AUDIO_FMT AUDIO_S16
#define AUDIO_CHANNELS 2

typedef struct
{
	char name[256];
	Mix_Chunk *snd;
} Sound;

static char itoc(int i)
{
	if (i < 10)
	{
		return (char)('0' + i);
	}
	i -= 10;
	if (i < 26)
	{
		return (char)('a' + i);
	}
	i -= 26;
	if (i < 26)
	{
		return (char)('A' + i);
	}
	return '?';
}
static int ctoi(const int c)
{
	int d = 0;
	if (c >= '0' && c <= '9')
	{
		return c - '0' + d;
	}
	d += 10;
	if (c >= 'a' && c <= 'z')
	{
		return c - 'a' + d;
	}
	d += 26;
	if (c >= 'A' && c <= 'Z')
	{
		return c - 'A' + d;
	}
	return -1;
}

int main(int argc, char *argv[])
{
	CWAudioInit();
	CWolfMap map;
	Sound *sounds = NULL;
	int nSounds = 0;
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

	if (Mix_OpenAudio(SAMPLE_RATE, AUDIO_FMT, AUDIO_CHANNELS, 1024) != 0)
	{
		goto bail;
	}

	for (int i = 0; i < map.vswap.nSounds; i++)
	{
		const char *data;
		size_t len;
		err = CWVSwapGetSound(&map.vswap, i, &data, &len);
		if (err != 0)
		{
			continue;
		}
		if (len == 0)
		{
			continue;
		}
		nSounds++;
		sounds = realloc(sounds, nSounds * sizeof(Sound));
		if (sounds == NULL)
		{
			goto bail;
		}
		SDL_AudioCVT cvt;
		SDL_BuildAudioCVT(
			&cvt, AUDIO_U8, 1, CWGetAudioSampleRate(&map), AUDIO_FMT,
			AUDIO_CHANNELS, SAMPLE_RATE);
		cvt.len = (int)len;
		cvt.buf = (Uint8 *)SDL_malloc(cvt.len * cvt.len_mult);
		memcpy(cvt.buf, data, len);
		SDL_ConvertAudio(&cvt);
		Sound *sound = &sounds[nSounds - 1];
		sprintf(sound->name, "SND%05d", i);
		sound->snd = Mix_QuickLoad_RAW(cvt.buf, cvt.len_cvt);
	}

	for (int i = 0; i < nSounds; i++)
	{
		printf("%c: %s\n", itoc(i), sounds[i].name);
	}
	printf("Choose sound to play: ");
	for (;;)
	{
		int cmd = ctoi(getch());
		if (cmd == -1)
		{
			break;
		}
		if (cmd >= nSounds)
		{
			continue;
		}
		Mix_PlayChannel(-1, sounds[cmd].snd, 0);
	}

bail:
	CWFree(&map);
	CWAudioTerminate();
	for (int i = 0; i < nSounds; i++)
	{
		Mix_FreeChunk(sounds[i].snd);
	}
	Mix_CloseAudio();
	free(sounds);
	return err;
}
