// The test binary: a second main (Chapter 28), so it can never be a source of
// the library. CHECK counts failures and sets the exit code - not assert,
// which a Release build compiles away (Appendix H).
#include "myplugin/session.h"

#include <cstdio>

static int g_failures = 0;
#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

int main() {
    myplugin::Session s;
    CHECK(s.Count() == 0);
    CHECK(s.Mean() == 0.0);  // an empty session has a defined mean, not a NaN
    s.Ingest(1.0);
    s.Ingest(2.0);
    s.Ingest(6.0);
    CHECK(s.Count() == 3);
    CHECK(s.Mean() == 3.0);
    if (g_failures == 0) {
        std::printf("session_test ok\n");
    }
    return g_failures == 0 ? 0 : 1;
}
