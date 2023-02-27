#pragma once
#include <stddef.h>
#include <stdint.h>
#include "wad/wad.h"

typedef enum
{
	CWMAPTYPE_WL1,
	CWMAPTYPE_WL6,
	CWMAPTYPE_SOD,
	CWMAPTYPE_BS1,
	CWMAPTYPE_BS6,
	CWMAPTYPE_N3D,
	CWMAPTYPE_UNKNOWN,
} CWMapType;

typedef struct
{
	uint32_t *offsets;
	size_t nOffsets;
} CWAudioHead;

typedef struct
{
	CWAudioHead head;
	int nSound;
	int nMusic;
	int startAdlibSounds;
	int startMusic;
	char *data;
	wad_t *wad;
} CWAudio;
