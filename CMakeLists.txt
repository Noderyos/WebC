cmake_minimum_required(VERSION 3.26)
project(WebC C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_executable(WebC src/main.c src/http.c src/base64.c src/sha1.c src/websocket.c)

target_link_libraries(WebC z)