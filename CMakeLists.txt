cmake_minimum_required(VERSION 3.19)
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
set(CMAKE_OSX_ARCHITECTURES x86_64)

find_package(SDL2 REQUIRED)
message("SDL2 include dir: ${SDL2_INCLUDE_DIRS}")
find_package(SDL2_mixer REQUIRED)
message("SDL2_mixer include dir: ${SDL2_MIXER_INCLUDE_DIR}")
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_MIXER_INCLUDE_DIR})

add_subdirectory(cwolfmap)

add_executable(cwolfmap_test cwolfmap_test.c rlutil.h)
target_link_libraries(cwolfmap_test cwolfmap)

add_executable(audio_play audio_play.c rlutil.h)
target_link_libraries(audio_play cwolfmap SDL2::SDL2 SDL2_mixer::SDL2_mixer)

add_executable(music_extract music_extract.c)
target_link_libraries(music_extract cwolfmap)

add_executable(music_play music_play.c)
target_link_libraries(music_play cwolfmap SDL2::SDL2 SDL2_mixer::SDL2_mixer)

add_executable(vswap_sound_extract vswap_sound_extract.c)
target_link_libraries(vswap_sound_extract cwolfmap SDL2::SDL2 SDL2_mixer::SDL2_mixer)

add_executable(vswap_sound_play vswap_sound_play.c rlutil.h)
target_link_libraries(vswap_sound_play cwolfmap SDL2::SDL2 SDL2_mixer::SDL2_mixer)

add_executable(wad_test wad_test.c)
target_link_libraries(wad_test wad SDL2::SDL2 SDL2_mixer::SDL2_mixer)

add_executable(zip_test zip_test.c)
target_link_libraries(zip_test zip)
