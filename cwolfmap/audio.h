#include <stdint.h>

typedef struct
{
	uint32_t *offsets;
	int nOffsets;
} CWAudioHead;

int CWAudioLoadHead(CWAudioHead *head, const char *path);
void CWAudioHeadFree(CWAudioHead *head);

int CWAudioLoadAudioT(const char *path);
