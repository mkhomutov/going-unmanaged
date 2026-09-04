// hostsdk.h - a host application's plug-in contract, in the shape a vendor
// drop arrives in: one C header, a function table the host hands every
// plug-in, one entry point the plug-in must export, and a small helper
// library (hostsdk) with NO CMake config package. Locating it is the
// consumer's job - see ../../plugin/cmake/FindHostSDK.cmake.
//
// Vendor code for Chapter 40: read it, link it, never edit it.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HOSTSDK_API_VERSION 3

// What the host hands a plug-in on load. The size field is Chapter 30's
// versioning device: a plug-in built against an older header sees a smaller
// struct and can say so, instead of reading past the end of it.
//
// LIFETIME: the table is valid only for the duration of the Plugin_Entry
// call. Copy what you keep; never store the pointer.
// THREADS: every function in the table may be called only from the thread
// Plugin_Entry was called on. The host sets every slot the size covers.
typedef struct HostApi {
    uint32_t size;                          // sizeof(HostApi), set by the host
    uint32_t api_version;                   // the host's HOSTSDK_API_VERSION
    void (*log)(const char* line);          // the host's log - the plug-in's only voice
} HostApi;

// The ONE symbol a plug-in exports, by this exact name. Returns 0 on success;
// nothing escapes it (Chapter 30's rule), because the frame above is the host's.
#define HOSTSDK_ENTRY_NAME "Plugin_Entry"
typedef int32_t (*PluginEntryFn)(const HostApi* host);

// From the helper library, libhostsdk: the SDK's own version string.
const char* HostSdk_VersionString(void);

#ifdef __cplusplus
}
#endif
