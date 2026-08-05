# Here Is the Report — ticket card (Chapter 33)

This lab is a **ticket, not a task** — and this ticket comes with the
evidence already attached. Chapter 33 states everything in full and walks
the diagnosis behind a spoiler fold — work the ticket cold first, and work
it **on paper first**: the rule of this exercise is no compiler until your
diagnosis is written down.

> **#5124 — Watched sensor reads 0.0 after hot-plug (since 2.6.0).** The
> dashboard lets the operator pin one sensor as "watched". A nine-sensor
> site reports that the watched value drops to 0.0 the moment the ninth
> sensor is plugged in; re-pinning the sensor fixes it, until the next
> hot-plug. Support cannot reproduce it — no rig in the building has more
> than eight sensors. Meanwhile the nightly sanitizer job, which drives the
> app against a simulated bus, has been red since the night 2.6.0 merged,
> and nobody had connected the two until this ticket. Its report is
> attached, and the code it points into is below, reduced to the three
> files the report actually touches.

**The files beside this card are the FIXED reference** — the lab's green
state, run by `build_all.sh` at two different hot-plug counts on every
push. Do not start from them. Recreate the broken 2.6.0 code below in a
scratch directory of your own, and work from there.

## The attached report

Trimmed of full addresses, the shadow map, and a run of standard-library
frames in the middle of each stack — what remains is everything the
diagnosis needs:

```text
==50242==ERROR: AddressSanitizer: heap-use-after-free on address 0x60c0000002a8
READ of size 8 at 0x60c0000002a8 thread T0
    #0 in main main.cpp:18

0x60c0000002a8 is located 40 bytes inside of 128-byte region [0x60c000000280,0x60c000000300)
freed by thread T0 here:
    #0 in _ZdlPv (libclang_rt.asan_osx_dynamic.dylib)
    #2 in std::__1::allocator<Sensor>::deallocate(Sensor*, unsigned long) allocator.h:120
    ...
    #8 in std::__1::vector<Sensor, std::__1::allocator<Sensor>>::push_back(Sensor&&) vector.h:457
    #9 in Registry::add(int) registry.cpp:4
    #10 in main main.cpp:15

previously allocated by thread T0 here:
    #0 in _Znwm (libclang_rt.asan_osx_dynamic.dylib)
    #2 in std::__1::allocator<Sensor>::allocate(unsigned long) allocator.h:105
    ...
    #8 in std::__1::vector<Sensor, std::__1::allocator<Sensor>>::push_back(Sensor&&) vector.h:457
    #9 in Registry::add(int) registry.cpp:4
    #10 in main main.cpp:7

SUMMARY: AddressSanitizer: heap-use-after-free main.cpp:18 in main
```

## The code as 2.6.0 shipped it

`registry.h` and `registry.cpp` — as beside this card, except `find`'s
line in the header says nothing about how long the pointer stays valid; it
reads:

```cpp
    Sensor* find(int id);                 // look up a sensor by id
```

`main.cpp` is:

```cpp
#include "registry.h"
#include <cstdio>

int main() {
    Registry reg;
    for (int id = 1; id <= 8; ++id) {
        reg.add(id);                      // boot: eight sensors discovered
    }

    Sensor* watched = reg.find(3);        // the dashboard pins sensor 3

    reg.record(3, 21.5);                  // a reading arrives
    std::printf("watched: %.1f\n", watched->last);

    reg.add(9);                           // hot-plug: a ninth sensor

    reg.record(3, 22.1);                  // the next reading arrives
    std::printf("watched: %.1f\n", watched->last);
    return 0;
}
```

## Work the ticket

1. **Read the report against Chapter 31's index — no compiler.** Count the
   stacks and name the bug class. For each stack, write down the line in
   *your* code it lands on — and notice where each stack spends the rest of
   its frames.
2. **Do the arithmetic.** `40 bytes inside of 128-byte region`, and
   `Sensor` is an `int` plus a `double`. How big is one `Sensor`? How many
   fit in the region? Which element is byte 40 inside, and which member of
   it? Name the sensor the program just read, by id, from the address
   alone.
3. **Name the guilty line.** It appears in none of the three stacks. Write
   it down before you build anything.
4. **Only now, reproduce.** Recreate the three files, build plain
   (`g++ -std=c++17 -Wall -Wextra -g`) and run it: the customer's 0.0,
   exit 0. Then build under the handbook's flags and check the report
   against your predictions, frame by frame.
5. **Fix it and prove it.**
   `../../scripts/check.sh registry.cpp main.cpp` on your fixed version
   must run green — and stay green when the hot-plug count is 100 (the
   fixed reference reads the count as a run argument:
   `../../scripts/check.sh registry.cpp main.cpp 100`). The acceptance
   test is that growth stopped mattering, not that one run went quiet.
6. **Stretch: build the two tempting non-fixes.** Store a copy
   (`Sensor watched = *reg.find(3);`), then instead give the registry a
   `reserve(16)`. Predict what each does before running — one is clean and
   wrong, the other is wrong and temporarily clean.
