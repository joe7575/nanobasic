cmake_minimum_required(VERSION 3.0.0)
project(tinybasic VERSION 0.1.0 LANGUAGES C)

#include_directories(./core)

add_executable(tinybasic
    ./TINYscanner.c
    ./TINYcompiler.c
    ./TINYruntime.c
    ./TINYmemory.c
    ./main.c
    ./TINY.h
    ./TINYint.h
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
