#include "audio.h"

#include <stdio.h>
#include <stdlib.h>

#define PATH_MAX 4096

int CWAudioLoadHead(CWAudioHead *head, const char *path)
{
	int err = 0;
	FILE *f = fopen(path, "rb");
	if (!f)
	{
		err = -1;
		fprintf(stderr, "Failed to read %s", path);
		goto bail;
	}
	fseek(f, 0, SEEK_END);
	const long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	head->nOffsets = fsize / sizeof(uint32_t);
	head->offsets = malloc(head->nOffsets * sizeof(uint32_t));
	if (fread(head->offsets, sizeof(uint32_t), head->nOffsets, f) !=
		head->nOffsets)
	{
		err = -1;
		fprintf(stderr, "Failed to read audio head");
		goto bail;
	}

bail:
	if (f)
	{
		fclose(f);
	}
	return err;
}

void CWAudioHeadFree(CWAudioHead *head)
{
	free(head->offsets);
}

int CWAudioLoadAudioT(const char *path)
{
	int err = 0;
	FILE *f = fopen(path, "rb");
	if (!f)
	{
		err = -1;
		fprintf(stderr, "Failed to read %s", path);
		goto bail;
	}

bail:
	if (f)
	{
		fclose(f);
	}
	return err;
}
