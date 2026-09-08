## Chapter 17 — Exercise: The FakeSDK

*Trains: Chapter 1 (RAII), Chapter 8 (error codes), Chapter 12 (multiple translation units). Time: ~90 min. This is the closest exercise to real plug-in work: the function you will write is structurally identical to "aggregate a property over all elements" against any desktop-application SDK.*

### The vendor code

Two files, `FakeSDK.h` and `FakeSDK.cpp` — **read them, compile them, link them, never edit them.** The header is the contract; every convention in it mirrors the classic desktop-SDK idiom: every function returns `ErrCode` (0 = success), "Get" functions fill caller-provided structs passed by address, and `Thing_GetData` allocates a payload the caller must release with `Thing_DisposeData` exactly once. The SDK has a built-in leak detector: `FakeSdk_LiveAllocations()` must be **0** after your code runs.

```cpp
--8<-- "exercises/fakesdk/FakeSDK.h"
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
--8<-- "solutions/fakesdk_solution.cpp"
```

</details>

### Pitfalls this exercise plants — and why they matter

**The documentation trap.** The header states: *"on ANY failure, 'data' is left untouched and nothing is allocated."* That single sentence is what makes `continue` after `ErrNoData` — and `return` after other errors — safe *without* a guard at those points. Miss it, and you either dispose something never allocated (harmless here because `Thing_DisposeData` tolerates zeroed structs — but only because you zero-initialized with `= {}`), or you wrap the guard too early and reason about it wrongly. Vendor docs reward forensic reading; at work, verify such claims with a test before trusting them, because real SDKs are not always this honest.

**The guard placement decision.** The guard is constructed *after* the success check, not before the call. Both placements can be made correct, but they encode different reasoning: guard-after-success relies on the "nothing allocated on failure" contract; guard-before-call relies on dispose-tolerates-empty plus zero-initialization. The reference chooses guard-after-success because it depends on the *documented* contract rather than on incidental tolerance. Being able to articulate which contract your cleanup depends on is exactly the skill real SDK payload-handling requires.

**Zero-initialization is load-bearing.** `ThingData data = {};` makes `values` null before any SDK call. Skip it and the struct holds stack garbage; on the `ErrNoData` path nothing was written, and any later dispose call would `delete[]` a garbage pointer — undefined behavior with no ASan warning until it detonates. The Chapter 2 idiom (`= {}` on every API struct) is not style; it is the difference between "skip path is safe" and "skip path is a time bomb."

**Why `Thing_DisposeData` and never your own `delete[]`.** The exercise states this as a rule and the reason arrives much later, so carry it now: the payload was allocated inside the SDK, and a library and its caller can hold two different heaps — a different C runtime, a different build configuration, sometimes only a different version of the same one. Releasing with your allocator a block that came from theirs is undefined behavior, and the failure surfaces somewhere unrelated, long after the free. That is why every SDK in this book ships a matching dispose function for anything it hands you, and why "it works on my machine" is the *expected* symptom here rather than a reassuring one — your machine is the case where the two heaps happen to be the same one. [Chapter 30](30-authoring-an-abi-boundary.md#chapter-30--authoring-an-abi-boundary) is this rule from the other side of the boundary, where publishing it becomes your job.

**Why the guard is non-copyable and non-movable.** It aliases one struct for one scope. A copy would mean two guards disposing the same payload — double-dispose (the FileHandle argument from Chapter 1). Movability has no use case at this scope and would complicate the invariant. Deleting both is not a limitation; it is the design stated in code.

**The empty-project scenario is a specification question, not a coding one.** Zero Things means `NoErr`, total 0, skipped 0 — the loop simply never runs. The exercise includes it because real plug-ins constantly meet empty selections and empty documents, and "what does success mean on empty input" is a question to settle *before* writing the loop, not after a bug report.
