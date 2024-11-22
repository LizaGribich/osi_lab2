cmake_minimum_required(VERSION 3.26)
project(osi_lab2 C)

set(CMAKE_C_STANDARD 11)

add_library(lab2 SHARED lab2.c)

add_executable(test test.c)
target_link_libraries(test lab2)

add_executable(create-file create-file.c)
add_executable(io-thpt-read io-thpt-read.c)

add_executable(io-thpt-read-new io-thpt-read-new.c)
target_link_libraries(io-thpt-read-new lab2)

