cmake_minimum_required(VERSION 3.18)

project(TheEditor LANGUAGES C CXX)

add_subdirectory(vendor/msgpack-c)

add_executable(TheEditor
    src/main.c
    src/direct2d.h
    src/direct2d.cpp
    src/neovim.h
    src/neovim.c
)
set_property(TARGET TheEditor PROPERTY C_STANDARD 99)
set_property(TARGET TheEditor PROPERTY C_STANDARD_REQUIRED ON)
set_property(TARGET TheEditor PROPERTY C_EXTENSIONS OFF)
target_link_libraries(TheEditor PRIVATE d2d1 dwrite msgpack-c-static)
target_compile_definitions(TheEditor PRIVATE UNICODE _UNICODE)
target_compile_options(TheEditor PRIVATE -Wall -Wpedantic -g)
