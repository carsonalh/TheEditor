cmake_minimum_required(VERSION 3.18)

project(TheEditor)

add_subdirectory(vendor/glfw)
add_subdirectory(vendor/freetype)
add_library(glad vendor/glad/src/gl.c)
target_include_directories(glad PUBLIC vendor/glad/include)

add_executable(TheEditor
    src/main.c
    src/layout.c
    src/util.c
    src/text.c
    src/render.c
    src/ui.c
    src/filetree.c
    src/theeditor.h "src/linmath.h")
set_target_properties(TheEditor PROPERTIES COMPILE_FLAGS -g -Wall -Werror)
target_link_libraries(TheEditor PRIVATE glfw user32 freetype glad)
target_include_directories(TheEditor PRIVATE vendor/glfw/include)