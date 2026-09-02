# Path 1 - VENDORED. The library's source is in your tree and your build
# descends into it. Chapter 27's first strategy, and by a distance the most
# common thing in native SDK work.

cmake_minimum_required(VERSION 3.16)
project(app_vendored LANGUAGES CXX)

# The whole strategy, in one call. The second argument is the binary directory:
# needed only because the source sits outside this directory, which is an
# artifact of this lab keeping one copy of mathlib for all three paths. A real
# vendored dependency lives under third_party/ inside your project and needs
# no second argument.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../mathlib
                 ${CMAKE_CURRENT_BINARY_DIR}/mathlib)

add_executable(app ../app/main.cpp)

# Note what is NOT here: no include_directories, no path to mathlib/include,
# no ../mathlib anywhere except the add_subdirectory above. The include path
# rides in on the link, because mathlib declared it PUBLIC. Chapter 27's step 2
# asks you to confirm exactly this, and build_all.sh asserts it.
target_link_libraries(app PRIVATE mathlib::mathlib)
