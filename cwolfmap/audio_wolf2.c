#include "audio_wolf2.h"

#include <stdlib.h>

#include "idcl/idcl.h"
#include "wwiser/ww2ogg/packed_codebooks_aoTuV_603.h"
#include "wwiser/ww2ogg/wwriff.h"
#include "wwiser/wwiser.h"
#include "audiowolf2.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static const musicnames songs[] = {
	//
	// Episode One
	//
	GETTHEM_MUS, SEARCHN_MUS, POW_MUS, SUSPENSE_MUS, GETTHEM_MUS, SEARCHN_MUS,
	POW_MUS, SUSPENSE_MUS,

	VICMARCH_MUS,	// Boss level
	CORNER_MUS,	  // Secret level

	//
	// Episode Two
	//
	VICTORS_MUS, PREGNANT_MUS, GOINGAFT_MUS, HEADACHE_MUS, VICTORS_MUS,
	PREGNANT_MUS, HEADACHE_MUS, GOINGAFT_MUS,

	VICMARCH_MUS, // Boss level
	DUNGEON_MUS,  // Secret level

	//
	// Episode Three
	//
	VICMARCH_MUS, NAZI_RAP_MUS, TWELFTH_MUS, ZEROHOUR_MUS, VICMARCH_MUS,
	NAZI_RAP_MUS, TWELFTH_MUS, ZEROHOUR_MUS,

	ULTIMATE_MUS, // Boss level
	PACMAN_MUS,	  // Secret level

	//
	// Episode Four
	//
	GETTHEM_MUS, SEARCHN_MUS, POW_MUS, SUSPENSE_MUS, GETTHEM_MUS, SEARCHN_MUS,
	POW_MUS, SUSPENSE_MUS,

	VICMARCH_MUS, // Boss level
	CORNER_MUS,	  // Secret level

	//
	// Episode Five
	//
	VICTORS_MUS, PREGNANT_MUS, GOINGAFT_MUS, HEADACHE_MUS, VICTORS_MUS,
	PREGNANT_MUS, HEADACHE_MUS, GOINGAFT_MUS,

	VICMARCH_MUS, // Boss level
	DUNGEON_MUS,  // Secret level

	//
	// Episode Six
	//
	VICMARCH_MUS, NAZI_RAP_MUS, TWELFTH_MUS, ZEROHOUR_MUS, VICMARCH_MUS,
	NAZI_RAP_MUS, TWELFTH_MUS, ZEROHOUR_MUS,

	ULTIMATE_MUS, // Boss level
	FUNKYOU_MUS	  // Secret level
};
int CWAudioWolf2GetLevelMusic(const int level)
{
	return songs[level];
}

int CWAudioWolf2GetSong(const CWSongType song)
{
	switch (song)
	{
	case SONG_INTRO:
		return HITLWLTZ_MUS;
	case SONG_MENU:
		return WONDERIN_MUS;
	case SONG_END:
		return ENDLEVEL_MUS;
	case SONG_ROSTER:
		return ROSTER_MUS;
	case SONG_VICTORY:
		return URAHERO_MUS;
	}
	return -1;
}


static int wemCallback(const WWiseSound *ws);

int CWAudioWolf2LoadAudio(CWAudio *audio, const char *path)
{
	(void)audio;
	int err = 0;
	char pathBuf[PATH_MAX];
	FileLump *lumps;
	int numLumps;

	// 1. Read soundbanks via idcl
	// sound/soundbanks/pc/sound.pack contains sb_wolfstone.bnk
	snprintf(
		pathBuf, sizeof(pathBuf), "%s/base/sound/soundbanks/pc/sound.pack",
		path);
	if (LoadWolf2Lumps(pathBuf, &lumps, &numLumps) != 0)
	{
		fprintf(stderr, "Error loading lumps %s\n", pathBuf);
		err = -1;
		goto bail;
	}
	for (int i = 0; i < numLumps; ++i)
	{
		const FileLump *lump = &lumps[i];
		if (strcmp(lump->name, "sb_wolfstone.bnk") == 0)
		{
			if (WWiseLoadSoundbank(lump->data, lump->size, wemCallback) != 0)
			{
				fprintf(stderr, "Failed to read soundbank %s\n", pathBuf);
				err = -1;
				goto bail;
			}
		}
		free(lump->data);
	}
	free(lumps);
	numLumps = 0;

	// patch_1_english(us).pack contains sb_vo_wolfstone.bnk (need to read
	// english(us).pack and patch_\d_english(us).pack)
	for (int i = 0;; i++)
	{
		if (i == 0)
		{
			snprintf(
				pathBuf, sizeof(pathBuf),
				"%s/base/sound/soundbanks/pc/english(us).pack", path);
		}
		else
		{

			snprintf(
				pathBuf, sizeof(pathBuf),
				"%s/base/sound/soundbanks/pc/patch_%d_english(us).pack", path,
				i);
		}
		if (LoadWolf2Lumps(pathBuf, &lumps, &numLumps) != 0)
		{
			break;
		}
		for (int j = 0; j < numLumps; ++j)
		{
			const FileLump *lump = &lumps[j];
			if (strcmp(lump->name, "sb_vo_wolfstone.bnk") == 0)
			{
				if (WWiseLoadSoundbank(lump->data, lump->size, wemCallback) !=
					0)
				{
					fprintf(stderr, "Failed to read soundbank %s\n", pathBuf);
					err = -1;
					goto bail;
				}
			}
			free(lump->data);
		}
		free(lumps);
		numLumps = 0;
	}

bail:
	return err;
}

int CWAudioWolf2GetMusic(
	CWAudio *audio, const int idx, char **data, size_t *len)
{
	// Map audio name to idx
	// CORNER - 0
	// DUNGEON - 1
	// ENDLEVEL - g
	// FUNKYOU - f
	// GETTHEM - 3
	// GOINGAFT - h
	// HEADACHE - 4
	// HITLWLTZ - 5
	// NAZI_RAP - k
	// PACMAN - q
	// POW - 9
	// PREGNANT - i
	// ROSTER - n
	// SALUTE - a
	// SEARCHN - b
	// SUSPENSE - c
	// TWELFTH - m
	// ULTIMATE - j
	// URAHERO - o
	// VICMARCH - p
	// VICTORS - d
	// WONDERIN - e
	// ZEROHOUR - l
	// Missing: 2 (WARMARCH, Boss level), 6 (INTROCW3), 7 (NAZI_NOR, intro), 8 (NAZI_OMI)
	// WARMARCH - uses VICMARCH instead; WARMARCH is a mashup of anthems including horst wessel lied
	// INTROCW3 - uses VICMARCH instead, contains hidden morse code
	// NAZI_NOR - uses HITLWLTZ instead
	// NAZI_OMI - uses VICTORS instead
	(void)audio;
	(void)idx;
	(void)data;
	(void)len;
	return 0;
}

// 2. Convert soundbanks to wem using wwiser
static int wemCallback(const WWiseSound *ws)
{
	// 3. Convert wem to ogg using ww2ogg
	Wwise_RIFF_Vorbis decoder;
	int err = Wwise_RIFF_Vorbis_init(
		&decoder, (char *)ws->data, ws->length, packed_codebooks_aoTuV_603,
		sizeof(packed_codebooks_aoTuV_603), false, false,
		kNoForcePacketFormat);
	if (err != 0)
	{
		fprintf(stderr, "Failed to read headers: %d\n", err);
		goto bail;
	}

	long stream_length;
	char *generated_stream =
		Wwise_RIFF_Vorbis_generate_ogg(&decoder, &stream_length);
	if (!generated_stream)
	{
		fprintf(stderr, "Failed to generate Ogg stream\n");
		err = 1;
		goto bail;
	}

	// 4. Map idx to ogg entry
	// 5. Apply gain to music
	// TODO: SFX?

bail:
	return err;
}
