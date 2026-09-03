// monitor.cpp - the plug-in. One exported entry point, everything else hidden.
#include <hostsdk/hostsdk.h>
#include "monitor_export.h"

#include <string>

namespace {
// Internal linkage AND hidden visibility: no symbol of ours leaks into the
// host's process, so a same-named function in the host - or in another
// plug-in - can never be the one the loader picks (Chapter 27's diamond, at
// plug-in scale).
std::string Describe(const HostApi& host) {
    return std::string("monitor loaded against ") + HostSdk_VersionString()
         + ", host api " + std::to_string(host.api_version);
}
}  // namespace

extern "C" MONITOR_EXPORT int32_t Plugin_Entry(const HostApi* host) {
    if (host == nullptr || host->size < sizeof(HostApi)) {
        return -1;                          // an older host: do not read past what it gave us
    }
    if (host->api_version != HOSTSDK_API_VERSION) {
        return -2;
    }
    try {
        host->log(Describe(*host).c_str());
        return 0;
    } catch (...) {
        return -3;                          // nothing escapes into the host's frames
    }
}
