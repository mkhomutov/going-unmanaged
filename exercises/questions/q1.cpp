// Q1 (Appendix M): what static_assert does and when to reach for it -
// the listing 10-modern-cpp-fluency.md's answer section includes.
//
// Not an exercise - in exercises/cost/'s shape: no TASK.md, a CHECK-style
// main() below the marked section that holds what the answer claims.
#include <cstdio>
#include <string_view>

namespace {
    int failures = 0;

    void check(bool ok, const char* what) {
        if (!ok) {
            std::printf("FAILED: %s\n", what);
            ++failures;
        }
    }
}

// --8<-- [start:answer]
// A portability assumption, checked once instead of trusted forever: this
// chapter's own note is that 'int' is not guaranteed 32 bits the way C#'s
// is, and this line is how the code that relies on it says so.
static_assert(sizeof(int) == 4, "this file assumes a 32-bit int");

// Three build-profile endpoints. Each must be set, and no two may collide -
// a collision would mean a build that silently talks to the wrong server
// the day a profile is misconfigured. constexpr std::string_view compares
// with == at compile time, so the guard runs once, at the build, never
// again at every startup the way a runtime check would.
constexpr std::string_view kDevEndpoint  = "https://dev.example.internal";
constexpr std::string_view kUatEndpoint  = "https://uat.example.internal";
constexpr std::string_view kProdEndpoint = "https://prod.example.internal";

static_assert(kDevEndpoint != kUatEndpoint, "dev and uat endpoints must differ");
static_assert(kDevEndpoint != kProdEndpoint, "dev and prod endpoints must differ");
static_assert(kUatEndpoint != kProdEndpoint, "uat and prod endpoints must differ");

// The one-argument, C++17 form: no message string, so a failure prints the
// condition's own source text instead.
static_assert(!kDevEndpoint.empty());
// --8<-- [end:answer]

int main() {
    // Nothing above can fail at run time - a false condition would have
    // refused the build already. This is the record that it compiled, and
    // that the three values still read back distinct.
    check(kDevEndpoint != kUatEndpoint, "dev and uat stayed distinct");
    check(kDevEndpoint != kProdEndpoint, "dev and prod stayed distinct");
    check(kUatEndpoint != kProdEndpoint, "uat and prod stayed distinct");
    check(sizeof(int) == 4, "int is four bytes on this target");

    if (failures != 0) {
        std::printf("q1: %d FAILED\n", failures);
        return 1;
    }
    std::printf("q1: all claims hold\n");
    return 0;
}
