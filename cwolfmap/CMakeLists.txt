set(CW_SOURCES audio.c cwolfmap.c expand.c)
set(CWHEADERS audio.h cwolfmap.h expand.h)

add_library(cwolfmap STATIC ${CW_SOURCES} ${CWHEADERS})
