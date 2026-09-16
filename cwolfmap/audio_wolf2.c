#include "audio_wolf2.h"

#include <stdlib.h>

#include "idcl/idcl.h"
#include "wwiser/ww2ogg/packed_codebooks_aoTuV_603.h"
#include "wwiser/ww2ogg/wwriff.h"
#include "wwiser/wwiser.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

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
