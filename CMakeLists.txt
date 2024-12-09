cmake_minimum_required(VERSION 3.0.0)
project(tinybasic VERSION 0.1.0 LANGUAGES C)

#include_directories(./core)
add_compile_options(-Wall)

add_executable(nanobasic
    ./nb_scanner.c
    ./nb_compiler.c
    ./nb_runtime.c
    ./nb_memory.c
    ./main.c
    ./nb.h
    ./nb_int.h
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
