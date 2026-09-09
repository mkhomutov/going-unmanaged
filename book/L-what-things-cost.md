## Appendix L — What Things Cost

[Chapter 36](36-the-host-stutters.md#chapter-36--dropouts-with-the-plug-in-loaded) is this book's performance chapter and it teaches one thing: how to read cost evidence without being lied to by an average. What it does not do — what nothing here does — is answer the question that arrives first, mid-task, when a review comment says *that will be slow* and you have thirty seconds to decide whether it will. The costs are in the book, taught eight places apart and indexed in none. This page is that index, in [Appendix G](G-the-bridge-catalogue.md#appendix-g--the-bridge-catalogue) and [Appendix J](J-cmake-catalogue.md#appendix-j--the-cmake-catalogue)'s shape: the thing, what it costs, how you would find out, and the page that owns it.

Two rules before the table, because the table is useless without them.

**Measure before you change anything.** Every row below is a *shape*, not a number for your machine: the ratios move with the compiler, the standard library, the allocator, the architecture and the flags, and the one honest number is the one you took. What a page like this is for is knowing *which* thing to measure and *with what*.

**Pick the instrument from the complaint.** [Chapter 36](36-the-host-stutters.md#chapter-36--dropouts-with-the-plug-in-loaded)'s whole point: a sampling profiler answers *where did the mean go*, and a complaint about a click every few minutes is not about the mean. A counter answers *how many*, exactly, every run. A profile can acquit code that is guilty; a counter cannot.

---

### The three instruments this repository owns

All three are small enough to paste into a lab and are checked on every push.

| Instrument | What it answers | What it cannot see |
|---|---|---|
| The copy/move counter — [`exercises/choosing/counted.h`](https://github.com/mkhomutov/going-unmanaged/blob/main/exercises/choosing/counted.h), [Appendix H](H-choosing.md#appendix-h--choosing-signatures-containers-and-storage) | How many copies and how many moves one signature costs against another. Deterministic, and a design answer rather than a timing | Allocation. A move that steals a pointer and a move that allocates count the same |
| The allocation counter — a replaced `operator new`, [`exercises/perflab/main.cpp`](https://github.com/mkhomutov/going-unmanaged/blob/main/exercises/perflab/main.cpp), [Chapter 36](36-the-host-stutters.md#chapter-36--dropouts-with-the-plug-in-loaded) | How many times this code reached the heap. The judge for anything on a deadline, because the answer is a count and not a duration | *Which* allocation, and what it cost. Also nothing about stack growth or page faults |
| A sampling profiler — [Chapter 31](31-reading-what-the-tools-tell-you.md#chapter-31--reading-what-the-tools-tell-you)'s section | Where the time went **on average**, in the optimized build, with the real workload | The tail. A stall once a minute is invisible to a sampler that reports percentages |

> [!TIP]
> **Key principle:** "I pick the instrument from the complaint — a sampler for the mean, a counter for the count, a trace for the tail — and I take the measurement before I change anything, because a cost I reasoned about is a guess with a number on it."

---

### What things cost

| The thing | What it costs | How you would find out | Owned by |
|---|---|---|---|
| A `std::string` copy | Nothing at all below the small-string threshold — around 15 to 22 bytes, unstandardised, invisible in the type — and a heap allocation plus a byte copy above it | Allocation counter, on a string built deliberately past the threshold | [Chapter 9](09-casts-conversions-and-strings.md#chapter-9--casts-conversions-and-strings), [Appendix H](H-choosing.md#appendix-h--choosing-signatures-containers-and-storage) |
| A missing `&` in a range-`for` | A full copy per element, per pass, silently | Copy/move counter, or a profile with a copy constructor under your own frame | [Chapter 2](02-value-semantics.md#chapter-2--value-semantics), [Chapter 36](36-the-host-stutters.md#chapter-36--dropouts-with-the-plug-in-loaded) |
| `vector` growth | Amortized O(1), paid as a reallocation that moves every element and invalidates every iterator, pointer and reference into it | `capacity()` before and after; the allocation counter over the loop | [Chapter 11](11-stl-containers-and-algorithms.md#chapter-11--stl-containers-algorithms-and-iterator-invalidation), Recipe 27 |
| A `unique_ptr` | The same code as correct manual `new`/`delete`: a null check in the destructor, and a by-value parameter passed in memory rather than a register | Read the disassembly, or trust Chapter 1's paragraph | [Chapter 1](01-ownership-and-raii.md#chapter-1--ownership-and-raii) |
| A `shared_ptr` copy | An **atomic** increment, and a decrement on the way out — contended between threads, and never free | Allocation counter for the control block; a profile for the contention | [Chapter 1](01-ownership-and-raii.md#chapter-1--ownership-and-raii) |
| A virtual call | One indirection, and no inlining *unless* the compiler can prove the dynamic type — which it often can, for a local, a `final` class, or under LTO | Disassembly, or a profile where the callee never appears inlined | [Chapter 5](05-virtual-dispatch-and-the-virtual-destructor.md#chapter-5--virtual-dispatch-and-the-virtual-destructor) |
| Entering a `try` block | Nothing. Table-based exceptions emit no instructions on the success path | — | [Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes) |
| A `throw` | Thousands of times a plain return, and — worse — a *variable* number of microseconds, scaling with the frames and destructors between the throw and the catch | A profile shows it as a timing outlier, never as a wrong answer | [Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes) |
| Checking an error code | A compare-and-branch at every call site, on every call, whether or not anything fails | — | [Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes) |
| A heap allocation on a deadline thread | A lock, a free-list walk, and a tail nobody sampled — 33 per tick was a click every few minutes in a real ticket | Allocation counter. **Not** a timing: the mean barely moved | [Chapter 36](36-the-host-stutters.md#chapter-36--dropouts-with-the-plug-in-loaded) |
| A local that does not fit the stack | A crash on *entry* to the function, before its first line — and the frame is 512 KB on a macOS worker thread, 1 MB on Windows | It does not need finding; it needs `sizeof` and Chapter 3's table | [Chapter 3](03-stack-heap-and-undefined-behavior.md#chapter-3--stack-heap-and-undefined-behavior), Recipe 34 |
| `std::regex` | Enough that the book says not to put it on a per-sample path at all | Allocation counter — it allocates freely | Recipe 44 |
| Reading a file with `ReadAllText` | Heap the size of the file, and every byte read before the first is looked at | Allocation counter, against Recipe 49's mapped view | Recipe 49 |
| Building with the sanitizers | Roughly twenty times, and **not uniformly** — the factor differs by code shape, so a sanitized comparison of two designs is not a comparison | Time both at `-O2` without them; that is the only comparison that means anything | [Chapter 36](36-the-host-stutters.md#chapter-36--dropouts-with-the-plug-in-loaded), Recipe 28 |

---

### What stops paying for an allocation

The one row above with no answer yet is the allocation itself. Chapter 36's fix was to stop making a copy; when the allocations are *necessary* and merely too many, the answer is to buy them in bulk. A **bump allocator** takes one block and hands out slices of it:

```cpp
--8<-- "exercises/cost/allocating.cpp:arena"
```

The trade is in the missing function: there is no `Free`. An arena gives back everything at once or nothing at all, which fits a frame, a request, a parse or a document load, and does not fit a cache. When it fits, the count stops depending on how many objects there are, and `exercises/cost/allocating.cpp` measures exactly that on every push:

```text
   501 allocations   500 nodes, one make_unique each
     1 allocation    500 nodes, one Arena
     0 allocations   500 nodes, pmr over a stack buffer
```

The third line is the standard library's own spelling, and it is in every reader's toolchain already: **`std::pmr`** (C++17, `<memory_resource>`). A `std::pmr::vector` takes a *memory resource* rather than an allocator type, so the choice is a constructor argument instead of part of the type — `std::pmr::vector<Node>` is one type however it allocates, where `std::vector<Node, MyAlloc>` is a different type from `std::vector<Node>` and will not pass to a function expecting one. `monotonic_buffer_resource` over a stack buffer is the arena above with nothing left to write, and passing `null_memory_resource()` as its upstream turns "the buffer was too small" from a silent heap allocation into a `std::bad_alloc` you can see.

> [!WARNING]
> **Trap:** an arena's objects are destroyed only if you destroy them — placement `new` does not register a destructor, and freeing the block does not run any. For trivially destructible types that is fine and is most of what arenas hold; for anything owning a resource it is a leak the allocation counter cannot see, because nothing was allocated to count.

---

### The cost that hides in the layout

Two counters written by two threads, adjacent in one struct, share a cache line — and each write invalidates the other core's copy, so two threads touching two *different* variables pay for a shared one. This is **false sharing**, and it is the cost that never appears in a profile as itself: the subtree is simply slower than its arithmetic can explain.

```cpp
--8<-- "exercises/cost/allocating.cpp:false-sharing"
```

The fix is padding, and it is a *size* decision made deliberately: `sizeof(Adjacent)` is 16 and `sizeof(Separated)` is 128 on the machine this was written on, so separating two `long`s cost 112 bytes. Worth it for two hot counters in a queue; not worth it for a member nobody contends.

And the constant is the part worth carrying, because everybody types 64. C++17 standardised `std::hardware_destructive_interference_size` for exactly this, and on the Apple-silicon machine this page was measured on it reports **256**, not 64. Two consequences: a hardcoded 64 does not separate anything on that hardware, and GCC warns if you use the standard constant in a type that crosses a binary boundary, because its value may change between compiler versions and take your struct's layout with it — [Chapter 30](30-authoring-an-abi-boundary.md#chapter-30--authoring-an-abi-boundary)'s rule, arriving from an unexpected direction. Pick a constant, write it down in one place, and treat it as an ABI decision rather than a tuning one.

---

### Reading a cost claim

The last thing this page is for is the moment somebody hands you a number. [Chapter 36](36-the-host-stutters.md#chapter-36--dropouts-with-the-plug-in-loaded) is the worked example — a profile that was genuine, professionally taken, correctly summarized, and answered a question nobody had asked. Four questions to put to any measurement, yours included:

1. **What was built?** An unoptimized profile is a profile of a different program, and a sanitized one is a profile of a slower, differently-shaped program. Chapter 31's checklist.
2. **Mean or tail?** A sampler's percentages describe the average. A complaint about a stutter, a dropout or a spike is about the tail, and the two can disagree completely without either being wrong.
3. **Against what?** A number with nothing beside it is not evidence. The 501 above means nothing until the 1 sits under it.
4. **Would this claim survive a counter?** Where a count is available — allocations, copies, moves, calls — it beats a duration, because it is the same on every machine and does not need the workload to be reproduced.

> [!TIP]
> **Key principle:** "A cost claim without a baseline, a build description and an instrument is an opinion. I would rather have a count I can reproduce than a duration I cannot."
