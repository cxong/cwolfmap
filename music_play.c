#include "cwolfmap/cwolfmap.h"
#include "cwolfmap/mame/fmopl.h"
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

#pragma pack(push, 1)
typedef struct
{
	uint8_t mChar, cChar, mScale, cScale, mAttack, cAttack, mSus, cSus, mWave,
		cWave, nConn,

		// These are only for Muse - these bytes are really unused
		voice, mode;
	uint8_t unused[3];
} Instrument;

typedef struct
{
	uint32_t length;
	uint16_t priority;
	Instrument inst;
	byte block;
	byte data[1];
} AdLibSound;
#pragma pack(pop)

/*static void SDL_ALPlaySound(AdLibSound *sound)
{
	alLengthLeft = sound->length;
	alBlock = ((sound->block & 7) << 2) | 0x20;

	SDL_AlSetFXInst(&sound->inst);
	alSound = (byte *)sound->data;
}*/

int main(int argc, char *argv[])
{
	CWolfMap map;
	Sound *sounds = NULL;
	int nSounds = 0;
	int err = 0;
	int oplChip = 0;
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

	// Init music
	if (YM3812Init(1, 3579545, SAMPLE_RATE))
	{
		printf("S_Init: Unable to create virtual OPL!!\n");
	}

	int volume = 20;
	for (int i = 1; i < 0xf6; i++)
		YM3812Write(oplChip, i, 0, &volume);

	YM3812Write(oplChip, 1, 0x20, &volume); // Set WSE=1

	for (int i = 0; i < map.audio.nMusic; i++)
	{
		const char *data;
		size_t len;
		err = CWAudioGetMusic(&map.audio, i, &data, &len);
		if (err != 0)
		{
			goto bail;
		}
		if (len == 88)
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
		sprintf(sound->name, "MUS%05d", i);
		const AdLibSound *als = (const AdLibSound *)data;
		printf(
			"Loaded adlib sound %d (%d len, %d data length)\n", i, als->length,
			(int)len);
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
		Mix_PlayChannel(-1, sounds[cmd].snd, 0);
	}

bail:
	CWFree(&map);
	for (int i = 0; i < nSounds; i++)
	{
		Mix_FreeChunk(sounds[i].snd);
	}
	YM3812Shutdown();
	Mix_CloseAudio();
	free(sounds);
	return err;
}
