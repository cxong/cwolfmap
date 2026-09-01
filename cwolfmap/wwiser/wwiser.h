#include <stdint.h>

typedef struct
{
	char filename[256];
	uint8_t *data;
	uint32_t length;
} WWiseSound;

int WWiseLoadSoundbank(
	const char *data, const size_t len, void (*callback)(const WWiseSound *));
