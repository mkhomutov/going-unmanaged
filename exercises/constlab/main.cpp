// Two judges in one file — Appendix I.
//
// Compiled with no flags it builds, runs and asserts what a const-correct
// class does. Compiled with -DCONSTLAB_VIOLATION_N it must FAIL TO COMPILE,
// which is the only way to assert the thing const is actually for: these are
// not bugs that reach a sanitizer, they are bugs that never reach a binary.
//
// build_all.sh does both, and requires the failures to mention const rather
// than merely failing.
#include "counter.h"

#include <cstdio>

inline int& Failures() { static int n = 0; return n; }

#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::fprintf(stderr, "FAILED %s:%d: %s\n", __FILE__, __LINE__,    \
                         #cond);                                              \
            ++Failures();                                                     \
        }                                                                     \
    } while (0)

// A function that only borrows. The const& says so in the signature, and it
// is the signature - not the body - that the caller can rely on.
static double Report(const Counter& c) {
    return c.Size() == 0 ? 0.0 : c.Average();
}

#ifdef CONSTLAB_VIOLATION_2
// A const member function that writes a member which is NOT mutable.
struct Leaky {
    int n = 0;
    void Touch() const { n = 1; }        // must not compile
};
#endif

int main() {
    Counter c;
    c.Add(2);
    c.Add(4);

    // ---- the const half is reachable through a const reference ----------
    const Counter& view = c;
    CHECK(view.Size() == 2);
    CHECK(Report(view) == 3.0);

    // ---- `mutable` is doing exactly what it claims ----------------------
    // Two calls, one computation: the cache is written inside a const member
    // function, and nothing a caller can see has changed.
    CHECK(view.Average() == 3.0);
    CHECK(c.Recomputes() == 1);

    // ---- and a mutator correctly invalidates it -------------------------
    c.Add(6);
    CHECK(c.Average() == 4.0);
    CHECK(c.Recomputes() == 2);

    // ---- the overload pair selects on the constness of the object -------
    c.At(0) = 10;                        // non-const object: writable
    CHECK(view.At(0) == 10);             // const object: readable only
    // The writable overload invalidated the cache on its way out, so the
    // next Average() is the third computation and reflects {10, 4, 6}.
    CHECK(c.Average() > 6.66 && c.Average() < 6.67);
    CHECK(c.Recomputes() == 3);

#ifdef CONSTLAB_VIOLATION_1
    view.Add(1);                         // non-const method via const&
#endif
#ifdef CONSTLAB_VIOLATION_3
    view.At(0) = 5;                      // write through a const int&
#endif
#ifdef CONSTLAB_VIOLATION_4
    const Counter frozen;
    Counter& writable = frozen;          // binding away const
    (void)writable;
#endif
#ifdef CONSTLAB_VIOLATION_5
    const char* text = "hello";
    text[0] = 'H';                       // write through pointer-to-const
#endif

    if (Failures() == 0) std::printf("constlab: the const half holds\n");
    return Failures() == 0 ? 0 : 1;
}
