cmake_minimum_required(VERSION 4.2)
project(malloc_project)

set(CMAKE_CXX_STANDARD 17)

add_executable(malloc_project my_malloc.cpp
        my_malloc.h
        tests.cpp)
        # main.cpp)
