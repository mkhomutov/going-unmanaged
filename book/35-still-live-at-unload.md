## Chapter 35 — Still Live at Unload

The fourth ticket-shaped chapter, and the one where the vendor moves the ground under you: an SDK upgrade changes the object model, and code that was correct for ten years becomes wrong by convention. The previous three tickets each ended at a guilty line. This one has no single guilty line to end at — the bug is a *misread rule*, so it recurs at every call site, in both directions at once — and that changes what "the fix" means: not a patch, but a type. Chapter 16's Bestiary promised that Shape 3 was "shared_ptr, someone else's implementation"; this is the ticket where you build the missing half of that sentence.

### The ticket

> **#5561 — Objects still live at unload (since the 2.0 port).** The vendor's 2.0 SDK made the project's Things shared, reference-counted objects. The port was mechanical — every 1.x call swapped for its 2.0 spelling, `Thing_DisposeData` for `Thing_Release`. Since then the host prints *"plug-in left N objects live"* at document close, with N varying by document — and two customers report crashes at close that support cannot reproduce. The vendor's migration notes are attached.

Two symptoms, and they do not obviously share a cause: a counter that drifts up, and a crash that happens to other people. Hold both; the diagnosis owes you the connection.

### The vendor's new contract

Chapter 17's SDK handed you a payload and one obligation: dispose exactly once. Version 2.0 replaces the payload with an opaque, reference-counted object — and the obligation with a *convention*:

```cpp
// ============================================================================
// FakeSDK2.h - version 2.0 of the FakeSDK: the project's Things are now
// shared, REFERENCE-COUNTED objects behind opaque handles.
// DO NOT MODIFY THIS FILE. Treat it as vendor code: read it, wrap it, obey it.
//
// THE OWNERSHIP CONVENTION (migration notes, section 2 - read it twice):
//   - Thing_Acquire hands you a reference YOU OWN: the SDK retains it on
//     your behalf, and you owe exactly one Thing_Release when you are done.
//   - Project_PeekActive hands you a reference you DO NOT own: the project
//     keeps it alive for now, and your pointer is valid only until the
//     active Thing changes. Do not release it. Call Thing_Retain first if
//     you intend to keep it.
//   - Thing_Retain / Thing_Release return the new count FOR DIAGNOSTICS
//     ONLY - do not base logic on it.
// ============================================================================
#pragma once
#include <cstddef>

using ErrCode = int;

constexpr ErrCode NoErr        = 0;
constexpr ErrCode ErrNullParam = 1;   // a required pointer was null
constexpr ErrCode ErrBadIndex  = 2;   // no Thing with that index

struct ThingRef;    // opaque: the SDK owns the definition

// How many Things exist in the "project". Never fails if count is non-null.
ErrCode Project_GetCount(size_t* count);

// Fill *out with a reference to the Thing at 'index' (0-based). On success
// the reference is RETAINED ON YOUR BEHALF - you own one Release.
ErrCode Thing_Acquire(size_t index, ThingRef** out);

// The project's currently-active Thing, or null if none. BORROWED: the
// project keeps its own reference, and yours is valid only until the
// active Thing changes. Retain it if you keep it; never release a peek.
ThingRef* Project_PeekActive();

size_t Thing_Retain(ThingRef* ref);    // +1; returns the new count (diagnostics only)
size_t Thing_Release(ThingRef* ref);   // -1; frees the Thing at zero; returns the new count

// Sum of the Thing's values. Requires a non-null ref.
ErrCode Thing_Sum(const ThingRef* ref, double* sum);

// Test-support: (re)build the fake project - thingCount Things, the one at
// activeIndex marked active. Releases any previous project first.
void FakeSdk2_Setup(size_t thingCount, size_t activeIndex);

// Test-support: the host closing the document - the project releases its
// own references and forgets its Things. Objects a client still holds
// references to survive this.
void FakeSdk2_Shutdown();

// Test-support: how many Thing objects are currently alive. The host
// checks this at plug-in unload, AFTER shutdown - it MUST be zero, and it
// is this SDK's leak detector (v1's FakeSdk_LiveAllocations, grown up).
size_t FakeSdk2_LiveObjects();
```

This is Bestiary Shape 3 with the serial numbers filed off: status codes, out-parameters that arrive already retained, `Retain`/`Release` returning a count the docs immediately tell you not to trust — the exact posture of COM's `AddRef`/`Release` and of retain/release APIs everywhere. Note what the SDK does *not* do: nothing in it validates your counting. Like the real thing, it simply believes you.

### The code it happened to

The port, mechanical as advertised. Two comments in it record the porter's reasoning — read them as evidence:

```cpp
#include "FakeSDK2.h"
#include <cstdio>

int main() {
    FakeSdk2_Setup(5, 2);    // five Things; the third is active

    double total = 0.0;
    size_t count = 0;
    Project_GetCount(&count);
    for (size_t i = 0; i < count; ++i) {
        ThingRef* ref = nullptr;
        if (Thing_Acquire(i, &ref) != NoErr) {
            continue;
        }
        double sum = 0.0;
        if (Thing_Sum(ref, &sum) == NoErr) {
            total += sum;
        }
        // 1.x had Thing_DisposeData here; 2.0 manages its own objects now
    }
    std::printf("total: %.1f\n", total);

    ThingRef* active = Project_PeekActive();
    double activeSum = 0.0;
    if (active != nullptr && Thing_Sum(active, &activeSum) == NoErr) {
        std::printf("active: %.1f\n", activeSum);
    }
    Thing_Release(active);    // ported from Thing_DisposeData: clean up after a Get

    FakeSdk2_Shutdown();
    std::printf("still live at unload: %zu\n", FakeSdk2_LiveObjects());
    return 0;
}
```

It compiles clean, and the sums are perfect. Plain or under the full canonical flags, on this machine, identically and silently:

```text
total: 45.0
active: 9.0
still live at unload: 4
```

Four. Not five — one object per acquisition leaked would be five — and not zero. The sanitizers have nothing to add: every read here was a valid read of live memory, and on this platform no leak report exists to disagree (Chapter 31's macOS note, still collecting rent). The counter the vendor ships is the one detector at work on every platform, which is precisely why they ship it. Hold that 4.

### Try it — before reading on

The ticket card is `exercises/comlab/TASK.md`, with the vendor files beside it (`FakeSDK2.h`/`.cpp` — read, compile, link, never edit; `ref.h` and `main.cpp` are the fixed reference — no peeking). This ticket is solved by accounting:

1. **Read the migration notes twice**, then run a **ledger** over the port on paper: for every SDK call, what the notes say it does to the count (+1, −1, nothing) and whether the reference is *yours*. Total the column per Thing and predict the still-live number before running. It is not 5, and it is not 0.
2. **Reproduce.** Plain and under the flags. Explain your number: which objects survived — and why is the one that should worry you most dead exactly on time?
3. **Fix the leak only** — the obvious patch, one `Thing_Release` in the loop. Predict what changes before you run. Read the plain build's counter and decide whether the program is fixed; *then* run the sanitized build.
4. **Read the report against Chapter 33.** Where is the culprit line this time, and why did this ticket's crime make it onto the report when Chapter 33's never could?
5. **The real fix: encode the convention in a type** — two named constructors, one per sentence of the migration notes; copy retains, move steals, the destructor pays on every path. Write yours before opening the fold. Acceptance is **both judges**: the counter at 0 after shutdown, and the sanitizers quiet, with copies of your handle in play.
6. **Stretch: the refcount self-assignment trap.** Write copy assignment the naive way — Release the old, Retain the new — and hand it the same handle on both sides. Work out what dies, and why copy-and-swap (Chapter 6) never had the problem.

### The diagnosis, walked through

<details>
<summary>Show the walkthrough — total your own ledger first</summary>

The ledger, from the migration notes and nothing else:

| Call in the port | The notes say | The port did | Balance per Thing |
|---|---|---|---|
| `Thing_Acquire`, five times | +1 each, and each is yours to release | never released | +1 owed on each of the five |
| `Project_PeekActive` | +0 — not yours | released it anyway | −1 overpaid on Thing 2 |

Now total per object. Things 0, 1, 3 and 4 carry one unpaid debt each: when `FakeSdk2_Shutdown` makes the project let go, each still has the leaked reference holding it — four objects alive with no pointer anywhere that remembers them. Thing 2 — the active one — carries *both* mistakes: the leak's +1 and the wrong release's −1. They cancel. It dies exactly on time, at shutdown, as if the code had been correct. **Two bugs, opposite signs, and one object where they overlap and add to zero.** The counter's 4 was the sum of a ledger nobody kept — and the object the port actually mistreated worst is the one the counter cannot see.

That cancellation is also where the customers' crashes live. The over-release only frees something while the leak is there to mask it — remove the mask and it fires. Which is exactly what the obvious patch does. Add the missing `Thing_Release(ref)` to the loop — 2.0.1, "fixes the counter" — and on this machine the plain build now prints `still live at unload: 0` and exits 0. It looks *completely* fixed. It is worse than before. Under the canonical flags, every run now ends:

```text
==77125==ERROR: AddressSanitizer: heap-use-after-free ...
READ of size 8 at 0x603000001d28 thread T0
    #0 in Thing_Release(ThingRef*) FakeSDK2.cpp:68
    #1 in (anonymous namespace)::release_project() FakeSDK2.cpp:33
    #2 in FakeSdk2_Shutdown() FakeSDK2.cpp:98
    #3 in main main.cpp:30

freed by thread T0 here:
    #0 in _ZdlPv (libclang_rt.asan_osx_dynamic.dylib)
    #1 in Thing_Release(ThingRef*) FakeSDK2.cpp:71
    #2 in main main.cpp:28

previously allocated by thread T0 here:
    #0 in _Znwm (libclang_rt.asan_osx_dynamic.dylib)
    #1 in (anonymous namespace)::make_thing(int) FakeSDK2.cpp:19
    #2 in FakeSdk2_Setup(unsigned long, unsigned long) FakeSDK2.cpp:91
    #3 in main main.cpp:5

SUMMARY: AddressSanitizer: heap-use-after-free FakeSDK2.cpp:68 in Thing_Release(ThingRef*)
```

Sequence it: with the loop now balanced, the active Thing's count is 1 — the project's own reference and nothing else. The wrong release at `main.cpp:28` takes it to zero and the SDK, believing you, frees it. Then the *host* closes the document, `FakeSdk2_Shutdown` walks the project's table, and the project releases its reference to an object that no longer exists — the access stack is the vendor's own teardown, tripping over your bookkeeping. Note the contrast with Chapter 33, where the guilty line appeared in no stack because the crime was a *decision*: here the crime is an *event* — the release itself — so `main.cpp:28` sits right on the freed-by stack. When an over-release is the bug, the report hands you the culprit; when a leak is the bug, no report exists at all. The two halves of this ticket sit on opposite sides of what sanitizers can see.

And the plain 2.0.1 build's clean-looking `0`? The dangling decrement inside `Thing_Release` landed in freed memory, so even the diagnostic arithmetic went through the looking glass — the counter that correctly said 4 on the broken build says 0 on the *more* broken one. The patch erased the evidence and promoted the crash from "two customers, sometimes" to "everyone, every close". Undefined behavior does not owe your leak detector the truth, either.

</details>

### What the contract actually says

A name for the notes file: **manual reference counting** — shared ownership where the count lives inside the object and moves only when a call moves it. The rule that governs every API of this shape is Chapter 16's, now with teeth: **one reference per acquisition** — and the function's *name* tells you whether an acquisition happened. `Acquire`, `Copy`, `Create`, an out-parameter documented as "retained on your behalf": that reference is yours, and you owe exactly one release. `Peek`, `Get`-that-borrows, a returned pointer whose comment mentions someone else keeping it alive: not yours, and releasing it spends money you never had. The 1.x reflex — "clean up after every Get" — was correct for a contract where every Get allocated. The 2.0 contract splits Get in two, and the port applied the old rule to both halves: skipped the payment it owed, and paid a debt it didn't.

> [!NOTE]
> **Surprise for C# devs:** you have been a COM client for most of your career — every Office interop object was one, and the runtime wrapped each in an RCW that did the counting for you. The folklore you may remember — `Marshal.ReleaseComObject`, "never use two dots with interop" — was exactly this chapter leaking up through that abstraction. There is no wrapper here: the count moves only when you move it, and a C# reference's superpower — being *seen* by the collector — is the one thing a raw `ThingRef*` does not have.

This is also `shared_ptr`'s discipline with the count relocated. `shared_ptr` keeps the count in a control block beside the object, and copies of the `shared_ptr` move it; a refcounted SDK keeps the count *inside* the object, because the object crosses a C boundary where no control block could follow. Same invariant — last release destroys — different bookkeeper. (One axis of Shape 3 this lab leaves unmodelled: COM's `QueryInterface`, capability lookup by ID. It rides the same rule — every successful query hands back a reference you own — so nothing below changes when you meet it.)

### The fix: encode the convention in a type

Patching call sites is the 2.0.1 story again with more steps: every future call site retakes the same exam, and the codebase converges on correct only as fast as its slowest reviewer. The fix that closes the ticket is Chapter 16's habit applied to a count — *wrap the answers in a guard type* — and the migration notes' two sentences become two named constructors:

```cpp
#pragma once
#include "FakeSDK2.h"
#include <utility>

// The migration notes' two sentences, encoded as two named constructors.
// adopt() wraps a reference some call already retained on your behalf
// (Thing_Acquire's +1); share() wraps a borrowed pointer by taking a
// reference of its own (Project_PeekActive). After this file, no line of
// the plug-in spells Thing_Retain or Thing_Release again.
class ThingHandle {
public:
    ThingHandle() = default;

    static ThingHandle adopt(ThingRef* raw) {    // "you own one Release"
        return ThingHandle(raw);
    }
    static ThingHandle share(ThingRef* raw) {    // "retain it if you keep it"
        if (raw != nullptr) {
            Thing_Retain(raw);
        }
        return ThingHandle(raw);
    }

    ThingHandle(const ThingHandle& other) : ref_(other.ref_) {
        if (ref_ != nullptr) {
            Thing_Retain(ref_);    // a copy duplicates the CLAIM, not the Thing
        }
    }
    ThingHandle& operator=(const ThingHandle& other) {
        ThingHandle tmp(other);              // copy-and-swap: Chapter 6's shape
        std::swap(ref_, tmp.ref_);
        return *this;                        // tmp's destructor pays our old debt
    }
    ThingHandle(ThingHandle&& other) noexcept : ref_(other.ref_) {
        other.ref_ = nullptr;                // steal and null out - Chapter 6's rule
    }
    ThingHandle& operator=(ThingHandle&& other) noexcept {
        std::swap(ref_, other.ref_);         // other's destructor pays our old debt
        return *this;
    }
    ~ThingHandle() {
        if (ref_ != nullptr) {
            Thing_Release(ref_);             // every path pays, exactly once
        }
    }

    ThingRef* get() const { return ref_; }
    explicit operator bool() const { return ref_ != nullptr; }

private:
    explicit ThingHandle(ThingRef* raw) : ref_(raw) {}

    ThingRef* ref_ = nullptr;
};
```

This is the Chapter 15 Buffer's Rule of Five with one substitution that changes everything: the Buffer's copy constructor duplicated the *resource*; this one duplicates the *claim* — a retain, not an allocation — because the resource is shared by design. The move operations are unchanged from Chapter 6's rule (steal and null out, or the destructor pays twice), copy-and-swap makes assignment self-safe for free, and the destructor is the whole point: **every path pays, exactly once** — including the `continue` that silently leaked in the 2.0.0 loop. It is `Microsoft::WRL::ComPtr` at one-tenth scale, and building it once is what makes the full-size ones legible.

The ported feature, re-ported — and notice what is absent:

```cpp
#include "FakeSDK2.h"
#include "ref.h"
#include <cstdio>

int main() {
    FakeSdk2_Setup(5, 2);    // five Things; the third is active

    double total = 0.0;
    double bestSum = -1.0;
    double activeSum = 0.0;
    {
        size_t count = 0;
        Project_GetCount(&count);
        ThingHandle best;
        for (size_t i = 0; i < count; ++i) {
            ThingRef* raw = nullptr;
            if (Thing_Acquire(i, &raw) != NoErr) {
                continue;
            }
            ThingHandle t = ThingHandle::adopt(raw);    // Acquire's +1 is ours
            double sum = 0.0;
            if (Thing_Sum(t.get(), &sum) != NoErr) {
                continue;                    // t still releases on this path
            }
            total += sum;
            if (sum > bestSum) {
                bestSum = sum;
                best = t;                    // a copy: one more claim, retained
            }
        }

        ThingHandle active = ThingHandle::share(Project_PeekActive());
        if (active) {
            Thing_Sum(active.get(), &activeSum);
        }
    }    // every handle returns its references here

    FakeSdk2_Shutdown();     // the host closes the document...
    const size_t live = FakeSdk2_LiveObjects();

    std::printf("total %.1f, best %.1f, active %.1f, live at unload %zu\n",
                total, bestSum, activeSum, live);
    if (total != 45.0 || bestSum != 15.0 || activeSum != 9.0 || live != 0) {
        std::printf("FAILED: the ledger does not balance\n");
        return 1;
    }
    return 0;
}
```

Not one `Thing_Retain` or `Thing_Release` in it — grep the plug-in and the only hits are inside `ref.h`, which is Shape 3's "never call Release by hand" turned from advice into a property you can check in review. The acceptance test needs **two judges, one per direction**: the vendor's counter catches a release too few (the leak that no macOS sanitizer would name), and the sanitizers catch a release too many (the over-release that no counter can be trusted about, as the fold showed). `build_all.sh` runs exactly that on every push — the fixed port, copies in play, both acquisition conventions exercised, asserting the sums *and* `FakeSdk2_LiveObjects() == 0` after shutdown, under the full canonical flags.

### Pitfalls

- **Patching call sites is whack-a-mole with a compiler.** Every `Release` you add by hand is correct until the next early return, and every new call site retakes the exam. If the same bug family keeps filing tickets, the fix is a type, not a patch — that is what it means for ownership to be *encoded* rather than remembered.
- **A near-right counter proves nothing.** The broken port's 4-of-5 came from two bugs cancelling on one object, and the half-fixed port's clean 0 came from undefined behavior corrupting the arithmetic itself. Keep the ledger per acquisition, never per total — totals are where opposite mistakes go to hide.
- **`shared_ptr` with a custom deleter is one constructor short.** `std::shared_ptr<ThingRef>(raw, Thing_Release)` spells `adopt` beautifully — and hands you nothing for `share`, so the peeked pointer gets wrapped the same way and the over-release is back, now hidden inside a smart pointer that looks like the fix. The convention has two sentences; whatever machinery you use must have two spellings.
- **The count that `Retain`/`Release` return is a diagnostic, not a value.** The docs say so, COM's do too, and in any host with threads the number is stale before you can branch on it. If your logic reads the returned count, the design is wrong somewhere else.

> [!TIP]
> **Key principle:** "In a refcounted API, the function's name tells me whether I own a release — acquired, copied or created means yes; peeked or borrowed means no. I encode each answer once, in a wrapper's named constructors — adopt or share — and after that no line of mine spells Retain or Release."

### In the wild

Shape 3's ecosystem is built from exactly the pieces this ticket assembled. COM's out-parameters arrive already `AddRef`'d and its `Release` returns a count the documentation tells you is for diagnostics only; the wrappers the Bestiary named — `Microsoft::WRL::ComPtr`, `winrt::com_ptr`, `CComPtr` — are `ThingHandle` grown up, down to the adopt-versus-share split (`Attach` versus assignment, in ComPtr's spelling). Core Foundation made the naming half explicit enough to have official names: the **Create Rule** (functions named Create or Copy hand you a reference you own) and the **Get Rule** (functions named Get hand you a borrow) — the migration notes' two sentences, published by a platform vendor. Python's C API says *new reference* and *borrowed reference* in every function's documentation and is notorious for exactly this chapter's pair of bugs in extension modules. Three ecosystems, one convention, independently reinvented — which is the practical lesson for whatever refcounted SDK lands on your desk next: its documentation contains those two sentences somewhere. Find them first, build `adopt` and `share` the same afternoon, and the rest of the integration never counts anything by hand. And when `QueryInterface` or its cousins appear, the rule holds: a successful query is an acquisition, and the new interface pointer is a debt in your ledger like any other.

<!-- nav:begin -->
[← Chapter 34 — Parse This Capture](34-parse-this-capture.md) · [Contents](README.md) · [Appendix A — Fundamentals Refresher →](A-fundamentals-refresher.md)
<!-- nav:end -->
