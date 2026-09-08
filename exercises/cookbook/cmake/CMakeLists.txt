# Appendix J's entry for a library the SYSTEM provides - Chapter 27's fourth
# strategy in CMake spelling, three ways, one per cookbook probe:
#
#   find_package(SQLite3)     a module CMake ships (FindSQLite3.cmake)
#   find_package(OpenSSL)     another (FindOpenSSL.cmake): OpenSSL::Crypto
#   pkg_check_modules(...)    FindPkgConfig, for a library CMake has no
#                             module for - libcurl here, though it has one,
#                             so the three spellings can be compared on
#                             libraries that exist on every machine
#
# The consumer's link line is the same shape for all three: an imported
# target, carrying its include directory and its library, from a find
# step that fails at CONFIGURE time with the library's name (REQUIRED).
# What build_all.sh does with pkg-config in bash, this file does in CMake.
# Built and run by build_all.sh under the cmake probe and the three
# library probes together; each recipe's own harness is its judge.
cmake_minimum_required(VERSION 3.16)

project(cookbook_system_libraries LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COOKBOOK ${CMAKE_CURRENT_SOURCE_DIR}/..)

# 1. A module CMake ships: find_package(SQLite3) defines SQLite::SQLite3.
find_package(SQLite3 REQUIRED)
add_executable(cb_database ${COOKBOOK}/database.cpp)
target_link_libraries(cb_database PRIVATE SQLite::SQLite3)

# 2. Another shipped module: find_package(OpenSSL) defines OpenSSL::Crypto
#    (and OpenSSL::SSL, which this recipe does not need).
find_package(OpenSSL REQUIRED)
add_executable(cb_crypto ${COOKBOOK}/crypto.cpp)
target_link_libraries(cb_crypto PRIVATE OpenSSL::Crypto)

# 3. FindPkgConfig: for a library CMake has no module for, pkg-config's
#    metadata becomes an imported target - PkgConfig::CURL - with one call.
#    -isystem exercises/third_party is Recipe 46's vendored JSON, Chapter
#    27's first strategy sitting beside its fourth in one target.
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED IMPORTED_TARGET libcurl)
add_executable(cb_http ${COOKBOOK}/http.cpp)
target_link_libraries(cb_http PRIVATE PkgConfig::CURL)
target_include_directories(cb_http SYSTEM PRIVATE ${COOKBOOK}/../third_party)

foreach(t cb_database cb_crypto cb_http)
    if(MSVC)
        target_compile_options(${t} PRIVATE /W4)
    else()
        target_compile_options(${t} PRIVATE -Wall -Wextra)
    endif()
endforeach()

enable_testing()
add_test(NAME cb_database COMMAND cb_database)
add_test(NAME cb_crypto COMMAND cb_crypto)
add_test(NAME cb_http COMMAND cb_http)
