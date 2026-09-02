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

# Both are CACHE variables, and that changes how you re-point the pin. The
# value written above is a DEFAULT: it is read on the first configure of a
# build directory and ignored on every configure after it, because a cache
# entry that already exists is not overwritten. So editing the line above and
# re-running cmake in the same build directory does nothing at all - the app
# goes on reporting the version you started with, with no message to say why.
#
# Step 3 says "re-point GIT_TAG and watch the build follow it". Here that
# means overriding the cache from the command line:
#
#     cmake -S . -B build -DMATHLIB_REPO=file:///path/to/repo -DMATHLIB_TAG=v1.1.0
#
# (or deleting the build directory, which is what build_all.sh does - it
# configures a fresh tree per tag). In your own version, where GIT_TAG is
# probably a literal in FetchContent_Declare rather than a cache variable,
# editing the file IS the gesture. The difference is the cache, not FetchContent.

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
