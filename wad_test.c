#include "cwolfmap/wad/wad.h"


int main(int argc, char *argv[])
{
	(void)argc;
	wad_t *wad = WAD_Open(argv[1]);
	if (wad == NULL)
	{
		goto bail;
	}
	printf("Loaded WAD with %d entries\n", WAD_EntryCount(wad));
	char name[9];
	name[8] = '\0';
	for (int i = 0; i < WAD_EntryCount(wad); i++)
	{
		wadentry_t *entry = WAD_GetEntry(wad, i);
		strncpy(name, (char *)entry->name, 8);
		printf("%d - %s\n", i, name);
	}

bail:
	WAD_Close(wad);
	return 0;
}
