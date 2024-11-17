cmake_minimum_required(VERSION 3.26)
project(osi_lab2 C)

set(CMAKE_C_STANDARD 11)

add_library(lab2_cache SHARED lab2.c)

add_executable(test test.c)
target_link_libraries(test lab2_cache)
