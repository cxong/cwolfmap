#include "cwolfmap/cwolfmap.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
	CWolfMap map;
	int err = 0;
	err = CWLoad(&map, "WOLF3D");
	if (err != 0)
	{
		goto bail;
	}

bail:
	CWFree(&map);
	return err;
}
