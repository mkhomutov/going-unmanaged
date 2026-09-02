# Path 2 - FETCHED. The build acquires the source itself at configure time.
# Chapter 27's second strategy.

cmake_minimum_required(VERSION 3.16)
project(app_fetched LANGUAGES CXX)

include(FetchContent)

# A file:// URL keeps this offline and is a real git remote as far as git is
# concerned - the mechanism is identical to a https:// one, so nothing here is
# a simulation. build_all.sh points these at a throwaway repository it builds
# from ../mathlib; doing it by hand is Chapter 27's step 3.
set(MATHLIB_REPO "" CACHE STRING "file:// URL of the mathlib repository")
set(MATHLIB_TAG "v1.0.0" CACHE STRING "the tag to pin - a TAG, never a branch")

if(NOT MATHLIB_REPO)
    message(FATAL_ERROR
        "Configure with -DMATHLIB_REPO=file:///path/to/repo -DMATHLIB_TAG=v1.0.0")
endif()

FetchContent_Declare(mathlib
    GIT_REPOSITORY ${MATHLIB_REPO}
    GIT_TAG        ${MATHLIB_TAG})   # the pin: a tag or a commit, never a branch

# Clones (once, into the build tree) and add_subdirectory()s the result, so
# what arrives is the same mathlib::mathlib target as the vendored path. That
# is why the app source and the link line below are byte-identical to path 1.
FetchContent_MakeAvailable(mathlib)

add_executable(app ../app/main.cpp)
target_link_libraries(app PRIVATE mathlib::mathlib)
