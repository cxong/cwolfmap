#pragma once

#include "common.h"

// TODO:
// - map wolfstone sound names to sound ids
// - load wwise sound files

int CWWolf2LoadResources(
	const char *path, Resource *mapHead, Resource *mapData, Resource *vswap);