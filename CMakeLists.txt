cmake_minimum_required(VERSION 3.12)
project(cwolfmap)

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
	set(CLANG 1)
endif()
if(MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -MP -W4 -WX -wd\"4204\" -wd\"4267\" -wd\"4996\"")
else()
	add_definitions(
		-fsigned-char
		-Wall -W
		-Wstrict-prototypes -Wpointer-arith -Wcast-qual)
    if(CLANG)
		add_definitions(-std=gnu99)
    else()
        # Probably GCC...
        add_definitions(-freg-struct-return -std=gnu99)
		if(NOT APPLE)
			add_definitions(-Wshadow)
		endif()
	endif()
	add_definitions(-Winline -Werror)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_MIXER_INCLUDE_DIRS})

add_subdirectory(cwolfmap)

add_executable(cwolfmap_test cwolfmap_test.c rlutil.h)
target_link_libraries(cwolfmap_test cwolfmap)

add_executable(audio_extract audio_extract.c)
target_link_libraries(audio_extract cwolfmap)

add_executable(music_extract music_extract.c)
target_link_libraries(music_extract cwolfmap)

add_executable(music_play music_play.c)
target_link_libraries(music_play cwolfmap ${SDL2_LIBRARY} ${SDL2_MIXER_LIBRARIES})

add_executable(vswap_sound_extract vswap_sound_extract.c)
target_link_libraries(vswap_sound_extract cwolfmap ${SDL2_LIBRARY} ${SDL2_MIXER_LIBRARIES})

add_executable(vswap_sound_play vswap_sound_play.c rlutil.h)
target_link_libraries(vswap_sound_play cwolfmap ${SDL2_LIBRARY} ${SDL2_MIXER_LIBRARIES})
