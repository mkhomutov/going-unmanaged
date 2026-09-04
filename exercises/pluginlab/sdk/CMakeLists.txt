# The vendor drop's own build: a static helper library and a header, installed
# to a prefix - and deliberately NO export set and NO config package. Chapter
# 27's deplab shows the SDK that ships CMake support; this is the other kind,
# which is most of them, and the reason plugin/cmake/FindHostSDK.cmake exists.
cmake_minimum_required(VERSION 3.16)
project(hostsdk VERSION 3.0.0 LANGUAGES CXX)

add_library(hostsdk STATIC src/hostsdk.cpp)
target_compile_features(hostsdk PUBLIC cxx_std_17)
include(GNUInstallDirs)
# BUILD_INTERFACE only: with no export set there is no install interface to
# describe (compare deplab's mathlib, which has both because it exports).
target_include_directories(hostsdk PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# What a drop looks like on disk after `cmake --install`: <prefix>/include/hostsdk/hostsdk.h
# and <prefix>/lib/libhostsdk.a (lib64/ on some Linux distributions;
# hostsdk.lib on Windows). Nothing else.
install(TARGETS hostsdk)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
