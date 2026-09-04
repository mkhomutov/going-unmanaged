# The plug-in's build: a MODULE library, an SDK located by a hand-written
# find-module, and a symbol surface of exactly one function. Chapter 40's
# reference; do the chapter's "Try it" from scratch before reading it.
cmake_minimum_required(VERSION 3.16)

project(monitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Match the host's C runtime on Windows before any target exists: a plug-in
# on the debug CRT loaded by a release host corrupts the heap it shares
# (Chapter 26's pitfall). The generator expression picks per configuration.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# The SDK ships no config package, so cmake/FindHostSDK.cmake writes the
# imported target by hand; this line is what makes find_package find it.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(HostSDK REQUIRED)

# MODULE, not SHARED: a library that exists to be loaded at run time by
# name, that nothing links against - so CMake publishes no import library
# on Windows, no soname on Linux, and refuses to let another target link it.
add_library(monitor MODULE monitor.cpp)
target_link_libraries(monitor PRIVATE HostSDK::Core)

set_target_properties(monitor PROPERTIES
    PREFIX ""                            # monitor.so (macOS too) / monitor.dll, not libmonitor
    CXX_VISIBILITY_PRESET hidden         # export nothing by default...
    VISIBILITY_INLINES_HIDDEN ON)        # ...inline bodies included
# ...and monitor_export.h marks the one function that IS exported.

# Hidden visibility covers code THIS project compiles. A static library it
# links brings its own: libhostsdk was built with default visibility, so
# without the line below every hostsdk function is exported from the module
# too - and the host, which links the same library, now has two copies of
# each in one process, and which copy a call reaches is the platform
# loader's decision, not yours (Chapter 27's diamond, delivered by your
# plug-in). Tell the linker the surface is one name. MSVC needs nothing: it
# exports only what wears __declspec(dllexport).
if(APPLE)
    target_link_options(monitor PRIVATE "-Wl,-exported_symbol,_Plugin_Entry")
elseif(NOT MSVC)
    target_link_options(monitor PRIVATE "-Wl,--exclude-libs,ALL")
endif()

if(MSVC)
    target_compile_options(monitor PRIVATE /W4)
else()
    target_compile_options(monitor PRIVATE -Wall -Wextra)
endif()
