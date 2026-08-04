## Chapter 33 — Here Is the Report

Chapter 32 handed you a symptom and made you produce the evidence. This one — the second of the ticket-shaped chapters — hands you the evidence itself, because that is the other way work arrives: a sanitizer report from a nightly job, a crash-reporting service, or a colleague who got that far and no further, already attached, already complete, and already unread. Chapter 31 taught you to read these. Consider this the exam. One rule, and it is the whole exercise: **no compiler until your diagnosis is written down.** The report and the source below are sufficient to name the guilty line, and proving that to yourself is what the chapter trains.

### The ticket

> **#5124 — Watched sensor reads 0.0 after hot-plug (since 2.6.0).** The dashboard lets the operator pin one sensor as "watched". A nine-sensor site reports that the watched value drops to 0.0 the moment the ninth sensor is plugged in; re-pinning the sensor fixes it, until the next hot-plug. Support cannot reproduce it — no rig in the building has more than eight sensors. Meanwhile the nightly sanitizer job, which drives the app against a simulated bus, has been red since the night 2.6.0 merged, and nobody had connected the two until this ticket. Its report is attached, and the code it points into is below, reduced to the three files the report actually touches.

Half the tickets you will ever work carry their own diagnosis like this — a red CI job nobody read, a workaround nobody decoded. This chapter is about cashing those in.

### The report

Trimmed the way your eye will learn to trim it — full addresses shortened, the shadow map cut (Chapter 31 told you why), and a run of standard-library frames elided from the middle of each stack. What remains is everything the diagnosis needs:

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

Chapter 31's index works before you see a line of source: count the stacks. Three — and the error name on the first line agrees. You already know the bug class. What you do not know yet is the part no index gives you: this report also contains which sensor, which member, the moment the block died and the moment it was born — everything, in fact, except the one line that is actually wrong.

### The code it happened to

Three files. `Registry` is 2.6.0's hot-plug work: sensors used to be a fixed array; now they arrive whenever the hardware does.

`registry.h`:

```cpp
#pragma once
#include <vector>

struct Sensor {
    int    id;
    double last;    // most recent reading
};

class Registry {
public:
    void add(int id);                     // register a sensor
    Sensor* find(int id);                 // look up a sensor by id
    void record(int id, double value);    // store a reading

private:
    std::vector<Sensor> sensors_;
};
```

`registry.cpp`:

```cpp
#include "registry.h"

void Registry::add(int id) {
    sensors_.push_back(Sensor{id, 0.0});
}

Sensor* Registry::find(int id) {
    for (Sensor& s : sensors_) {
        if (s.id == id) {
            return &s;
        }
    }
    return nullptr;
}

void Registry::record(int id, double value) {
    if (Sensor* s = find(id)) {
        s->last = value;
    }
}
```

And `main.cpp` — the session the sanitizer job drives, reduced: boot discovers eight sensors, the operator pins number 3, a reading arrives, a ninth sensor hot-plugs, another reading arrives:

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

It compiles clean under `-Wall -Wextra`. Built plain, on this machine, it prints the ticket — `watched: 21.5`, then `watched: 0.0` — and exits 0.

### Try it — before reading on

The ticket card is `exercises/reportlab/TASK.md`, with the report and the broken listings together (the files beside the card are the fixed reference — no peeking). Pencil before compiler:

1. **Read the report against Chapter 31's index.** Three stacks name three events. Write down, for each, the line in *your* code it lands on — and remember which stack the mistake usually hides in. Then notice where each stack spends the rest of its frames.
2. **Do the arithmetic.** `40 bytes inside of 128-byte region`, and `Sensor` is an `int` plus a `double`. How big is one `Sensor`? How many fit in the region? Which element is byte 40 inside, and which member of it? Name the sensor the program just read, by id, from the address alone.
3. **Name the guilty line.** It appears in none of the three stacks. That is not a defect in the report; it is the lesson of this chapter.
4. **Only now, reproduce.** Recreate the three files in a scratch directory, build plain (the 0.0, exit 0), then under the canonical flags — and check the report against your predictions, frame by frame.
5. **Fix it and prove it.** `scripts/check.sh` on your fixed version, then again with the hot-plug count at 100. The acceptance test is that growth stopped mattering — not that one run went quiet.
6. **Stretch: build the two tempting non-fixes.** Store a copy (`Sensor watched = *reg.find(3);`), then instead give the registry a `reserve(16)`. Predict what each does before running — one is clean and wrong, the other is wrong and temporarily clean.

### The diagnosis, walked through

<details>
<summary>Show the walkthrough — write your own diagnosis down first</summary>

Three stacks: a use-after-free. The first stack answers *where did I touch it* — `main.cpp:18`, the display line, `watched->last`. Chapter 31 warned that this is where the program noticed, and rarely where the mistake is. Here that warning is true twice over.

The second stack answers *who killed it*, and it is the exam's first real question. Frame #0 is the sanitizer's interceptor, and Chapter 31 said to start reading at #1 — but #1 through #8 are a wall of `std::__1::`. The first frame of yours is #9, `Registry::add`. A function that only ever adds. Grep registry.cpp for `delete`: nothing. Read the wall as one word instead: those frames are `vector` **growing** — `push_back` hit capacity, allocated a bigger block, moved the eight sensors across, and freed the old block. The free is real, correct, and the container's. Your only decision on that stack is having called `add` at `main.cpp:15`, and that decision was fine too.

The third stack answers *where did it come from* — the same road, older: `Registry::add` again, this time from `main.cpp:7`, the boot loop. So the whole life of this block, birth and death, belongs to the vector's private bookkeeping. It was never your block. You only kept a pointer into it.

Now the region line pays out. `Sensor` is an `int` and a `double`: four bytes, four of padding, eight — sixteen. Chapter 31 did this sum for a single object; here it unlocks an array. A 128-byte region is exactly eight sensors — the boot-time block. And byte 40 is 2 × 16 + 8: **element 2, offset 8 — the third sensor's `last`**. The vector was filled with ids 1 through 8, so element 2 is id 3. The pinned one. From an address and a struct definition, the report has named the watched sensor and the very number on the dashboard.

Which leaves the guilty line — and it is in none of the three stacks: `main.cpp:10`, `Sensor* watched = reg.find(3);`. The stacks name *events* — a read, a free, an allocation. The bug is not an event; it is a **decision**: keeping a pointer into a container across a mutation of it. No stack will ever point at a decision. You triangulate instead: the first stack names the expression (`watched->last`), so the question becomes *where did `watched` get its value, and what happened to the container between there and here?* The report hands you both endpoints; the pin is the line between them.

The ticket even attached its own confirmation. Re-pinning fixes it — of course it does: a fresh `find` returns a pointer into the *current* block, good until the next hot-plug moves the sensors again. The customer had discovered the mechanism and written it down as a workaround; nobody read it as evidence.

And the 0.0? The stale read landed in freed-but-still-mapped memory — Chapter 3's quiet failure again. On this machine the allocator had already scribbled over that offset, so the lie was 0.0; an allocator that leaves the bytes alone serves yesterday's reading instead, and the sensor "freezes" — plausible, current-looking, wrong. Same read, different lie, and neither is a crash. That is why no plain run flagged it, on any rig, in any test.

</details>

### What the contract actually says

A name for the notes file, now that you have earned it: **pointer invalidation across container growth** — Chapter 11's trap, Chapter 21's Task 3, arriving the way it arrives at work. `push_back` has been allowed to do this since the beginning: when size reaches capacity, the vector allocates a larger block, moves every element into it, frees the old one — and from that moment every pointer, reference, and iterator into the vector is invalid. Chapter 21 put the `push_back` on the very next line and it *still* sometimes behaved; the job puts the growth in another file, behind a hardware event no test rig fires, and mails you the report weeks later. The mechanism is identical. Distance is the whole difficulty.

> [!NOTE]
> **Surprise for C# devs:** `List<T>` reallocates on growth exactly like `std::vector` — you never noticed, because everyday C# never gives you an address inside the backing array; the one API that does, `CollectionsMarshal.AsSpan`, carries this chapter's warning in its own docs. References track objects, not slots; and on a `List` of structs, `sensors[2].last = v` does not even compile (CS1612) — the compiler refuses at build time precisely the slot-aliasing this chapter just debugged at run time.

The instinct "hold a reference to the thing" was never wrong *in C#*, because a C# reference follows the object wherever the runtime moves it. A C++ pointer is an address. When the object moves, the pointer goes on pointing at where it used to live — and Chapter 3 explains why living there is not even an error until someone checks.

### The fix

Store the key, borrow at the point of use: the fixed main remembers the watched sensor as an id — the one thing growth cannot move — and asks the registry for a pointer only at the moment of each read:

```cpp
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
```

Two decisions, and both matter. The caller keeps the sensor's **id** — the piece of identity that survives reallocation — and turns it into a pointer only at the moment of use, discarding it afterwards. The borrow now lives entirely between two mutations, which is the only place a pointer into a vector was ever valid.

And the API says so now. `registry.h`'s lookup line reads:

```cpp
    Sensor* find(int id);                 // borrow: valid only until the next add()
```

One comment — the term of the loan, written where every caller reads it. Notice what the fix did *not* touch: registry.cpp, the file two of the three stacks pointed at, is unchanged. The weapon was never the culprit.

The acceptance test is the ticket card's, mechanized: `build_all.sh` runs the fixed program with **0 hot-plugs and with 100** — no growth at all, and growth several reallocations deep — because the fix's claim is that growth stopped mattering, and one count cannot prove a claim about all of them. A fresh `find` per display is a linear walk, and for nine sensors that is nothing; the day it is something, the answer is still not a cached pointer — it is an index, or a container that promises stable addresses (below).

### Pitfalls

- **The copy that passes every check and fails the customer.** `Sensor watched = *reg.find(3);` is the C# hand reaching for "just hold the object" — legal, warning-free, sanitizer-clean, exit 0. And the dashboard now shows 0.0 from the *first* screen: the copy froze at pin time, and no `record` will ever reach it. A copy is not a view (Chapter 2). The sanitizer's silence is correct — nothing illegal happens — which is why predicted values, not clean runs, are the test (Finding 10 of Chapter 25 keeps collecting these).
- **`reserve(16)` is the same bet with a higher table limit.** Reserving inside the registry makes this program run clean — nine sensors fit — and the seventeenth sensor brings back the identical report, verified. The ticket returns with a bigger customer attached. `reserve` is for performance and stated intent, never for pinning addresses.
- **"The report says registry.cpp, so registry.cpp is broken."** Two of the three stacks land in a file with no bug in it. The stacks name the weapon and the birthplace; the decision that made them a crime scene appears in no stack, ever. When a freed-by stack is a wall of standard-library frames over one innocent line of yours, read the wall as a single word: *reallocation*.
- **Null-checking the symptom line.** A guard at main.cpp:18 changes nothing: `watched` is not null, it is stale — it points at memory that is fully readable and no longer true. The dangerous pointers are the ones that look fine.

> [!TIP]
> **Key principle:** "A pointer into a growable container is a loan that the next reallocation calls in — I store the key and borrow at the point of use, and when my API hands out a pointer, the comment says how long the loan lasts."

### In the wild

The loan sentence is everywhere once you look for it. The C standard's own `getenv` is allowed to invalidate its previous result on the next call; SQLite's `sqlite3_column_text` pointer is good only until the next step or reset; every vendor `Get*` that returns a pointer into an internal buffer carries the same clause in its docs — or tragically omits it, leaving you to establish the loan's term the way this chapter did. When a design genuinely needs long-lived handles to elements, change the container, not the discipline: `std::vector<std::unique_ptr<Sensor>>` reallocates the *pointers* while the sensors stand still — C#'s `List<T>` of references, rebuilt deliberately, at Chapter 1's price per element — and the node-based containers in Chapter 11's gentler column (`map`, `list`) never move an element at all. And when the pointer crosses a boundary you author (Chapter 30), the loan's term stops being a comment and becomes contract: write it in the header, because the caller on the far side cannot read your source.

<!-- nav:begin -->
[← Chapter 32 — It Crashes on Exit](32-it-crashes-on-exit.md) · [Contents](README.md) · [Chapter 34 — Parse This Capture →](34-parse-this-capture.md)
<!-- nav:end -->
