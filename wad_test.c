#include "cwolfmap/wad/wad.h"

#include <stdlib.h>
#include <SDL_mixer.h>
#include "rlutil.h"

#define SAMPLE_RATE 44100
#define AUDIO_FMT AUDIO_S16
#define AUDIO_CHANNELS 2


int main(int argc, char *argv[])
{
	(void)argc;
	unsigned char *buf = NULL; 
	wad_t *wad = WAD_Open(argv[1]);
	Mix_Music *mus = NULL;
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

	// Read one entry
	wadentry_t *e1 = WAD_GetEntry(wad, 0);
	buf = malloc(e1->length);
	if (WAD_GetEntryData(wad, e1, buf) < 0)
	{
		printf("Failed to read entry\n");
		goto bail;
	}
	if (Mix_OpenAudio(SAMPLE_RATE, AUDIO_FMT, AUDIO_CHANNELS, 1024) != 0)
	{
		goto bail;
	}
	SDL_RWops *rwops = SDL_RWFromMem(buf, e1->length);
	mus = Mix_LoadMUS_RW(rwops, 1);
	Mix_PlayMusic(mus, 0);
	getch();


bail:
	Mix_FreeMusic(mus);
	WAD_Close(wad);
	free(buf);
	Mix_CloseAudio();
	return 0;
}
