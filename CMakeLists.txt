cmake_minimum_required(VERSION 3.12)
project(cwolfmap)

add_subdirectory(cwolfmap)

add_executable(cwolfmap_test cwolfmap_test.c)
target_link_libraries(cwolfmap_test cwolfmap)

add_executable(audio_extract audio_extract.c)
target_link_libraries(audio_extract cwolfmap)
