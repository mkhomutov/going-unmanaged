## Chapter 17 — Exercise: The FakeSDK

*Trains: Chapter 1 (RAII), Chapter 8 (error codes), Chapter 12 (multiple translation units). Time: ~90 min. This is the closest exercise to real plug-in work: the function you will write is structurally identical to "aggregate a property over all elements" against any desktop-application SDK.*

### The vendor code

Two files, `FakeSDK.h` and `FakeSDK.cpp` — **read them, compile them, link them, never edit them.** The header is the contract; every convention in it mirrors the classic desktop-SDK idiom: every function returns `ErrCode` (0 = success), "Get" functions fill caller-provided structs passed by address, and `Thing_GetData` allocates a payload the caller must release with `Thing_DisposeData` exactly once. The SDK has a built-in leak detector: `FakeSdk_LiveAllocations()` must be **0** after your code runs.

```cpp
// ============================================================================
// FakeSDK.h - a miniature C-style API in the classic desktop-SDK idiom.
// DO NOT MODIFY THIS FILE. Treat it as vendor code: read it, wrap it, obey it.
//
// Conventions (the classic C-flavored desktop-SDK idiom):
//   - every function returns ErrCode; 0 (NoErr) means success
//   - "Get" functions fill caller-provided structs passed by address
//   - ThingData owns heap allocations made by the SDK; the caller MUST
//     release them with Thing_DisposeData exactly once
//   - passing null pointers is an error (ErrNullParam), not a crash
// ============================================================================
#pragma once
#include <cstddef>
#include <cstdint>   // SIZE_MAX, used as "no index" in FakeSdk_Setup

using ErrCode = int;

constexpr ErrCode NoErr        = 0;
constexpr ErrCode ErrNullParam = 1;   // a required pointer was null
constexpr ErrCode ErrBadIndex  = 2;   // no Thing with that index
constexpr ErrCode ErrNoData    = 3;   // Thing exists but has no payload
constexpr ErrCode ErrInternal  = 4;   // simulated transient failure

// A Thing's payload. 'values' is allocated BY THE SDK inside Thing_GetData;
// the caller owns disposal via Thing_DisposeData. All other fields are inline.
struct ThingData {
    int     id;          // stable identifier of the Thing
    size_t  valueCount;  // number of entries in 'values'
    double* values;      // SDK-allocated array; null until Thing_GetData
};

// How many Things exist in the "project". Never fails if count is non-null.
ErrCode Thing_GetCount(size_t* count);

// Fill 'data' for the Thing at 'index' (0-based).
//   - allocates data->values (caller must dispose)
//   - on ANY failure, 'data' is left untouched and nothing is allocated
// Note: some Things in the project legitimately have no payload and
// return ErrNoData. Others may fail transiently with ErrInternal.
ErrCode Thing_GetData(size_t index, ThingData* data);

// Release the payload of 'data'. Safe on a zeroed struct. After the call,
// data->values is null and valueCount is 0. Calling twice is safe;
// calling on a struct whose 'values' you overwrote by hand is not.
ErrCode Thing_DisposeData(ThingData* data);

// Sum of all entries in data->values. Requires a non-null, filled 'data'.
ErrCode Thing_SumValues(const ThingData* data, double* sum);

// Test-support: configure the fake project. 'failAtIndex' makes
// Thing_GetData return ErrInternal for that index (pass SIZE_MAX for none).
void FakeSdk_Setup(size_t thingCount, size_t emptyIndex, size_t failAtIndex);

// Test-support: how many SDK allocations are currently live.
// After your code runs, this MUST be zero - it is the leak detector.
size_t FakeSdk_LiveAllocations();
```

(The matching `FakeSDK.cpp` implements this contract and ships with the repository. Build with both translation units: `g++ -std=c++17 -Wall -Wextra -fsanitize=address -g FakeSDK.cpp yourfile.cpp -o task`.)

### The task

**Part A** — `ThingDataGuard`: an RAII wrapper ensuring disposal on every path (the guard shape from Chapter 1). Decide and be ready to defend: copyable? movable? neither?

**Part B** — the worker:

```cpp
// Sums the values of every Thing in the project.
// Things with no payload (ErrNoData) are skipped and counted, not errors.
// Any other failure aborts and propagates the code - with NO leaks.
ErrCode SumAllThings(double* total, size_t* skippedCount);
```

Check **every** return code; `ErrNoData` is a normal skip; other failures propagate; early returns must not leak; validate your own parameters the way the SDK validates its own. Style target: flat early-return chains, not nested ifs (Chapter 8).

**Part C** — three scenarios in `main`, predictions computed **by hand** as comments before running, asserting *values* — not just "no crash" (Finding 10): a happy path with one empty Thing; a mid-loop transient failure (the critical check: were the payloads of the already-read Things disposed?); and an empty project (decide what "correct" even means there).

### Reference solution

<details>
<summary><strong>Show the solution — do the exercise cold first</strong></summary>

```cpp
// FakeSDK exercise - reference solution.
#include "FakeSDK.h"
#include <cassert>
#include <iostream>

// Part A - the RAII guard. Non-copyable, non-movable: it aliases one struct
// for one scope; copying would double-dispose, moving has no use case here.
class ThingDataGuard {
    ThingData& d_;
public:
    explicit ThingDataGuard(ThingData& d) : d_(d) {}
    ~ThingDataGuard() { Thing_DisposeData(&d_); }   // safe even if never filled
    ThingDataGuard(const ThingDataGuard&) = delete;
    ThingDataGuard& operator=(const ThingDataGuard&) = delete;
};

// Part B - the worker. Flat early returns; every code checked; no leaks.
ErrCode SumAllThings(double* total, size_t* skippedCount) {
    if (!total || !skippedCount) return ErrNullParam;   // validate like the SDK does
    *total = 0;
    *skippedCount = 0;

    size_t count = 0;
    ErrCode err = Thing_GetCount(&count);
    if (err != NoErr) return err;

    for (size_t i = 0; i < count; ++i) {
        ThingData data = {};                    // zero-init: values == nullptr
        err = Thing_GetData(i, &data);
        if (err == ErrNoData) {                 // documented: nothing allocated
            ++*skippedCount;                    // on failure -> safe to just skip
            continue;
        }
        if (err != NoErr) return err;           // ditto: nothing to dispose

        ThingDataGuard guard(data);             // from here, disposal guaranteed

        double sum = 0;
        err = Thing_SumValues(&data, &sum);
        if (err != NoErr) return err;           // guard disposes on this exit
        *total += sum;
    }                                           // guard disposes each iteration
    return NoErr;
}

int main() {
    double total; size_t skipped; ErrCode err;

    // Scenario 1: 4 Things, index 2 empty. Hand-computed expectation:
    // thing0: 3 vals 0,1,2        -> 3
    // thing1: 4 vals 10..13       -> 46
    // thing2: skipped
    // thing3: 3 vals 30,31,32     -> 93        total = 142, skipped = 1
    FakeSdk_Setup(4, 2, SIZE_MAX);
    err = SumAllThings(&total, &skipped);
    assert(err == NoErr && skipped == 1 && total == 142.0);
    assert(FakeSdk_LiveAllocations() == 0);
    std::cout << "scenario1 ok: total=" << total << " skipped=" << skipped << "\n";

    // Scenario 2: Thing 2 fails transiently. Things 0,1 were read first -
    // the CRITICAL check is that their payloads were disposed on the abort.
    FakeSdk_Setup(4, SIZE_MAX, 2);
    err = SumAllThings(&total, &skipped);
    assert(err == ErrInternal);
    assert(FakeSdk_LiveAllocations() == 0);     // Finding 10: check VALUES
    std::cout << "scenario2 ok: propagated err=" << err << ", no leaks\n";

    // Scenario 3: empty project. Correct = NoErr, total 0, skipped 0.
    FakeSdk_Setup(0, SIZE_MAX, SIZE_MAX);
    err = SumAllThings(&total, &skipped);
    assert(err == NoErr && total == 0.0 && skipped == 0);
    std::cout << "scenario3 ok: empty project is a valid, zero result\n";

    // Robustness: our own null-param contract.
    assert(SumAllThings(nullptr, &skipped) == ErrNullParam);
    return 0;
}
```

</details>

### Pitfalls this exercise plants — and why they matter

**The documentation trap.** The header states: *"on ANY failure, 'data' is left untouched and nothing is allocated."* That single sentence is what makes `continue` after `ErrNoData` — and `return` after other errors — safe *without* a guard at those points. Miss it, and you either dispose something never allocated (harmless here because `Thing_DisposeData` tolerates zeroed structs — but only because you zero-initialized with `= {}`), or you wrap the guard too early and reason about it wrongly. Vendor docs reward forensic reading; at work, verify such claims with a test before trusting them, because real SDKs are not always this honest.

**The guard placement decision.** The guard is constructed *after* the success check, not before the call. Both placements can be made correct, but they encode different reasoning: guard-after-success relies on the "nothing allocated on failure" contract; guard-before-call relies on dispose-tolerates-empty plus zero-initialization. The reference chooses guard-after-success because it depends on the *documented* contract rather than on incidental tolerance. Being able to articulate which contract your cleanup depends on is exactly the skill real SDK payload-handling requires.

**Zero-initialization is load-bearing.** `ThingData data = {};` makes `values` null before any SDK call. Skip it and the struct holds stack garbage; on the `ErrNoData` path nothing was written, and any later dispose call would `delete[]` a garbage pointer — undefined behavior with no ASan warning until it detonates. The Chapter 2 idiom (`= {}` on every API struct) is not style; it is the difference between "skip path is safe" and "skip path is a time bomb."

**Why `Thing_DisposeData` and never your own `delete[]`.** The exercise states this as a rule and the reason arrives much later, so carry it now: the payload was allocated inside the SDK, and a library and its caller can hold two different heaps — a different C runtime, a different build configuration, sometimes only a different version of the same one. Releasing with your allocator a block that came from theirs is undefined behavior, and the failure surfaces somewhere unrelated, long after the free. That is why every SDK in this book ships a matching dispose function for anything it hands you, and why "it works on my machine" is the *expected* symptom here rather than a reassuring one — your machine is the case where the two heaps happen to be the same one. [Chapter 30](30-authoring-an-abi-boundary.md#chapter-30--authoring-an-abi-boundary) is this rule from the other side of the boundary, where publishing it becomes your job.

**Why the guard is non-copyable and non-movable.** It aliases one struct for one scope. A copy would mean two guards disposing the same payload — double-dispose (the FileHandle argument from Chapter 1). Movability has no use case at this scope and would complicate the invariant. Deleting both is not a limitation; it is the design stated in code.

**The empty-project scenario is a specification question, not a coding one.** Zero Things means `NoErr`, total 0, skipped 0 — the loop simply never runs. The exercise includes it because real plug-ins constantly meet empty selections and empty documents, and "what does success mean on empty input" is a question to settle *before* writing the loop, not after a bug report.

---


<!-- nav:begin -->
[← Chapter 16 — The SDK Bestiary](16-the-sdk-bestiary.md) · [Contents](README.md) · [Chapter 18 — Exercise: The Device SDK →](18-exercise-the-device-sdk.md)
<!-- nav:end -->
