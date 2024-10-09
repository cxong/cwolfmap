cmake_minimum_required(VERSION 3.19)
project(cwolfmap)

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
	set(CLANG 1)
endif()
if(MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -MP -W4 -WX -wd\"4090\" -wd\"4996\" -wd\"4204\"")
	if(DEBUG)
		add_definitions(-ZI)
	endif()
else()
	add_definitions(
		-fsigned-char
		-Wall -W
		-Wstrict-prototypes -Wpointer-arith -Wcast-qual)
	if(DEBUG)
		add_definitions(-g)
		if(DEBUG_PROFILE AND CMAKE_COMPILER_IS_GNUCC)
			add_definitions(-p)
		endif()
	else()
		add_definitions(-O2)
	endif()
	if(GCW0)
		add_definitions(-D__GCWZERO__)
	endif()
    if(CLANG)
		add_definitions(-std=gnu99)
    else()
        # Probably GCC...
        add_definitions(-freg-struct-return -std=gnu99)
		if(NOT APPLE AND NOT BEOS AND NOT HAIKU)
			add_definitions(-Wshadow)
		endif()
		if (CMAKE_C_COMPILER_VERSION VERSION_GREATER 7)
			add_definitions(
				-C
				-Wno-error=format-overflow # TODO: should probably just make sure buffer sizes are appropriate
			)
		endif()
	endif()
endif()
set(CMAKE_OSX_ARCHITECTURES x86_64)

find_package(SDL2 REQUIRED)
message("SDL2 include dir: ${SDL2_INCLUDE_DIRS}")
find_package(SDL2_mixer REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_subdirectory(cwolfmap)

add_executable(cwolfmap_test cwolfmap_test.c rlutil.h)
target_link_libraries(cwolfmap_test cwolfmap)
if(APPLE)
	set_target_properties(cwolfmap_test PROPERTIES
		MACOSX_RPATH 1
		BUILD_WITH_INSTALL_RPATH 1
		INSTALL_RPATH "@loader_path/../Frameworks;/Library/Frameworks")
endif()

add_executable(audio_play audio_play.c rlutil.h)
target_link_libraries(audio_play cwolfmap SDL2::SDL2 SDL2_mixer::SDL2_mixer)

add_executable(music_extract music_extract.c)
target_link_libraries(music_extract cwolfmap)

add_executable(music_play music_play.c)
target_link_libraries(music_play cwolfmap SDL2::SDL2 SDL2_mixer::SDL2_mixer)

add_executable(n3d_quiz_test n3d_quiz_test.c rlutil.h)
target_link_libraries(n3d_quiz_test cwolfmap)

add_executable(vswap_sound_extract vswap_sound_extract.c)
target_link_libraries(vswap_sound_extract cwolfmap SDL2::SDL2 SDL2_mixer::SDL2_mixer)

add_executable(vswap_sound_play vswap_sound_play.c rlutil.h)
target_link_libraries(vswap_sound_play cwolfmap SDL2::SDL2 SDL2_mixer::SDL2_mixer)

add_executable(wad_test wad_test.c)
target_link_libraries(wad_test wad SDL2::SDL2 SDL2_mixer::SDL2_mixer)

add_executable(zip_test zip_test.c)
target_link_libraries(zip_test zip)
