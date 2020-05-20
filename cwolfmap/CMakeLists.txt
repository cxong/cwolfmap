set(CW_SOURCES audio.c cwolfmap.c expand.c)
set(CWHEADERS AUDIOWL1.H audiowl6.h audio.h cwolfmap.h expand.h)

add_library(cwolfmap STATIC ${CW_SOURCES} ${CWHEADERS})
