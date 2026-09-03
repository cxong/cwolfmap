cmake_minimum_required(VERSION 3.19)
project(wwiser C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(wwiser wwiser.c wwiser.h hashmap.h)

add_subdirectory(ww2ogg)

add_executable(wwiser_test main.c)
target_include_directories(wwiser_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/ww2ogg)
target_link_libraries(wwiser_test PRIVATE wwiser ww2ogg)
