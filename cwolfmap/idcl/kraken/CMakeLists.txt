cmake_minimum_required(VERSION 3.19)
project(kraken C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(kraken kraken.c kraken.h)

add_executable(kraken_test main.c)
target_link_libraries(kraken_test kraken)
