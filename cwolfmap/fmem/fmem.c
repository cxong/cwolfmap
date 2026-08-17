// Simplified backend selection
#if defined(_WIN32) && !defined(__CYGWIN__)
#include "fmem-winapi-tmpfile.c"

#elif defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) ||   \
	defined(__NetBSD__)
#define _GNU_SOURCE
#include "alloc.c"
#include "fmem-funopen.c"

#elif defined(__linux__)
#define _GNU_SOURCE
#include "fmem-open_memstream.c"

#else
/* Fallback for anything else */
#include "fmem-tmpfile.c"
#endif
