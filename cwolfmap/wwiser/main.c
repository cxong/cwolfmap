#include <stdio.h>
#include <stdlib.h>

#include "wwiser.h"

int wemCallback(const WWiseSound* ws)
{
	// Write out the file for now
	FILE *out = fopen(ws->filename, "wb");
	if (out == NULL)
	{
		printf("Failed to open output file for sound\n");
		return 1;
	}
	fwrite(ws->data, 1, ws->length, out);
	fclose(out);
	return 0;
}

int main(int argc, char *argv[])
{
	char *data = NULL;
	if (argc != 2)
	{
		printf("Usage: wwiser_test <testfile.bnk>");
		return 1;
	}
	// Test files: sb_wolfstone.bnk, sb_vo_wolfstone.bnk

	FILE *f = fopen(argv[1], "rb");
	fseek(f, 0, SEEK_END);
	const long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	data = malloc(len);
	fread(data, 1, len, f);
	if (WWiseLoadSoundbank(data, len, wemCallback) != 0)
	{
		goto bail;
	}

bail:
	free(data);
	fclose(f);
	return 0;
}
