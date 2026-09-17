#include "wwiser.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "hashmap.h"

// Based on WolfstoneExtract https://github.com/ECWolfEngine/WolfstoneExtract
// GPL3

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#pragma pack(push, 1)

typedef struct
{
	uint32_t Id;
	uint32_t Length;
} FSbSection;

#define FSB_HEADER_MAGIC 0x44484B42 // BHKD

typedef struct
{
	uint32_t NumObjects;
} FSbHirc;

typedef struct
{
	uint8_t Type; // Sound = 2
	uint32_t Length;
	uint32_t Id;
} FSbHircObject;

// Not a fixed size structure
typedef struct
{
	char Unknown[4];
	uint8_t Included;
	uint32_t FileId;
	uint32_t SourceId;
	union {
		struct Embedded
		{
			uint32_t Offset;
			uint32_t Length;
			uint8_t IsVoice;
		} embed;
		struct Streamed
		{
			uint8_t IsVoice;
		} stream;
	} u;
} HircSound;

// DIDX
typedef struct
{
	uint32_t WemId;
	uint32_t Offset;
	uint32_t Length;
} FSbIndex;

#pragma pack(pop)

// TODO: used for both input and output, should be split into separate
// structures
typedef struct
{
	uint64_t Offset;
	uint32_t Length;
	uint32_t Id;
	uint8_t *Data;
} Entry;

typedef struct
{
	uint32_t Id;
	const char *Name;
} SoundName;

SoundName WolfstoneSoundNames[] = {
	{0x00A7A4A8u, "DSSWITCH"},
	{0x011D4829u, "DSMCHSTP"},
	{0x015F0AC5u, "DSDOGDTH"},
	{0x017BD4A2u, "DSMUTDTH"},
	{0x02E7D2B6u, "DSENDBN2"},
	{0x0310F041u, "DSFAKSIT"},
	{0x03247F84u, "DSWALK1"},
	{0x03CA7301u, "NAZI_RAP"},
	{0x03F3872Cu, "DSPLDETH"},
	{0x057E0D67u, "DSHITWAL"},
	{0x05987A19u, "DSGRDFIR"},
	{0x05BA1401u, "GETTHEM"},
	{0x06A65B2Au, "DSGMOVER"},
	{0x06AEF84Cu, "DSWALK2"},
	{0x082A773Fu, "HEADACHE"},
	{0x084EC4ECu, "DSSHTDOR"},
	{0x08B07D1Cu, "DSDROPN"},
	{0x0910C9CEu, "DSMVGUN1"},
	{0x09C2AFBAu, "VICTORS"},
	{0x0A011B88u, "DSGDDTH5"},
	{0x0A1A4A9Du, "DSGOOB"},
	{0x0A3A2104u, "DSSSSIT"},
	{0x0B5B0E79u, "DSDOGATK"},
	{0x0C0E9104u, "DSSLCTWN"},
	{0x0C0E9F24u, "DSNOITEM"},
	{0x0C48B24Du, "SUSPENSE"},
	{0x0C50AA50u, "ROSTER"},
	{0x0D6ACAA5u, "SEARCHN"},
	{0x0D83D8B5u, "DSFOODUP"},
	{0x0DDB7AE3u, "DSHANSIT"},
	{0x0E0894DFu, "TWELFTH"},
	{0x0E0A7172u, "DSGRDSIT"},
	{0x0E0B6F1Au, "DSFAKDTH"},
	{0x0F3798CDu, "DSNAZPAI"},
	{0x0FA36143u, "DSRLAUNC"},
	{0x10B7A13Eu, "DSGDDTH4"},
	{0x10FE10ACu, "DSHITSHI"},
	{0x11225254u, "HITLWLTZ"},
	{0x11242DC5u, "DSFATSIT"},
	{0x1167D12Cu, "DSPISTOL"},
	{0x11BEF1F3u, "ENDLEVEL"},
	{0x13708AD1u, "DSMGUN"},
	{0x13BE4844u, "DSFATDTH"},
	{0x13D5899Fu, "DSGDDTH1"},
	{0x13F3E3EBu, "DSFART"},
	{0x1463D500u, "DSDRCLS"},
	{0x14FE3846u, "DSMVGUN2"},
	{0x152B095Fu, "DSENDBN1"},
	{0x15F2D059u, "DSSELECT"},
	{0x160EE504u, "DSSCBDTH"},
	{0x167ED112u, "DSCGUNUP"},
	{0x17D122ADu, "URAHERO"},
	{0x18020AD9u, "DSHANDTH"},
	{0x18A2C485u, "PACMAN"},
	{0x19CE1EB9u, "DSBNS1UP"},
	{0x1A5EBDD3u, "DSPSHWAL"},
	{0x1A6ADEEBu, "DSBOSSFR"},
	{0x1AE7B296u, "DSBONUS4"},
	{0x1B1D7751u, "DSGDDTH7"},
	{0x1B5090CFu, "DSESCPRS"},
	{0x1B66113Eu, "DSKNFSWG"},
	{0x1B8BF20Bu, "DSGDDTH6"},
	{0x1BEF5B1Du, "DSAMMOUP"},
	{0x1EE17085u, "DSOFFDTH"},
	{0x214D99C0u, "DSGETKEY"},
	{0x21A6CFDEu, "DSPRC100"},
	{0x21DA0A8Au, "DUNGEON"},
	{0x222D92DAu, "DSSSDTH"},
	{0x22F3B414u, "SALUTE"},
	{0x23AB1329u, "ZEROHOUR"},
	// Not sure on this one since there are many very similar sounds in Wolf3D.
	// However, it doesn't matter much since this sound is unused.
	{0x23AE4001u, "DSBOSSIT"},
	{0x2479AADBu, "DSSCBATK"},
	{0x24D870DFu, "DSMGUNUP"},
	{0x24F7A979u, "DSBONUS3"},
	{0x27067F95u, "DSGRTDTH"},
	{0x27370765u, "DSHARTBT"},
	{0x2761FCCBu, "DSPLPAIN"},
	{0x28BB8C3Du, "DSOTOSIT"},
	{0x295945DEu, "CORNER"},
	{0x2B9C7F9Fu, "FUNKYOU"},
	{0x2BDCCC72u, "DSSCBSIT"},
	{0x2BFE167Au, "DSGDDTH2"},
	{0x2C1F5DF3u, "DSMEDIUP"},
	{0x2D077554u, "DSOFFSIT"},
	{0x2E631A44u, "ULTIMATE"},
	{0x2EFC91F9u, "DSSSFIRE"},
	{0x2F9F663Fu, "PREGNANT"},
	{0x306646D8u, "DSOTODTH"},
	// This seems to be an extra sound. Not that it matters much since even
	// BOSSIT isn't used. Judging by length this is one of non-Hans boss
	// sounds.
	{0x310FB57Bu, "DSBOSSI2"},
	{0x31A2B52Fu, "POW"},
	{0x31F30118u, "DSNOWAY"},
	// Copy of DSGDDTH2 (although bytes differ bitstream is same). Probably
	// here to complete the adlib sound table although it's never used.
	{0x3223E805u, "DSGDDTH3"},
	{0x34962C4Au, "DSBONUS2"},
	{0x34E37DC5u, "VICMARCH"},
	{0x34FAC876u, "DSNAZIHT"},
	{0x3570E2F0u, "DSGRTSIT"},
	{0x3613AA1Du, "DSCGUN"},
	{0x36AFEDDFu, "WONDERIN"},
	{0x370A956Fu, "DSGDDTH8"},
	{0x3893F45Fu, "DSFAKFIR"},
	{0x39475108u, "DSSLCTIT"},
	{0x39F47F3Eu, "DSSLURPE"},
	{0x3A63EC71u, "DSHITSIT"},
	{0x3B04E074u, "DSBONUS1"},
	{0x3C3E5E47u, "DSNOTIN"},
	{0x3D93D4EFu, "DSBAREXP"},
	{0x3D9BF301u, "DSDOGSIT"},
	{0x3DAEE1EAu, "GOINGAFT"},
	{0x3DB6F9F4u, "DSYEAH"},
	{0x3DEBD92Du, "DSNOBNS"},
	{0x3ED3FE9Fu, "DSHITDTH"}};

int WWiseLoadSoundbank(
	const char *data, const size_t len,
	int (*callback)(const WWiseSound *, void *), void *callbackData)
{
	int err = 0;
	uint64_t offset = 8;
	bool foundFourCC = false;
	// Look for four sections: BHKD, DATA, HIRC and DIDX
	Entry bkhdEntry, dataEntry, hircEntry, didxEntry;
	memset(&bkhdEntry, 0, sizeof(Entry));
	memset(&dataEntry, 0, sizeof(Entry));
	memset(&hircEntry, 0, sizeof(Entry));
	memset(&didxEntry, 0, sizeof(Entry));
	const char *ptr = data;
	while (ptr <= data + len - sizeof(FSbSection))
	{
		const FSbSection *section = (const FSbSection *)ptr;
		ptr += sizeof(*section);
		if (!foundFourCC && section->Id != FSB_HEADER_MAGIC)
		{
			fprintf(stderr, "Header not found\n");
			err = -1;
			break;
		}
		foundFourCC = true;

		Entry *entry = NULL;
		if (section->Id == FSB_HEADER_MAGIC)
		{
			entry = &bkhdEntry;
		}
		else if (section->Id == 0x41544144) // DATA
		{
			entry = &dataEntry;
		}
		else if (section->Id == 0x43524948) // HIRC
		{
			entry = &hircEntry;
		}
		else if (section->Id == 0x58444944) // DIDX
		{
			entry = &didxEntry;
		}

		if (entry != NULL)
		{
			entry->Offset = offset;
			entry->Length = section->Length;
			entry->Id = 0;
			entry->Data = NULL;
		}

		offset += section->Length + sizeof(section);
		ptr += section->Length;
	}

	if (bkhdEntry.Length == 0 || dataEntry.Length == 0 ||
		hircEntry.Length == 0 || didxEntry.Length == 0)
	{
		fprintf(stderr, "One or more sections not found\n");
		err = -1;
		goto bail;
	}

	const int numSounds = didxEntry.Length / sizeof(FSbIndex);
	// Map WEM Ids to sound Ids which are constant across languages
	struct hashmap_s wemToSoundId;
	if (0 != hashmap_create(numSounds, &wemToSoundId))
	{
		fprintf(stderr, "Failed to create hashmap\n");
		err = -1;
		goto bail;
	}
	ptr = data + hircEntry.Offset;
	const FSbHirc *hirc = (const FSbHirc *)ptr;
	ptr += sizeof *hirc;
	for (unsigned int i = 0; i < hirc->NumObjects; ++i)
	{
		const FSbHircObject *obj = (const FSbHircObject *)ptr;
		ptr += sizeof *obj;

		// printf("%u: Type = %d, Id = %X, Length = %u\n", i, obj.Type, obj.Id,
		// obj.Length);
		if (obj->Type == 2) // Sound
		{
			const HircSound *sfx = (const HircSound *)ptr;
			size_t nread = MIN(sizeof(sfx), obj->Length - 4);
			ptr += nread;
			// SoundId is obj.Id, mapped to FileId
			char key[256];
			sprintf(key, "%X", sfx->FileId);
			if (0 != hashmap_put(
						 &wemToSoundId, strdup(key),
						 (hashmap_uint32_t)strlen(key),
						 (void *)(intptr_t)obj->Id))
			{
				fprintf(stderr, "Failed to put entry in hashmap\n");
				err = -1;
				goto bail;
			}
			ptr += obj->Length - 4 - nread;
		}
		else
			ptr += obj->Length - 4;
	}

	Entry *sounds = malloc(sizeof(Entry) * numSounds);
	ptr = data + didxEntry.Offset;
	for (int i = 0; i < numSounds; ++i)
	{
		const FSbIndex *index = (const FSbIndex *)ptr;
		ptr += sizeof *index;

		char key[256];
		sprintf(key, "%X", index->WemId);
		void *soundIdPtr =
			hashmap_get(&wemToSoundId, key, (hashmap_uint32_t)strlen(key));
		// Default to using WemId for sound
		uintptr_t soundIdPtrCast = (uintptr_t)soundIdPtr;
		const uint32_t soundId = (soundIdPtr != NULL)
									 ? (const uint32_t)soundIdPtrCast
									 : index->WemId;

		sounds[i].Offset = dataEntry.Offset + index->Offset;
		sounds[i].Length = index->Length;
		sounds[i].Id = soundId;
	}
	hashmap_destroy(&wemToSoundId);

	// Build mapping of sound Ids to names for better output file names
	struct hashmap_s soundIdToName;
	if (0 != hashmap_create(numSounds, &soundIdToName))
	{
		fprintf(stderr, "Failed to create hashmap\n");
		err = -1;
		goto bail;
	}
	for (size_t i = 0;
		 i < sizeof(WolfstoneSoundNames) / sizeof(WolfstoneSoundNames[0]); ++i)
	{
		char key[256];
		sprintf(key, "%X", WolfstoneSoundNames[i].Id);
		if (0 != hashmap_put(
					 &soundIdToName, strdup(key),
					 (hashmap_uint32_t)strlen(key),
					 (void *)WolfstoneSoundNames[i].Name))
		{
			fprintf(stderr, "Failed to put entry in hashmap\n");
			err = -1;
			goto bail;
		}
	}

	for (int i = 0; i < numSounds; ++i)
	{
		Entry *sound = &sounds[i];
		sounds[i].Data = malloc(sound->Length);

		ptr = data + sound->Offset;
		memcpy(sound->Data, ptr, sound->Length);

		WWiseSound ws;
		ws.data = sound->Data;
		ws.length = sound->Length;
		// TODO: convert WEM
		// sound.Data = ConvertWem(sound.Data.data(), sound.Length);
		char key[256];
		sprintf(key, "%X", sound->Id);
		const char *name = (const char *)hashmap_get(
			&soundIdToName, key, (hashmap_uint32_t)strlen(key));
		if (name != NULL)
			snprintf(ws.filename, sizeof(ws.filename), "%s.wem", name);
		else
			snprintf(
				ws.filename, sizeof(ws.filename), "sound_%X.wem", sound->Id);
		err = callback(&ws, callbackData);
		if (err != 0)
		{
			goto bail;
		}
	}

	hashmap_destroy(&soundIdToName);

bail:
	return err;
}
