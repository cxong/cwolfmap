#include "cwolfmap/cwolfmap.h"
#include "rlutil.h"
#include <SDL_mixer.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SAMPLE_RATE 44100
#define AUDIO_FMT AUDIO_S16
#define AUDIO_CHANNELS 2
#define MUSIC_RATE 700
#define SAMPLES_PER_MUSIC_TICK (SAMPLE_RATE / MUSIC_RATE)

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


static void SD_StartMusic(const char *data, const int len, Uint8 **out, int *outLen)
{
	static const Instrument ChannelRelease = {
		0, 0, 0x3F,		0x3F, 0xFF, 0xFF, 0xF, 0xF, 0, 0, 0,

		0, 0, {0, 0, 0}};
	for (int i = 0; i < OPL_CHANNELS; ++i)
		SDL_AlSetChanInst(&ChannelRelease, i);

	const uint16_t *sqHack = (const uint16_t *)data;
	int sqHackLen;
	if (*sqHack == 0)
	{
		// LumpLength?
		sqHackLen = len;
	}
	else
	{
		sqHackLen = *sqHack++;
	}
	const uint16_t *sqHackPtr = sqHack;

	int sqHackTime = 0;
	int alTimeCount;
	for (alTimeCount = 0; sqHackLen > 0; alTimeCount++)
	{
		do
		{
			if (sqHackTime > alTimeCount)
				break;
			sqHackTime = alTimeCount + *(sqHackPtr + 1);
			sqHackPtr += 2;
			sqHackLen -= 4;
		} while (sqHackLen > 0);
	}
	*outLen = alTimeCount * SAMPLES_PER_MUSIC_TICK * 4;
	*out = malloc(*outLen);
	int16_t *stream16 = (int16_t *)*out;

	sqHack = (const uint16_t *)data;
	if(*sqHack == 0)
	{
		// LumpLength?
		sqHackLen = len;
	}
	else
	{
		sqHackLen = *sqHack++;
	}
	sqHackPtr = sqHack;
	sqHackTime = 0;
	for (alTimeCount = 0; sqHackLen > 0; alTimeCount++)
	{
		do
		{
			if (sqHackTime > alTimeCount)
				break;
			sqHackTime = alTimeCount + *(sqHackPtr + 1);
			alOut(
				*(const uint8_t *)sqHackPtr,
				*(((const uint8_t *)sqHackPtr) + 1));
			sqHackPtr += 2;
			sqHackLen -= 4;
		} while (sqHackLen > 0);

		const int numreadysamples = SAMPLES_PER_MUSIC_TICK;
		YM3812UpdateOne(oplChip, stream16, numreadysamples);

		stream16 += numreadysamples * 2;
	}
}

int main(int argc, char *argv[])
{
	CWolfMap map;
	Sound *sounds = NULL;
	int nSounds = 0;
	int err = 0;
	if (argc == 2)
	{
		err = CWLoad(&map, argv[1]);
	}
	else
	{
		err = CWLoad(&map, "WOLF3D");
	}

	if (err != 0)
	{
		goto bail;
	}
	printf("Loaded %d music tracks\n", map.audio.nMusic);

	if (Mix_OpenAudio(SAMPLE_RATE, AUDIO_FMT, AUDIO_CHANNELS, 1024) != 0)
	{
		goto bail;
	}

	for (int i = 0; i < map.audio.nMusic; i++)
	{
		const char *data;
		size_t len;
		err = CWAudioGetMusic(&map.audio, i, &data, &len);
		if (err != 0)
		{
			goto bail;
		}
		nSounds++;
		sounds = realloc(sounds, nSounds * sizeof(Sound));
		if (sounds == NULL)
		{
			goto bail;
		}
		Sound *sound = &sounds[nSounds - 1];
		sprintf(sound->name, "MUS%05d", i);
		Uint8 *out;
		int outLen;
		SD_StartMusic(data, len, &out, &outLen);
		printf("Loaded adlib music %d (%d len, %d decoded len)\n", i, (int)len, outLen);

		sound->snd = Mix_QuickLoad_RAW(out, outLen);
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
	for (int i = 0; i < nSounds; i++)
	{
		Mix_FreeChunk(sounds[i].snd);
	}
	Mix_CloseAudio();
	free(sounds);
	return err;
}
