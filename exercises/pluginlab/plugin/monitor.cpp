// monitor.cpp - the plug-in. One exported entry point, everything else hidden.
#include <hostsdk/hostsdk.h>
#include "monitor_export.h"

#include <string>
#include <type_traits>

// External linkage on purpose: this is the function CXX_VISIBILITY_PRESET
// hidden exists for. Remove the preset and Describe appears in the export
// table, where a same-named function in the host or another plug-in could
// collide with it (Chapter 27's diamond, at plug-in scale). Hidden, it is
// ours alone.
std::string Describe(const HostApi& host) {
    return std::string("monitor loaded against ") + HostSdk_VersionString()
         + ", host api " + std::to_string(host.api_version);
}

extern "C" MONITOR_EXPORT int32_t Plugin_Entry(const HostApi* host) {
    if (host == nullptr || host->size < sizeof(HostApi)) {
        return -1;                          // an older host: do not read past what it gave us
    }
    if (host->api_version != HOSTSDK_API_VERSION) {
        return -2;
    }
    if (host->log == nullptr) {
        return -1;                          // a slot the host left empty is not ours to call
    }
    try {
        host->log(Describe(*host).c_str());
        return 0;
    } catch (...) {
        return -3;                          // nothing escapes into the host's frames
    }
}

// The header publishes the entry point as a name and a typedef, never a
// prototype - so nothing above compared this definition to the contract.
// This line does (Chapter 39's "written twice, compared by nothing", closed).
static_assert(std::is_same<decltype(&Plugin_Entry), PluginEntryFn>::value,
              "Plugin_Entry must match the SDK's PluginEntryFn");
