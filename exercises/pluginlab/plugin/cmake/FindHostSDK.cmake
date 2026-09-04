# FindHostSDK.cmake - the imported target, written by hand.
#
# The SDK ships a header and a library and no CMake config package, so the
# consumer builds the target a config package would have generated: locate
# the two files under CMAKE_PREFIX_PATH (or HostSDK_ROOT), and present them
# as HostSDK::Core, carrying its include directory as usage requirements.
# After this file, the rest of the project links HostSDK::Core exactly as it
# would link an SDK that had done this work itself.
find_path(HostSDK_INCLUDE_DIR hostsdk/hostsdk.h)
find_library(HostSDK_LIBRARY NAMES hostsdk)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HostSDK
    REQUIRED_VARS HostSDK_LIBRARY HostSDK_INCLUDE_DIR)

if(HostSDK_FOUND AND NOT TARGET HostSDK::Core)
    # UNKNOWN: static or shared, CMake need not know - the file is the file.
    add_library(HostSDK::Core UNKNOWN IMPORTED)
    set_target_properties(HostSDK::Core PROPERTIES
        IMPORTED_LOCATION "${HostSDK_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${HostSDK_INCLUDE_DIR}")
endif()

mark_as_advanced(HostSDK_INCLUDE_DIR HostSDK_LIBRARY)
