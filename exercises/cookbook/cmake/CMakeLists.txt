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
# --8<-- [start:system-libraries]
cmake_minimum_required(VERSION 3.18)      # an ALIAS of an imported target needs 3.18

project(cookbook_system_libraries LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(COOKBOOK ${CMAKE_CURRENT_SOURCE_DIR}/..)

# 1. A module CMake ships: find_package(SQLite3) defines a target - named
#    SQLite::SQLite3 until CMake 4.3, SQLite3::SQLite3 from there, the old
#    name kept but deprecated. The module's own advice for a project that
#    must configure under both is this alias, and it is the price the
#    catalogue names: the target's spelling is the machine's CMake's.
find_package(SQLite3 REQUIRED)
if(NOT TARGET SQLite3::SQLite3)
    add_library(SQLite3::SQLite3 ALIAS SQLite::SQLite3)
endif()
add_executable(cb_database ${COOKBOOK}/database.cpp)
target_link_libraries(cb_database PRIVATE SQLite3::SQLite3)

# 2. Another shipped module: find_package(OpenSSL) defines OpenSSL::Crypto
#    (and OpenSSL::SSL, which this recipe does not need).
find_package(OpenSSL REQUIRED)
add_executable(cb_crypto ${COOKBOOK}/crypto.cpp)
target_link_libraries(cb_crypto PRIVATE OpenSSL::Crypto)

# 3. FindPkgConfig: for a library CMake has no module for, pkg-config's
#    metadata becomes an imported target - PkgConfig::CURL - with one call.
#    libcurl has a module too (FindCURL, CURL::libcurl); it is spelled this
#    way here so the third mechanism is shown on a library every machine has.
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
# --8<-- [end:system-libraries]
