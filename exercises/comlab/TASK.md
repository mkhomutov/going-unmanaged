# Still Live at Unload — ticket card (Chapter 35)

This lab is a **ticket, not a task** — the fourth and last of the book's
ticket chapters. The vendor from Chapter 17 has shipped SDK 2.0, and the
object model changed: payloads are now shared, reference-counted objects.
Chapter 35 states everything in full and walks the diagnosis behind a
spoiler fold — work the ticket cold first, and work it with a **ledger**:
this ticket is solved by accounting, one +1 or −1 per SDK call, before
anything is rebuilt.

> **#5561 — Objects still live at unload (since the 2.0 port).** The
> vendor's 2.0 SDK made the project's Things shared, reference-counted
> objects. The port was mechanical — every 1.x call swapped for its 2.0
> spelling, `Thing_DisposeData` for `Thing_Release`. Since then the host
> prints *"plug-in left N objects live"* at document close, with N varying
> by document — and two customers report crashes at close that support
> cannot reproduce. The vendor's migration notes are the comment block at
> the top of `FakeSDK2.h`, beside this card.

**Vendor code:** `FakeSDK2.h` and `FakeSDK2.cpp` beside this card are SDK
2.0 — read them, compile them, link them, **never edit them** (the same
rule as `fakesdk/` and `fakedevice/`). Nothing in the SDK validates your
counting; like the real thing, it simply believes you.

**The other files beside this card are the FIXED reference** — `ref.h`
(the wrapper Chapter 35 builds) and `main.cpp` (the fixed port), kept
green by `build_all.sh` on every push. Do not start from them. Recreate
the broken port below in a scratch directory of your own.

## The code as the 2.0 port shipped it

`main.cpp`, one file:

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

## Work the ticket

1. **Read the migration notes twice.** They are the comment block at the
   top of `FakeSDK2.h`, and they are two sentences long where it matters.
   Then run a **ledger** over the port on paper: for every SDK call, write
   down what the notes say it does to the count (+1, −1, or nothing) and
   whether the reference is *yours*. Total the column per Thing, and
   predict the still-live number before running anything. It is not 5,
   and it is not 0.
2. **Reproduce.** Build the broken port, plain and under the handbook's
   flags. Explain the number you got: *which* objects survived, and why is
   the one that should worry you most dead exactly on time?
3. **Fix the leak only** — the obvious patch, one `Thing_Release` in the
   loop. Predict what changes before you run. Then run the plain build,
   read its counter, and decide whether the program is fixed — *then* run
   the sanitized build.
4. **Read the report against Chapter 33.** Where is the culprit line this
   time — and why did this ticket's crime make it onto the report when
   Chapter 33's never could?
5. **The real fix: encode the convention in a type.** Two named
   constructors — one for references you were handed owning (`Acquire`),
   one for borrows you choose to keep (`PeekActive`) — copy retains, move
   steals, the destructor pays on every path. Write yours before opening
   the chapter's. Acceptance is **both judges at once**: the vendor's
   counter at 0 after shutdown, and the sanitizers quiet, with copies of
   your handle in play.
6. **Stretch: the refcount self-assignment trap.** Write the copy
   assignment the naive way — Release the old, Retain the new — and hand
   it the same handle on both sides. Work out what dies, and why
   copy-and-swap (Chapter 6) never had the problem.
