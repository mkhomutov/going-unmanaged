// main.cpp - the caller as the three teams wrote it against the 2009
// header. This file is the acceptance test's other half: build_all.sh
// compiles it, unchanged, against before/ and against after/, and the two
// runs must print the same thing. Included whole by Chapter 45 from below
// this banner.
// --8<-- [start:listing]
#include "catalog.h"
#include <cstdio>

static void Report(const Catalog* c, const char* key) {
    int v = 0;
    if (c->TryGet(key, &v)) {
        std::printf("%s = %d\n", key, v);
    } else {
        std::printf("%s: not found\n", key);
    }
}

int main() {
    Catalog c;
    int n = c.Parse("alpha=1;beta=2;gamma=3");
    std::printf("parsed %d, count %d\n", n, c.Count());

    const Entry* alpha = c.Find("alpha");     // borrowed, held across Adds
    for (int i = 0; i < 10; ++i) {
        char key[16];
        std::snprintf(key, sizeof key, "k%d", i);
        c.Add(key, i * 10);
    }
    std::printf("alpha still at %s = %d, count %d\n", alpha->key, alpha->value, c.Count());

    c.Add("beta", 22);                        // replace in place
    Report(&c, "beta");
    Report(&c, "delta");
    std::printf("malformed: %d, count %d\n", c.Parse("x=;"), c.Count());

    c.Clear();
    std::printf("cleared: count %d\n", c.Count());
    return 0;
}
// --8<-- [end:listing]
