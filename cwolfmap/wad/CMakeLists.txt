set(WAD_SOURCES wad.c waderrno.c)
set(WAD_HEADERS wad.h wad_config.h waderrno.h)

if(MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -MP -W4 -WX -wd\"4057\" -wd\"4100\" -wd\"4273\" -wd\"4706\"")
endif()

add_library(wad STATIC ${WAD_SOURCES} ${WAD_HEADERS})
