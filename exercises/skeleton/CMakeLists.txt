# The root build description of a plug-in project in the layout Chapter 26
# describes: one CMakeLists per directory, the root one adding the others.
# Copy this directory, rename the project and the include subdirectory, and
# the three commands in README.md work on day one.
cmake_minimum_required(VERSION 3.21)      # CMakePresets.json version 3 needs 3.21

project(myplugin VERSION 0.1.0 LANGUAGES CXX)

# The standard, stated once (Appendix K): CMake spells -std= and /std: for
# you, and EXTENSIONS OFF asks for C++17 rather than the compiler's dialect.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# compile_commands.json: what clangd, clang-tidy and any check that asks
# "which flags reached this file" read (Chapter 40).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Chapter 26's sanitizer switch, as an INTERFACE target every other target
# links: one place to change when the flag list grows, and a checklist you
# can read - any target not linking it is a target nobody is checking.
option(MYPLUGIN_SANITIZE "Build with Address and UB sanitizers" OFF)
add_library(myplugin_sanitizers INTERFACE)
if(MYPLUGIN_SANITIZE)
    if(MSVC)
        target_compile_options(myplugin_sanitizers INTERFACE /fsanitize=address)
    else()
        target_compile_options(myplugin_sanitizers INTERFACE -fsanitize=address,undefined -g)
        target_link_options(myplugin_sanitizers INTERFACE -fsanitize=address,undefined)
    endif()
endif()

add_subdirectory(src)

# Tests are a second executable with their own main (Chapter 28), so they
# live apart from the library's sources - and they are built by default
# only when this project is the top level. A consumer that vendors it with
# add_subdirectory (Chapter 27's first strategy) gets the library and not
# the tests, for the reason deplab's mathlib guards its install rules:
# reached that way, these lines would run as the consumer's. That consumer
# calls enable_testing() in its own root - the call here registers tests
# for this directory and below, never for a parent.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(MYPLUGIN_IS_TOP_LEVEL ON)
else()
    set(MYPLUGIN_IS_TOP_LEVEL OFF)
endif()
option(MYPLUGIN_BUILD_TESTS "Build the test executable" ${MYPLUGIN_IS_TOP_LEVEL})
if(MYPLUGIN_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
