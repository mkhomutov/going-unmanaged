# The dependency: a standalone project, buildable on its own.
#
# It has to be standalone or two of the three consumption paths cannot work -
# FetchContent clones and configures it as a project, and the install/export
# half below is what makes find_package(mathlib) possible at all.

cmake_minimum_required(VERSION 3.16)

# The VERSION here is the thing step 3 watches move. It reaches the code as a
# compile definition, not as a header the consumer parses.
project(mathlib VERSION 1.0.0 LANGUAGES CXX)

add_library(mathlib src/mathlib.cpp)

# The namespaced alias. Consumers link mathlib::mathlib whichever way they got
# it, so switching between the three paths does not touch their CMakeLists -
# and a typo becomes a configure-time error instead of a linker guess, because
# CMake knows a name with :: in it must be a target.
add_library(mathlib::mathlib ALIAS mathlib)

target_compile_features(mathlib PUBLIC cxx_std_17)

# PUBLIC: consumers compile against this header, so the include directory is
# part of the interface. The two generator expressions are the same directory
# seen from two places - the source tree while building here, the install tree
# after install(). Hardcode the source path and the installed package points at
# a directory that exists only on the machine that built it.
target_include_directories(mathlib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

# PRIVATE: compiles this library and stops there (Chapter 26's distinction).
target_compile_definitions(mathlib PRIVATE MATHLIB_VERSION="${PROJECT_VERSION}")
if(MSVC)
    target_compile_options(mathlib PRIVATE /W4)
else()
    target_compile_options(mathlib PRIVATE -Wall -Wextra)
endif()

# --- the install/export half -------------------------------------------------
# Everything below exists so that find_package(mathlib CONFIG REQUIRED) works
# in another project. This is the "if it ships one" in Chapter 27's
# `find_package(VendorSDK REQUIRED)  # a config package, if it ships one`:
# a config package is a thing the library AUTHOR generates and installs, and
# this is what generating one looks like.

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# EXPORT records the target in a set; the install(EXPORT) below writes that set
# out as importable CMake code.
install(TARGETS mathlib EXPORT mathlibTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT mathlibTargets
    FILE mathlibTargets.cmake
    NAMESPACE mathlib::          # so the consumer's name matches the alias above
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mathlib)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/mathlibConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/mathlibConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mathlib)

# SameMajorVersion: find_package(mathlib 1.0 REQUIRED) accepts 1.1, refuses 2.0.
# The policy is the author's to choose and the consumer's to rely on.
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/mathlibConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/mathlibConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/mathlibConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mathlib)
