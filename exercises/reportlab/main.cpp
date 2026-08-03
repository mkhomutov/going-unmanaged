#include "registry.h"
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    // Hot-plug count. build_all.sh runs this with 0 AND with 100: the fix's
    // claim is that growth stopped mattering, and one count cannot prove a
    // claim about all of them.
    const int hotplug = argc > 1 ? std::atoi(argv[1]) : 1;

    Registry reg;
    for (int id = 1; id <= 8; ++id) {
        reg.add(id);                      // boot: eight sensors discovered
    }

    const int watched = 3;                // the dashboard keeps the key,
    reg.record(watched, 21.5);            // not a pointer

    for (int i = 0; i < hotplug; ++i) {
        reg.add(9 + i);                   // hot-plug arrives mid-session
    }

    reg.record(watched, 22.1);
    const Sensor* s = reg.find(watched);  // borrowed at the point of use,
    if (s == nullptr || s->last != 22.1) {    // used, and not kept
        std::printf("FAILED: watched sensor is stale or lost\n");
        return 1;
    }
    std::printf("watched sensor %d: %.1f after %d hot-plug(s)\n",
                watched, s->last, hotplug);
    return 0;
}
