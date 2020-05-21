#include <stdint.h>

#include "audio.h"
#include "vswap.h"

#define CW_LEVELS 100
#pragma pack(push, 1)
typedef struct
{
	uint16_t magic;
	int32_t ptr[CW_LEVELS];
} CWMapHead;
typedef struct
{
	uint32_t offPlane0;
	uint32_t offPlane1;
	uint32_t offPlane2;
	uint16_t lenPlane0;
	uint16_t lenPlane1;
	uint16_t lenPlane2;
	uint16_t width;
	uint16_t height;
	char name[16];
	char signature[4];
} CWLevelHead;
#pragma pack(pop)

#define NUM_PLANES 3
typedef struct
{
	int len;
	uint16_t *plane;
} CWPlane;
typedef struct
{
	CWLevelHead header;
	CWPlane planes[NUM_PLANES];
} CWLevel;

typedef struct
{
	CWMapHead mapHead;
	CWLevel *levels;
	int nLevels;
	CWAudio audio;
	CWVSwap vswap;
} CWolfMap;

int CWLoad(CWolfMap *map, const char *path);
void CWFree(CWolfMap *map);

uint16_t CWLevelGetTile(
	const CWLevel *level, const int planeIndex, const int x, const int y);
