cmake_minimum_required(VERSION 3.12)
project(cwolfmap)

add_subdirectory(cwolfmap)

add_executable(cwolfmap_test cwolfmap_test.c rlutil.h)
target_link_libraries(cwolfmap_test cwolfmap)

add_executable(audio_extract audio_extract.c)
target_link_libraries(audio_extract cwolfmap)

add_executable(music_extract music_extract.c)
target_link_libraries(music_extract cwolfmap)

add_executable(vswap_sound_extract vswap_sound_extract.c)
target_link_libraries(vswap_sound_extract cwolfmap)
