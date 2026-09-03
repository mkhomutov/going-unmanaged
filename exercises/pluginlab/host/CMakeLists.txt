# The stand-in host: an executable that links the SDK's helper library and
# loads a plug-in by path. It borrows the plug-in's find-module, because
# locating the SDK is the same job from either side of the boundary.
cmake_minimum_required(VERSION 3.16)
project(host LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../plugin/cmake")
find_package(HostSDK REQUIRED)

add_executable(host host.cpp)
target_link_libraries(host PRIVATE HostSDK::Core ${CMAKE_DL_LIBS})
if(MSVC)
    target_compile_options(host PRIVATE /W4)
else()
    target_compile_options(host PRIVATE -Wall -Wextra)
endif()
