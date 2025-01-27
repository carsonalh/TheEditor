cmake_minimum_required(VERSION 3.18)

project(TheEditor)

set(CMAKE_CXX_STANDARD 23)

add_executable(TheEditor
    src/Application.hpp
    src/Application.cpp
    src/FileTree.hpp
    src/FileTree.cpp
    src/FileTreeComponent.hpp
    src/FileTreeComponent.cpp
    src/LinearMath.hpp
    src/main.cpp
    src/TheEditor.hpp
)

find_package(Python REQUIRED)
execute_process(
    COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}\\scripts\\find_asan_dir.py"
    RESULT_VARIABLE ASAN_RESULT
    OUTPUT_VARIABLE ASAN_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(NOT (${ASAN_RESULT} EQUAL 0))
    message(FATAL_ERROR "Could not find ASAN DLL directory in Visual Studio toolchain installation")
endif()
find_file(
    ASAN_RUNTIME clang_rt.asan_dynamic-x86_64.dll
    PATHS "${ASAN_DIR}"
)

# TODO this should have a generator expression to only run in debug, all current attempts at this have failed
add_custom_command(
    TARGET TheEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ASAN_RUNTIME}" $<TARGET_FILE_DIR:TheEditor>
    VERBATIM
)

target_compile_definitions(
    TheEditor PRIVATE
    _CRT_SECURE_NO_WARNINGS
    UNICODE _UNICODE
)
target_compile_options(TheEditor PRIVATE
    $<$<CONFIG:Debug>:/Zi /W4 /fsanitize=address /external:anglebrackets /external:W0 /wd4100>
    $<$<CONFIG:Release>:/W4 /wd4100>
)
target_link_libraries(TheEditor PRIVATE user32 d2d1 dwrite)