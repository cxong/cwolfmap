#include "cwolfmap/cwolfmap.h"
#include "rlutil.h"
#include <SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>


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
	printf("Loaded %d music tracks\n", map.audio.nMusic);

	if (Mix_OpenAudio(MUSIC_SAMPLE_RATE, MUSIC_AUDIO_FMT, MUSIC_AUDIO_CHANNELS, 1024) != 0)
	{
		goto bail;
	}

	for (int i = 0; i < map.audio.nMusic; i++)
	{
		char *data;
		size_t len;
		err = CWAudioGetMusic(&map.audio, i, &data, &len);
		if (err != 0)
		{
			goto bail;
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
		printf("Loaded adlib music %d (%d len)\n", i, (int)len);

		Sound *sound = &sounds[nSounds - 1];
		sprintf(sound->name, "MUS%05d", i);
		sound->snd = Mix_QuickLoad_RAW((Uint8 *)data, len);
	}

	for (int i = 0; i < nSounds; i++)
	{
		printf("%c: %s\n", itoc(i), sounds[i].name);
	}
	printf("Choose music to play: ");
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
		Mix_PlayChannel(0, sounds[cmd].snd, 0);
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
