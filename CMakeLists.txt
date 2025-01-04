cmake_minimum_required(VERSION 3.18)

project(TheEditor)

add_subdirectory(vendor/glfw)
add_subdirectory(vendor/freetype)
add_library(glad vendor/glad/src/gl.c)
target_include_directories(glad PUBLIC vendor/glad/include)

add_executable(TheEditor main.c layout.c util.c text.c theeditor.h)
target_link_libraries(TheEditor PRIVATE glfw user32 freetype glad)
target_include_directories(TheEditor PRIVATE vendor/glfw/include)