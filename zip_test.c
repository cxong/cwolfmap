#include "cwolfmap/zip/zip.h"

#include <stdlib.h>
#include <stdio.h>

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif


int main(int argc, char *argv[])
{
	char *buf = NULL; 
	struct zip_t *zip = NULL;
	if (argc != 3)
	{
		printf("Usage: zip_test <zip_file> <entry_filename>\n");
		goto bail;
	}
	zip = zip_open(argv[1], 0, 'r');
	if (zip == NULL)
	{
		goto bail;
	}
	zip_entry_open(zip, argv[2]);
	const size_t bufsize = zip_entry_size(zip);
	buf = malloc(bufsize);
	zip_entry_noallocread(zip, (void *)buf, bufsize);
	printf("Loaded zip entry %s with size %d\n", argv[2], (int)bufsize);

	// Read entry
	char *start = buf;
	char linebuf[1024];
	while (start)
	{
		long long len = MIN(1024, strlen(start));
		char *nl = strchr(start, '\n');
		if (nl != NULL)
		{
			len = MIN(len, nl - start);
		}
		strncpy(linebuf, start, 1024);
		linebuf[len] = '\0';
		printf("%s\n", linebuf);
		if (nl == NULL)
		{
			break;
		}
		start = nl + 1;
	}


bail:
	zip_entry_close(zip);
	zip_close(zip);
	free(buf);
	return 0;
}
