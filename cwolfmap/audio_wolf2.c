#include "audio_wolf2.h"

#include <stdlib.h>

#include "audiowolf2.h"
#include "idcl/idcl.h"
#include "revorb.h"
#include "wwiser/ww2ogg/packed_codebooks_aoTuV_603.h"
#include "wwiser/ww2ogg/wwriff.h"
#include "wwiser/wwiser.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

void CWAudioWolf2LoadAudioT(CWAudio *audio)
{
	audio->nSound = 0;
	audio->nMusic = LASTMUSIC;
	audio->startAdlibSounds = 0;
	audio->startMusic = 0;
}

static const musicnames songs[] = {
	//
	// Episode One
	//
	GETTHEM_MUS, SEARCHN_MUS, POW_MUS, SUSPENSE_MUS, GETTHEM_MUS, SEARCHN_MUS,
	POW_MUS, SUSPENSE_MUS,

	VICMARCH_MUS, // Boss level
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

static int wemCallback(const WWiseSound *ws, void *data);

int CWAudioWolf2LoadAudio(CWAudio *audio, const char *path)
{
	// Just save the path
	free(audio->path);
	audio->path = strdup(path);
	return 0;
}

typedef struct
{
	const char *target;
	char **data;
	size_t *len;
} WEMCallbackData;

static void FreeLumps(FileLump *lumps, const int numLumps)
{
	if (lumps)
	{
		for (int i = 0; i < numLumps; ++i)
		{
			const FileLump *lump = &lumps[i];
			free(lump->data);
		}
	}
	free(lumps);
}

static const char *WEM_MUSIC[] = {
	"CORNER.wem",	"DUNGEON.wem",	"GETTHEM.wem",	"HEADACHE.wem",
	"HITLWLTZ.wem", "POW.wem",		"SALUTE.wem",	"SEARCHN.wem",
	"SUSPENSE.wem", "VICTORS.wem",	"WONDERIN.wem", "FUNKYOU.wem",
	"ENDLEVEL.wem", "GOINGAFT.wem", "PREGNANT.wem", "ULTIMATE.wem",
	"NAZI_RAP.wem", "ZEROHOUR.wem", "TWELFTH.wem",	"ROSTER.wem",
	"URAHERO.wem",	"VICMARCH.wem", "PACMAN.wem"};

int CWAudioWolf2GetMusic(
	CWAudio *audio, const int idx, char **data, size_t *len)
{
	if (idx < 0 || idx >= LASTMUSIC)
	{
		return 1;
	}
	int err = 0;
	char pathBuf[PATH_MAX];
	FileLump *lumps = NULL;
	int numLumps;
	WEMCallbackData wData;
	wData.target = WEM_MUSIC[idx];
	wData.data = data;
	wData.len = len;

	// 1. Read soundbanks via idcl
	// sound/soundbanks/pc/sound.pack contains sb_wolfstone.bnk
	snprintf(
		pathBuf, sizeof(pathBuf), "%s/base/sound/soundbanks/pc/sound.pack",
		audio->path);
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
			int found = WWiseLoadSoundbank(
				lump->data, lump->size, wemCallback, &wData);
			if (found < 0)
			{
				fprintf(stderr, "Failed to read soundbank %s\n", pathBuf);
				err = -1;
				goto bail;
			}
			if (found == 1)
			{
				goto bail;
			}
		}
	}
	FreeLumps(lumps, numLumps);
	lumps = NULL;
	numLumps = 0;

	// patch_1_english(us).pack contains sb_vo_wolfstone.bnk (need to read
	// english(us).pack and patch_\d_english(us).pack)
	for (int i = 0;; i++)
	{
		if (i == 0)
		{
			snprintf(
				pathBuf, sizeof(pathBuf),
				"%s/base/sound/soundbanks/pc/english(us).pack", audio->path);
		}
		else
		{

			snprintf(
				pathBuf, sizeof(pathBuf),
				"%s/base/sound/soundbanks/pc/patch_%d_english(us).pack",
				audio->path, i);
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
				int found = WWiseLoadSoundbank(
					lump->data, lump->size, wemCallback, &wData);
				if (found < 0)
				{
					fprintf(stderr, "Failed to read soundbank %s\n", pathBuf);
					err = -1;
					goto bail;
				}
			}
		}
		FreeLumps(lumps, numLumps);
		lumps = NULL;
		numLumps = 0;
	}

bail:
	FreeLumps(lumps, numLumps);
	return err;
}

// 2. Convert soundbanks to wem using wwiser
static int wemCallback(const WWiseSound *ws, void *data)
{
	char *generated_stream = NULL;
	// 3. Convert wem to ogg using ww2ogg
	int err = 0;
	WEMCallbackData *wData = data;
	if (strcmp(ws->filename, wData->target) == 0)
	{
		Wwise_RIFF_Vorbis decoder;
		err = Wwise_RIFF_Vorbis_init(
			&decoder, (char *)ws->data, ws->length, packed_codebooks_aoTuV_603,
			sizeof(packed_codebooks_aoTuV_603), false, false,
			kNoForcePacketFormat);
		if (err != 0)
		{
			fprintf(stderr, "Failed to read headers: %d\n", err);
			err = -1;
			goto bail;
		}

		long stream_length;
		generated_stream =
			Wwise_RIFF_Vorbis_generate_ogg(&decoder, &stream_length);
		if (!generated_stream)
		{
			fprintf(stderr, "Failed to generate Ogg stream\n");
			err = -1;
			goto bail;
		}

		// 4. Revorb so SDL can play it
		char *data_out = NULL;
		size_t size_out = 0;
		if (!revorb(generated_stream, stream_length, &data_out, &size_out))
		{
			fprintf(stderr, "Failed to revorb ogg stream\n");
			err = -1;
			goto bail;
		}

		// 5. Apply gain to music
		// TODO: SFX?
		*wData->len = size_out;
		*wData->data = data_out;
		err = 1; // found
	}

bail:
	free(generated_stream);
	return err;
}
