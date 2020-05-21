#include <stdint.h>

typedef struct
{
	uint32_t *offsets;
	int nOffsets;
} CWAudioHead;

typedef struct
{
	CWAudioHead head;
	int nMusic;
	char *data;
} CWAudio;

int CWAudioLoadHead(CWAudioHead *head, const char *path);
void CWAudioHeadFree(CWAudioHead *head);

int CWAudioLoadAudioT(CWAudio *audio, const char *path);

void CWAudioFree(CWAudio *audio);

// http://www.vgmpf.com/Wiki/index.php?title=IMF
int CWAudioGetMusic(
	const CWAudio *audio, const int i, const char **data, int *len);
