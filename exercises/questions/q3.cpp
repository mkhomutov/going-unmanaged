// Field question Q3: why doesn't switching a CMake preset always change
// what gets built?
//
// Included by book/26-build-systems-and-cmake.md between the answer
// markers below (CONTRIBUTING.md, "The Question template"). Not an
// exercise - CHECK-style judge, never assert, exercises/questions/'s rule.
#include <cstdio>
#include <string_view>

// --8<-- [start:answer]
// A build-variant switch with an implicit default is one state a stale
// CMake cache can reuse without anyone asking it to: a preset that forgets
// to set the variable looks identical to a preset that set it and meant
// it. Make the set exhaustive and let the compiler catch a variant nobody
// picked, instead of silently keeping whichever one configured first.
#if defined(APP_VARIANT_DEV) + defined(APP_VARIANT_UAT) + defined(APP_VARIANT_PROD) == 0
#define APP_VARIANT_DEV 1   // the only place a default lives - one, not an #else
#endif

#if defined(APP_VARIANT_DEV) + defined(APP_VARIANT_UAT) + defined(APP_VARIANT_PROD) != 1
#error "exactly one of APP_VARIANT_DEV/UAT/PROD must be defined - a stale cache holding two would trip this instead of picking one silently"
#endif

const char* variant_name() {
#if defined(APP_VARIANT_PROD)
    return "prod";
#elif defined(APP_VARIANT_UAT)
    return "uat";
#else
    return "dev";
#endif
}
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
    // build_all.sh passes none of the three defines, so the guard's own
    // default applies - the same path a preset that never mentions the
    // variable falls back to.
    check(std::string_view(variant_name()) == "dev",
          "no variant defined falls back to the guard's single default, dev");

    if (failures == 0) {
        std::printf("q3: OK\n");
    }
    return failures == 0 ? 0 : 1;
}
