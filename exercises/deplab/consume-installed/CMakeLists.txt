# Path 3 - INSTALLED, found as a config package. This is the shape a vendor
# SDK arrives in when it ships CMake support, and the consuming half of
# Chapter 27's `find_package(VendorSDK REQUIRED)  # a config package, if it
# ships one`. The producing half is the install/export block in
# ../mathlib/CMakeLists.txt.

cmake_minimum_required(VERSION 3.16)
project(app_installed LANGUAGES CXX)

# CONFIG: load the package's own mathlibConfig.cmake rather than letting CMake
# hunt with a Find module. REQUIRED: fail at CONFIGURE time with a clear
# message, not at link time with a missing symbol - Chapter 27's pitfall about
# assuming the build machine's environment.
#
# Nothing here says where it is installed. build_all.sh passes
# -DCMAKE_PREFIX_PATH=<the install tree>, which is how a consumer points at an
# SDK without any path being written into the project.
find_package(mathlib CONFIG REQUIRED)

add_executable(app ../app/main.cpp)

# Identical to both other paths. The target came from an installed file this
# time, and the app's build description cannot tell.
target_link_libraries(app PRIVATE mathlib::mathlib)
