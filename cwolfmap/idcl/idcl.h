#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
	char name[256];
	void* data;
	uint64_t size;
	uint64_t compressedSize;
	uint64_t position;
	bool encrypted;
} FileLump;

int LoadWolf2Lumps(const char* filename, FileLump** lumps, int* numLumps);