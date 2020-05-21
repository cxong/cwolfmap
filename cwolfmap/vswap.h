#include <stdint.h>

typedef struct
{
	uint16_t chunkCount;
	uint16_t firstSprite;
	uint16_t firstSound;
} CWVSwapHead;

typedef struct
{
	CWVSwapHead head;
	uint32_t *chunkOffset;
	uint16_t *chunkLength;
} CWVSwap;

int CWVSwapLoad(CWVSwap *vswap, const char *path);
void CWVSwapFree(CWVSwap *vswap);
