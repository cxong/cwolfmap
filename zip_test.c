#include "cwolfmap/zip/zip.h"

#include <stdlib.h>
#include <stdio.h>


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
	char *line = strtok(buf, "\n");
	while (line)
	{
		printf("%s\n", line);
		line = strtok(NULL, "\n");
	}


bail:
	zip_entry_close(zip);
	zip_close(zip);
	free(buf);
	return 0;
}
