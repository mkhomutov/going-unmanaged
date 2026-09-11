// Field question Q1: does dev/uat/prod belong in the CMake build
// configuration, or at runtime?
//
// Included by book/26-build-systems-and-cmake.md between the answer
// markers below (CONTRIBUTING.md, "The Question template"). Not an
// exercise - CHECK-style judge, never assert, exercises/questions/'s rule.
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

namespace {
    void set_env(const char* name, const char* value) {
#if defined(_WIN32)
        _putenv_s(name, value);
#else
        setenv(name, value, 1);
#endif
    }

    void unset_env(const char* name) {
#if defined(_WIN32)
        _putenv_s(name, "");
#else
        unsetenv(name);
#endif
    }
}

// --8<-- [start:answer]
// One binary, one build configuration (Debug or Release, sanitizers on or
// off) - and the environment it talks to is a value read into this struct
// ONCE, at startup, the same discipline Recipe 31 uses for a feature flag.
// Nothing here is a preprocessor definition: swapping environments never
// asks for a reconfigure, let alone a rebuild.
struct EndpointConfig {
    std::string url = "https://dev.example.invalid/api";   // the default IS an environment

    static EndpointConfig from_environment() {
        EndpointConfig c;
        if (const char* v = std::getenv("MYPLUGIN_ENDPOINT")) {
            c.url = v;
        }
        return c;
    }
};

class Client {
public:
    explicit Client(EndpointConfig config) : config_(std::move(config)) {}   // read once, kept as a member

    const std::string& endpoint() const { return config_.url; }

private:
    EndpointConfig config_;
};
// --8<-- [end:answer]

namespace {
    int failures = 0;

    void check(bool ok, const char* what) {
        if (!ok) {
            std::printf("FAILED: %s\n", what);
            ++failures;
        }
    }
}

int main() {
    unset_env("MYPLUGIN_ENDPOINT");
    Client dev(EndpointConfig::from_environment());
    check(dev.endpoint() == "https://dev.example.invalid/api",
          "unset MYPLUGIN_ENDPOINT falls back to the dev default");

    // The environment change happens AFTER dev's constructor already ran -
    // proof that the read is a startup event, not a per-call lookup.
    set_env("MYPLUGIN_ENDPOINT", "https://prod.example.invalid/api");
    check(dev.endpoint() == "https://dev.example.invalid/api",
          "a client already constructed does not follow a later environment change");

    Client prod(EndpointConfig::from_environment());
    check(prod.endpoint() == "https://prod.example.invalid/api",
          "a client constructed after the change reads the new environment");

    unset_env("MYPLUGIN_ENDPOINT");

    if (failures == 0) {
        std::printf("q1: OK\n");
    }
    return failures == 0 ? 0 : 1;
}
