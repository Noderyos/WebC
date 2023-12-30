cmake_minimum_required(VERSION 3.26)
project(WebC C)

set(CMAKE_C_STANDARD 11)

add_executable(WebC main.c
        request.c
        request.h)