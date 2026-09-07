# The library: the code worth reusing, testing and shipping. Sources are
# LISTED, never globbed (Chapter 26).
add_library(myplugin session.cpp)
add_library(myplugin::myplugin ALIAS myplugin)

# PUBLIC: consumers include <myplugin/session.h>, so the include directory is
# part of the interface. Private headers stay in src/ and are never listed here.
target_include_directories(myplugin PUBLIC ${PROJECT_SOURCE_DIR}/include)

# PRIVATE: warnings are this library's opinion, not its consumers'.
if(MSVC)
    target_compile_options(myplugin PRIVATE /W4)
else()
    target_compile_options(myplugin PRIVATE -Wall -Wextra)
endif()

target_link_libraries(myplugin PUBLIC myplugin_sanitizers)
