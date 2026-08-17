/*
** Adapted from file_idcl.cpp
**
**---------------------------------------------------------------------------
** Copyright 2019 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
** A not so serious Wolfenstein II resource file loader.  All we care about is
** getting the Wolfstone files out of it, so don't expect this code to be
** complete!
**
*/
#include "idcl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

// From answer by plinth
// http://stackoverflow.com/a/744822/2038264
// License: http://creativecommons.org/licenses/by-sa/3.0/
// Author profile: http://stackoverflow.com/users/20481/plinth
bool StrEndsWith(const char* str, const char* suffix)
{
	if (str == NULL || suffix == NULL)
	{
		return false;
	}
	const size_t lenStr = strlen(str);
	const size_t lenSuffix = strlen(suffix);
	if (lenSuffix > lenStr)
	{
		return false;
	}
	return strncmp(str + lenStr - lenSuffix, suffix, lenSuffix) == 0;
}

#pragma pack(push, 1)
typedef struct
{
	char magic[4];
	uint32_t version;
	char pad[32];
	uint32_t files;
	char pad2[4];
	uint32_t ids;
	char pad3[20];
	uint64_t stringTableOffset;
	char pad4[8];
	uint64_t dirOffset;
	char pad5[8];
	uint64_t idOffset;
	uint64_t dataOffset;
}IDCLHeader;

typedef struct
{
	char pad[32];
	uint64_t name;
	char pad2[16];
	uint64_t offset;
	uint64_t compressedSize;
	uint64_t size;
	char pad3[24];
	uint32_t typeIndicator;
	char pad4[36];
}IDCLEntry;

typedef struct
{
	uint64_t type;
	uint64_t name;
}IDCLStringRef;
#pragma pack(pop)

int LoadWolf2Lumps(const char* filename, FileLump** lumps, int* numLumps)
{
	FILE* file = fopen(filename, "rb");
	if (!file) {
		printf("Error: Could not open file %s\n", filename);
		return 1; // Error opening file
	}
	IDCLHeader header;
	fread(&header, sizeof(IDCLHeader), 1, file);
	// Check file fields
	if (strncmp(header.magic, "IDCL", 4) != 0 || header.version != 12) {
		printf("Error: Invalid IDCL file\n");
		fclose(file);
		return 1; // Invalid file
	}

	// Read string table directory
	uint64_t numStringTableEntries;
	fseek(file, (long)header.stringTableOffset, SEEK_SET);
	fread(&numStringTableEntries, sizeof(numStringTableEntries), 1, file);
	uint64_t* stringOffsets = (uint64_t*)malloc(numStringTableEntries * sizeof(uint64_t));
	fread(stringOffsets, sizeof(uint64_t), numStringTableEntries, file);

	// Read string table
	uint64_t strBufLen = 0;
	for (uint64_t i = 0; i < numStringTableEntries; ++i)
		strBufLen = MAX(strBufLen, stringOffsets[i]);
	strBufLen += 1024; // Read enough extra to hopefully get the last string
	char* stringTableBuffer = malloc(strBufLen);
	fread(stringTableBuffer, 1, strBufLen, file);

	char** stringTable = (char**)malloc(numStringTableEntries * sizeof(char*));
	for (uint64_t i = 0; i < numStringTableEntries; ++i)
		stringTable[i] = &stringTableBuffer[stringOffsets[i]];

	// Read file ids
	IDCLStringRef* fileIds = (IDCLStringRef*)malloc(header.files * sizeof(IDCLStringRef));
	fseek(file, (long)header.idOffset + header.ids * 4, SEEK_SET);
	fread(fileIds, sizeof(IDCLStringRef), header.files, file);

	// Read directory
	IDCLEntry* dirEntries = (IDCLEntry*)malloc(header.files * sizeof(IDCLEntry));
	fseek(file, (long)header.dirOffset, SEEK_SET);
	fread(dirEntries, sizeof(IDCLEntry), header.files, file);

	// Loop over file lumps and find the ones we want (WL6 etc.)
	*lumps = (FileLump*)malloc((header.files + 1) * sizeof(FileLump));
	*numLumps = 0;
	for (uint64_t i = 0; i < header.files; ++i)
	{
		bool encrypted = false;

		if (dirEntries[i].compressedSize == dirEntries[i].size)
		{
			switch (dirEntries[i].typeIndicator)
			{
				// Wolfenstein II language files have an indicator of 1 and are
				// encrypted and oodle compressed. Youngblood language files are
				// 3 and are not encrypted but still compressed.
			case 1:
			{
				encrypted = true;

				fseek(file, (long)dirEntries[i].offset, SEEK_SET);
				uint32_t size;
				fread(&size, sizeof(size), 1, file);

				dirEntries[i].offset += 4;
				dirEntries[i].compressedSize -= 4;
				dirEntries[i].size = size;
				break;
			}

			case 3:
			{
				fseek(file, (long)dirEntries[i].offset, SEEK_SET);
				uint32_t size, csize;
				fread(&size, sizeof(size), 1, file);
				fread(&csize, sizeof(csize), 1, file);
				dirEntries[i].offset += 8;
				dirEntries[i].compressedSize = csize;
				dirEntries[i].size = size;
				break;
			}

			// Seemingly uncompressed entry could be compressed if typeIndicator
			// is 4, but the sizes will be in the header of the data.
			case 4:
				fseek(file, (long)dirEntries[i].offset, SEEK_SET);
				fread(&dirEntries[i].size, sizeof(dirEntries[i].size), 1, file);
				fread(&dirEntries[i].compressedSize, sizeof(dirEntries[i].compressedSize), 1, file);
				dirEntries[i].offset += 16;

				// compressedSize of -1 means uncompressed
				if ((int64_t)dirEntries[i].compressedSize == -1)
					dirEntries[i].compressedSize = dirEntries[i].size;
				break;
			}
		}

		const char* name = stringTable[fileIds[i].name];
		if (StrEndsWith(name, ".wl6") ||
			StrEndsWith(name, "sb_wolfstone.bnk") ||
			StrEndsWith(name, "sb_vo_wolfstone.bnk")
			// StrEndsWith(name, ".wem") ||  TODO: don't need to look at wem files, only for Youngblood
			)
		{
			FileLump* lump = &(*lumps)[(*numLumps)++];

			// Get rid of the path since we're only loading the embedded Wolf3D data
			const char* slash = strrchr(name, '/');
			const char* realName = slash == NULL ? name : slash + 1;
			if (strncmp(name, "strings/", 8) == 0)
				realName = name;
			strncpy(lump->name, realName, sizeof(lump->name) - 1);
			lump->size = dirEntries[i].size;
			lump->compressedSize = dirEntries[i].compressedSize;
			lump->data = malloc(lump->compressedSize);
			fseek(file, (long)dirEntries[i].offset, SEEK_SET);
			fread(lump->data, 1, lump->compressedSize, file);
			lump->position = dirEntries[i].offset;
			lump->encrypted = encrypted;
			printf("Found lump: %s (size: %llu, compressed size: %llu, encrypted: %d)\n", lump->name, lump->size, lump->compressedSize, lump->encrypted);
		}
	}

	printf("%d lumps\n", *numLumps);

	fclose(file);
	free(stringOffsets);
	free(stringTableBuffer);
	free(fileIds);
	free(dirEntries);

	return 0;
}
