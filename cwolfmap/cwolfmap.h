#include <stdint.h>

#define CW_LEVELS 100
#pragma pack(push, 1)
typedef struct
{
	uint16_t magic;
	int32_t mapPtrs[CW_LEVELS];
} CWMapHead;
#pragma pack(pop)
typedef struct
{
	CWMapHead mapHead;
} CWolfMap;

int CWLoad(CWolfMap *map, const char *path);
void CWFree(CWolfMap *map);
