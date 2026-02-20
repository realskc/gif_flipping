cmake_minimum_required(VERSION 3.16)

project(gifdump)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(gifdump
    src/gifdump.cpp
    src/gif_parse.cpp
    src/gif_reverse.cpp
)
target_compile_options(gifdump PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wimplicit-fallthrough
    -Wmisleading-indentation
    -Werror
    -Wno-unused-parameter
)