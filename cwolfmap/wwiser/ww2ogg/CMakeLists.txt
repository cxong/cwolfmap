cmake_minimum_required(VERSION 3.31.6)
project(ww2ogg C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(ww2ogg Bit_stream.h Bit_stream.c c_array.c c_array.h codebook.c codebook.h crc.c crc.h errors.h wwriff.c wwriff.h packed_codebooks_aoTuV_603.c packed_codebooks_aoTuV_603.h)

add_executable(ww2ogg_test main.c)
target_link_libraries(ww2ogg_test ww2ogg)
