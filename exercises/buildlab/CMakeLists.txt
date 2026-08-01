# Chapter 26's reference CMakeLists for the Build-Model Lab trio.
#
# Do the chapter's "Try it" from scratch before reading this: writing your own
# is the exercise, and this file is only the answer. It is assembled from the
# chapter's own snippets, in the shape the chapter ends with - library plus
# executable, warnings PRIVATE, sanitizers behind an option and carried by an
# INTERFACE target. scripts/build_all.sh configures, builds and runs it both
# ways, which is what keeps THIS shape honest; the earlier forms the chapter
# walks through are in no file and nothing checks them.

cmake_minimum_required(VERSION 3.16)

project(greeter LANGUAGES CXX)

# The standard IS abstracted by CMake - it knows each compiler's spelling
# (-std=c++17 vs /std:c++17), so you state the intent once.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # fail loudly rather than silently downgrade
set(CMAKE_CXX_EXTENSIONS OFF)         # plain C++17, not the compiler's dialect of it

# The library: the code worth reusing, testing, and shipping.
# Sources are LISTED, never globbed - see the chapter's pitfalls.
add_library(greeter Greeter.cpp)

# PUBLIC: anything linking greeter also needs this on its include path,
# because Greeter.h is part of the library's *interface*.
target_include_directories(greeter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# PRIVATE: these compile the library itself and stop there - consumers have
# their own opinion about warnings and shouldn't inherit mine.
if(MSVC)
    target_compile_options(greeter PRIVATE /W4)
else()
    target_compile_options(greeter PRIVATE -Wall -Wextra)
endif()

add_executable(greet main.cpp)
target_link_libraries(greet PRIVATE greeter)   # include dirs arrive automatically

# The handbook's canonical sanitizer build as a switch rather than a memory:
#
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DGREETER_SANITIZE=ON
#
# The flags live in a target of their own rather than on greeter, so growing
# the project cannot quietly shrink the coverage: any target not linking
# `sanitizers` is a target nobody is checking.
option(GREETER_SANITIZE "Build with Address and UB sanitizers" OFF)

add_library(sanitizers INTERFACE)   # no sources; the flags ARE the target

if(GREETER_SANITIZE)
    if(MSVC)
        # MSVC ships ASan only - there is no /fsanitize=undefined - and its
        # linker pulls in the runtime by itself, so no link options here.
        target_compile_options(sanitizers INTERFACE /fsanitize=address)
    else()
        # Both the compiler AND the linker: instrumenting the code without
        # linking the runtime is the chapter's ___asan_init trap.
        target_compile_options(sanitizers INTERFACE -fsanitize=address,undefined -g)
        target_link_options(sanitizers INTERFACE -fsanitize=address,undefined)
    endif()
endif()

target_link_libraries(greeter PUBLIC sanitizers)   # and every other target too
