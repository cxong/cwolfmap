#pragma once
#include "audio.h"

int CWAudioWolf2LoadAudio(CWAudio *audio, const char *path);
int CWAudioWolf2GetMusic(
	CWAudio *audio, const int idx, char **data, size_t *len);
int CWAudioWolf2GetLevelMusic(const int level);
int CWAudioWolf2GetSong(const CWSongType song);