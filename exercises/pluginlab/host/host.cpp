// A stand-in host: loads a plug-in by path, finds its one entry point, and
// hands it the function table. Every real host does exactly this, with an
// application wrapped around it. Test scaffolding for Chapter 40's lab.
#include <hostsdk/hostsdk.h>

#include <cstdio>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {
void Log(const char* line) { std::printf("[host] %s\n", line); }
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: host <path to plug-in module>\n");
        return 2;
    }
#if defined(_WIN32)
    HMODULE lib = LoadLibraryA(argv[1]);
    if (lib == nullptr) {
        std::fprintf(stderr, "cannot load %s\n", argv[1]);
        return 1;
    }
    auto entry = reinterpret_cast<PluginEntryFn>(GetProcAddress(lib, HOSTSDK_ENTRY_NAME));
#else
    void* lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (lib == nullptr) {
        std::fprintf(stderr, "cannot load %s: %s\n", argv[1], dlerror());
        return 1;
    }
    auto entry = reinterpret_cast<PluginEntryFn>(dlsym(lib, HOSTSDK_ENTRY_NAME));
#endif
    if (entry == nullptr) {
        std::fprintf(stderr, "%s exports no %s\n", argv[1], HOSTSDK_ENTRY_NAME);
        return 1;
    }
    HostApi api{};
    api.size = sizeof(HostApi);
    api.api_version = HOSTSDK_API_VERSION;
    api.log = &Log;
    const int32_t rc = entry(&api);
    std::printf("[host] plug-in returned %d (%s)\n", rc, HostSdk_VersionString());
    return rc == 0 ? 0 : 1;
}
