#include "cwolfmap/cwolfmap.h"
#include <stdbool.h>
#include <stdio.h>
#include <SDL_mixer.h>

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

int main(void)
{
	CWolfMap map;
	Sound *sounds = NULL;
	int nSounds = 0;
	int err = 0;
	err = CWLoad(&map, "WOLF3D");
	if (err != 0)
	{
		goto bail;
	}
	printf("Loaded %d VSWAP sounds\n", map.vswap.nSounds);

	// TODO: support different output format
	if (Mix_OpenAudio(SND_RATE, AUDIO_U8, 1, 1) != 0)
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
		Sound *sound = &sounds[nSounds - 1];
		sprintf(sound->name, "SND%05d", i);
		sound->snd = Mix_QuickLoad_RAW((Uint8 *)data, len);
	}

	for (;;)
	{
		for (int i = 0; i < nSounds; i++)
		{
			printf("%c: %s\n", itoc(i), sounds[i].name);
		}
		printf("Choose sound to play: ");
		int cmd = ctoi(getchar());
		if (cmd == -1)
		{
			break;
		}
		Mix_PlayChannel(-1, sounds[cmd].snd, 0);
		// Clear input buf
		while ((cmd = getchar()) != '\n' && cmd != EOF)
			;
	}

bail:
	CWFree(&map);
	for (int i = 0; i < nSounds; i++)
	{
		Mix_FreeChunk(sounds[i].snd);
	}
	Mix_CloseAudio();
	free(sounds);
	return err;
}
