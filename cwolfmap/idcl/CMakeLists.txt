cmake_minimum_required(VERSION 3.19)
project(idcl C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(idcl idcl.c idcl.h)

add_subdirectory(kraken)

add_executable(idcl_test main.c)
target_include_directories(idcl_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/kraken)
target_link_libraries(idcl_test PRIVATE idcl kraken)
