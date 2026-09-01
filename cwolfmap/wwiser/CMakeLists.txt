cmake_minimum_required(VERSION 3.19)
project(wwiser C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(wwiser wwiser.c wwiser.h hashmap.h)

add_executable(wwiser_test main.c)
target_link_libraries(wwiser_test PRIVATE wwiser)
