// A stand-in host: loads a plug-in by path, finds its one entry point, and
// hands it the function table. Every real host does exactly this, with an
// application wrapped around it. Test scaffolding for Chapter 40's lab.
//
//   host <module>            the normal load
//   host <module> --older    pass a table shorter than the plug-in's header
//                            says (Try it, step 4): the plug-in must refuse
#include <hostsdk/hostsdk.h>

#include <cstdio>
#include <cstring>

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
        std::fprintf(stderr, "usage: host <path to plug-in module> [--older]\n");
        return 2;
    }
    const bool older_host = argc > 2 && std::strcmp(argv[2], "--older") == 0;
#if defined(_WIN32)
    HMODULE lib = LoadLibraryA(argv[1]);
    if (lib == nullptr) {
        std::fprintf(stderr, "cannot load %s (error %lu)\n", argv[1], GetLastError());
        return 1;
    }
    // The cast from the loader's untyped result to a function pointer: the
    // one reinterpret_cast every host contains, blessed by the platform.
    auto entry = reinterpret_cast<PluginEntryFn>(GetProcAddress(lib, HOSTSDK_ENTRY_NAME));
#else
    void* lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (lib == nullptr) {
        std::fprintf(stderr, "cannot load %s: %s\n", argv[1], dlerror());
        return 1;
    }
    // void* to a function pointer is conditionally-supported in C++ and
    // guaranteed by POSIX for exactly this call: dlsym's whole purpose.
    auto entry = reinterpret_cast<PluginEntryFn>(dlsym(lib, HOSTSDK_ENTRY_NAME));
#endif
    int rc = 1;
    if (entry == nullptr) {
        std::fprintf(stderr, "%s exports no %s\n", argv[1], HOSTSDK_ENTRY_NAME);
    } else {
        HostApi api{};
        // --older: the table an older host would pass - the same bytes, a
        // smaller size - which the plug-in must refuse rather than read past.
        api.size = older_host ? sizeof(uint32_t) : sizeof(HostApi);
        api.api_version = HOSTSDK_API_VERSION;
        api.log = &Log;
        const int32_t result = entry(&api);
        std::printf("[host] plug-in returned %d (%s)\n", result, HostSdk_VersionString());
        rc = result == 0 ? 0 : 1;
    }
    // Unload only after the last call into the module returned, and after
    // nothing the module handed us is still in use - the callback-lifetime
    // rule of Chapter 16, from the host's seat. Here that is now.
#if defined(_WIN32)
    FreeLibrary(lib);
#else
    dlclose(lib);
#endif
    return rc;
}
