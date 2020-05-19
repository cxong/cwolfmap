set(CW_SOURCES cwolfmap.c expand.c)
set(CWHEADERS cwolfmap.h expand.h)

add_library(cwolfmap STATIC ${CW_SOURCES} ${CWHEADERS})
