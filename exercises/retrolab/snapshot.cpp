// snapshot.cpp - the 4.0 feature that could not be written against the 2009
// class, and the judge for the retrofit's own promises: a copy shares
// nothing, a move leaves a valid empty source, an assignment to itself is
// harmless, and Find's address promise survived the change of storage.
// Included whole by Chapter 45 from below this banner. CHECK rather than
// assert, for exercises/choosing/'s reason.
// --8<-- [start:listing]
#include "catalog.h"
#include <cstdio>
#include <utility>

namespace {
int g_failures = 0;
void Check(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAILED: %s\n", what);
        ++g_failures;
    }
}
}   // namespace

int main() {
    Catalog live;
    live.Parse("alpha=1;beta=2");

    Catalog snapshot = live;                  // the feature: a copy to compare against later
    live.Add("alpha", 100);
    live.Add("gamma", 3);
    int v = 0;
    Check(snapshot.TryGet("alpha", &v) && v == 1, "the snapshot kept the old value");
    Check(!snapshot.TryGet("gamma", &v), "the snapshot did not gain the new key");
    Check(live.Count() == 3 && snapshot.Count() == 2, "the two catalogs are independent");

    const Entry* alpha = live.Find("alpha");
    for (int i = 0; i < 100; ++i) {
        char key[16];
        std::snprintf(key, sizeof key, "k%d", i);
        live.Add(key, i);
    }
    Check(live.Find("alpha") == alpha && alpha->value == 100, "Find's address survived a hundred Adds - the 2009 promise, kept");

    Catalog moved = std::move(live);
    Check(moved.Count() == 103 && live.Count() == 0, "a move leaves the source valid and empty");
    live.Add("again", 1);
    Check(live.Count() == 1, "and usable");

    Catalog assigned;
    assigned = std::move(moved);              // move ASSIGNMENT, which a move constructor check does not cover
    Check(assigned.Count() == 103 && moved.Count() == 0, "a move assignment steals the same way");

    Catalog* self = &assigned;
    assigned = *self;                         // self-assignment through a pointer, so the compiler cannot see it
    Check(assigned.Count() == 103, "self-assignment changes nothing");

    if (g_failures != 0) {
        std::printf("retrolab: %d FAILED\n", g_failures);
        return 1;
    }
    std::printf("retrolab: the snapshot feature holds, and the address promise with it\n");
    return 0;
}
// --8<-- [end:listing]
