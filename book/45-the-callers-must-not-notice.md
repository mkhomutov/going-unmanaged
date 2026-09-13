## Chapter 45 — The Callers Must Not Notice

Most of the code you are handed at work already works, has callers in three teams, and must still have them, unchanged, when you are done — a place none of the labs so far started from, and the one the job most often does. The mandate arrives as a ticket, the way it does, and the first thing to notice about it is that the acceptance test is written into the ticket itself.

### The ticket

> **Modernise `Catalog` for 4.0 — without touching the callers.** The native layer's `Catalog` is 2009 code: raw pointers, a hand-rolled array, a manual `Clear`. The 4.0 snapshot feature takes a copy of a `Catalog`, and the feature branch dies at exit under the sanitizers — *heap-use-after-free in `Catalog::Clear`*. The review board's rule for 4.0 is *no owning raw pointers in the native layer*. Three teams' files include `catalog.h`; none of them may change, and none of their behaviour may.

Read the last sentence twice. *None of them may change* is checkable by a build: compile the same caller against the old implementation and the new one. *None of their behaviour may* is the half that is not, and is the half this chapter is about.

### The code it happened to

The header the three teams compiled against, comments included — they are part of what the callers rely on:

```cpp
--8<-- "exercises/retrolab/before/catalog.h:listing"
```

And the implementation behind it, exactly as it has shipped since 2009:

```cpp
--8<-- "exercises/retrolab/before/catalog.cpp:listing"
```

A caller, as one of the three teams wrote it — this file is the acceptance test's other half, and it will not change by a byte between here and the end of the chapter:

```cpp
--8<-- "exercises/retrolab/main.cpp:listing"
```

It works. Under the full canonical flags it is green, it prints six lines, and it has done so for fifteen years. Nothing in the ticket is a bug in this code — until the snapshot feature writes the one line nobody had:

```cpp
Catalog snapshot = live;                  // the feature: a copy to compare against later
```

Against the 2009 class, on this machine, the branch dies at the end of `main` with the report the ticket quoted:

```text
==91837==ERROR: AddressSanitizer: heap-use-after-free on address 0x603000001c60 ...
READ of size 8 at 0x603000001c60 thread T0
    #0 in Catalog::Clear() catalog.cpp:82
    #1 in Catalog::~Catalog() catalog.cpp:12
    ...
    #3 in main snapshot.cpp:61

freed by thread T0 here:
    #0 in _ZdaPv (libclang_rt.asan_osx_dynamic.dylib)
    #1 in Catalog::Grow() catalog.cpp:20
    #2 in Catalog::Add(char const*, int) catalog.cpp:32
    #3 in main snapshot.cpp:38

previously allocated by thread T0 here:
    #0 in _Znam (libclang_rt.asan_osx_dynamic.dylib)
    #1 in Catalog::Grow() catalog.cpp:18
    #2 in Catalog::Add(char const*, int) catalog.cpp:32
    #3 in Catalog::Parse(char const*) catalog.cpp:58

SUMMARY: AddressSanitizer: heap-use-after-free catalog.cpp:82 in Catalog::Clear()
```

### Try it — before reading on

The task card is `exercises/retrolab/TASK.md`; `before/` beside it is the 2009 class, `main.cpp` the caller that may not change, `snapshot.cpp` the 4.0 feature with its own judge, and `after/` the retrofit — no peeking at the last. The ticket is worked one seam at a time, and every seam has the same test:

1. **Read the header for the promises it makes**, in its comments as much as its declarations: what `Find` says about the address it returns, what `Parse` says about malformed input, what `Clear` is for. Write them down. That list, not the declarations, is the contract you are about to keep — and the part of it the compiler cannot check.
2. **Reproduce.** The caller against `before/`, green; `snapshot.cpp` against `before/`, the report above. Read it against [Chapter 6](06-the-rule-of-five-and-move-semantics.md#chapter-6--the-rule-of-five-and-move-semantics): which special member is on the access stack, who wrote it, and what did the `freed by` stack's `Grow` do to the *other* catalog?
3. **Seam 1 — declare what the compiler was writing for you.** Two lines: the copy operations, `= delete`. Rebuild every caller; then rebuild the snapshot branch. One of those outcomes is the point.
4. **Seam 2 — move the ownership inside.** Before you choose the container, predict what `std::vector<Entry>` would do to the caller's `alpha`, then try it, and run the byte-identical caller. Then choose again.
5. **Seam 3 — let the destructor go.** `= default`, with `Clear` still public because callers call it. Say why this seam comes *after* seam 2 and not before.
6. **Seam 4 — earn the copy.** Deep copy, move, copy-and-swap. The snapshot branch compiles, and its judge passes. At every seam: the caller against your current state, its output compared byte for byte with the 2009 output, under the full flags.
7. **Stretch: the boundary.** Suppose `catalog.h` shipped in an SDK and the three teams' binaries could not be rebuilt. Which of the four seams are still allowed? Take the answer to [Chapter 30](30-authoring-an-abi-boundary.md#chapter-30--authoring-an-abi-boundary) and check it.

### The diagnosis, walked through

<details>
<summary>Show the walkthrough — read the report against Chapter 6 first</summary>

The class has two special members it never declared — the copy constructor and copy assignment — and in 2009 the compiler wrote both, member-wise, which for `Entry** entries_` means *copy the pointer*. Nobody copied a `Catalog` for fifteen years, so the shallow copy was never wrong; it was never *run*. The snapshot feature runs it: two `Catalog` objects, one `entries_` array between them. Then the live catalog grows — `Grow` allocates a bigger array and deletes the old one, which is the `freed by` stack — and from that line the snapshot's `entries_` points at freed memory. Its destructor's `Clear` walks it, and that is the access stack. Chapter 6's Rule of Five — its C++03 half, the Rule of Three — violated by omission in 2009, invoiced in 4.0: a class that owns a resource and does not say what a copy means gets a copy that means the wrong thing, on the day someone finally writes one.

Read what the report does *not* say. Nothing in it is in `snapshot.cpp`'s copy line; the crime happened on line 38, in `Add`, where a perfectly ordinary growth freed something a second object still believed it owned. The report names the victim and the freer and never the moment of the shared ownership, because that moment was a compiler-generated function with no source line of its own. That is what "declare what the compiler was writing for you" means: seam 1 gives that function a line, and the line says `= delete`.

</details>

### What the contract actually says

A name for the mandate: **the retrofit** — changing the insides of a class whose *surface* is frozen because other people compiled against it. The surface is more than the declarations. It is everything a caller can observe: the declarations, yes, and so the same `const char*` parameters and `int` returns and the public `Clear`; but also the promises written in the header's comments, and the ones nobody wrote down because the old implementation simply had them. `Find` returns a pointer that stays valid while the catalog grows. `Parse` adds nothing when the text is malformed. A `Catalog` with nothing in it is a valid `Catalog`. None of those appears in a signature, and every one is something a caller somewhere depends on.

> [!NOTE]
> **Surprise for C# devs:** you have done this refactor many times — an interface or a `public` surface held still, the class behind it rewritten — and the compiler held the surface for you, because in C# a caller can observe almost nothing else. Here a caller can observe the *address* of what you hand back, the order in which things are destroyed, and which of copy and move the compiler generated for you. Source that still compiles is a necessary condition. It is not behaviour that still holds, and no keyword says so.

That is why the acceptance test has two halves. The first is a build: the caller's translation unit, byte-identical, compiled against the old implementation and the new one. The second is a run: the two binaries' output, byte-identical, under the full flags — because the byte-identical caller is exactly the one that will die with a use-after-free the moment a seam breaks a promise nobody wrote down. Seam 2 is where that happens. Modernise the storage to `std::vector<Entry>` — the obvious move, one type, no pointers anywhere — and the caller, unchanged, prints its first line and then:

```text
==91866==ERROR: AddressSanitizer: heap-use-after-free on address 0x60d000000060 ...
READ of size 4 at 0x60d000000060 thread T0
    #0 in main main.cpp:30

freed by thread T0 here:
    #0 in _ZdlPv (libclang_rt.asan_osx_dynamic.dylib)
    #1 in std::__libcpp_deallocate<Entry>(...) allocate.h:81
    #2 in std::allocator<Entry>::deallocate(Entry*, unsigned long) allocator.h:120
    ...

SUMMARY: AddressSanitizer: heap-use-after-free main.cpp:30 in main
```

The caller held `alpha` across ten `Add`s, as the header told it it could, and the vector moved every `Entry` on the second one. The caller did not change. Its behaviour did — which is the sentence the ticket ends on, and the reason the container is `std::vector<std::unique_ptr<Entry>>`: the vector may move, the entries do not, and the 2009 promise survives a change of storage it was never told about.

And one boundary the retrofit does not cross. Everything above assumes the three teams *recompile* — source compatibility. The day `catalog.h` ships in an SDK and the callers' binaries cannot be rebuilt, `sizeof(Catalog)` is part of the contract too, and the seam that moves the storage changes it — the one every later seam is built on. That is [Chapter 30](30-authoring-an-abi-boundary.md#chapter-30--authoring-an-abi-boundary)'s subject, and its answer is the one seam that never moves again: a pointer to an implementation, behind which this whole chapter can happen without a caller relinking.

### The fix, seam by seam

Four seams, each a commit, each green against the unchanged caller before the next begins. That discipline is the deliverable: not the final class, which any of the earlier chapters could have written from scratch, but the *path* to it through states that never break the three teams.

**Seam 1 — declare what the compiler was writing for you.** Two lines in the header, and nothing else:

```cpp
    Catalog(const Catalog&) = delete;
    Catalog& operator=(const Catalog&) = delete;
```

Every caller still compiles, because none of them copied — that is the build proving the claim rather than the author asserting it. The snapshot branch now fails to *compile*, at the copy line, with a message that names the deleted function: a crash at exit on a customer's machine has become a compile error on the developer's, which is the largest single improvement in the whole ticket and it cost two lines. [Chapter 6](06-the-rule-of-five-and-move-semantics.md#chapter-6--the-rule-of-five-and-move-semantics)'s advice — if you cannot yet say what a copy means, forbid it — applied to code that had been silently permitting one for fifteen years.

**Seam 2 — move the ownership inside, keeping the address promise.** The private section changes, the public one does not:

```cpp
--8<-- "exercises/retrolab/after/catalog.cpp:add"
```

```cpp
--8<-- "exercises/retrolab/after/catalog.cpp:find"
```

`Grow` is gone — the vector grows itself — and `Find` returns `e.get()`, a borrowed pointer into an `Entry` that the vector's growth never moves. That is the whole reason for the `unique_ptr` per entry rather than the entry by value, and it is a decision made *for the caller* rather than for the class.

**Seam 3 — let the destructor go.** `~Catalog() = default;` — after seam 2, never before it: a defaulted destructor over a raw `Entry**` is a leak of every entry, and the sanitizers on this platform would not have said so (Chapter 31's macOS note). `Clear()` stays public and stays a member, because callers call it; it just is not the destructor's job any more.

**Seam 4 — earn the copy.** Now that the storage owns, a correct copy is two short functions, and the move operations are the compiler's for the asking:

```cpp
--8<-- "exercises/retrolab/after/catalog.cpp:copy-and-move"
```

The header after all four, every public declaration where it was and the special members it always needed finally declared:

```cpp
--8<-- "exercises/retrolab/after/catalog.h:listing"
```

The judge for the feature, and for the retrofit's own promises — a copy that shares nothing, a move that leaves a valid empty source, a self-assignment that changes nothing, and the address promise carried across the change of storage:

```cpp
--8<-- "exercises/retrolab/snapshot.cpp:listing"
```

`build_all.sh` runs the acceptance test as the ticket wrote it: the caller above, unchanged, built against `before/` and against `after/`, both under the full flags, and the two outputs compared byte for byte; then the snapshot judge against `after/`. The caller's file being identical is true by construction. The caller's *output* being identical is the claim, and it is the one the value-storage seam would have failed.

### Pitfalls

- **Modernising the storage to values.** `std::vector<Entry>` is the cleanest type and the wrong retrofit: the 2009 promise was address stability, and every caller holding a pointer across an `Add` dies without a line of theirs changing. The container that keeps a promise nobody wrote down is chosen by reading the callers, not the class.
- **Defaulting the destructor before the storage owns.** A `= default` over a raw `Entry**` leaks every entry, silently on macOS. Seams have an order: ownership moves inside first, then the manual cleanup goes.
- **Improving the surface while you are in there.** `const char*` to `std::string_view`, `int` to `size_t`, `Find` to `std::optional` — each a better API, each a change to what three teams compiled against, each out of scope by the ticket's first sentence. Write them on the list for 5.0 and put nothing in the header a caller can see.
- **Adding an include to the header that callers did not have.** `<vector>` and `<memory>` now reach three teams' translation units. Usually harmless; occasionally a name collision or a build-time change they will notice. It is a change to the surface, however small, and it goes in the commit message.
- **One big rewrite.** The end state is easy; the ticket is the path. A seam is the unit — small enough that when the byte-identical caller's output changes, there is one thing it could have been.
- **Stopping at "it compiles".** Source compatibility is what the compiler checks. The caller's output under the sanitizers is what the ticket asked for, and the difference between the two is a customer's crash report with your name on the retrofit commit.

> [!TIP]
> **Key principle:** "I modernise a working class one seam at a time — declare what the compiler was writing for me, move the ownership inside, then earn the copy — with the callers' files untouched and their output byte-identical at every seam, because source that still compiles is not behaviour that still holds."

### In the wild

The rule-of-zero retrofit is the commonest engineering task in a codebase that predates C++11, and the industry's tooling grew around exactly the seams above. The standard has deprecated generating a copy for a class with a user-declared destructor since C++11, and clang's `-Wdeprecated-copy-with-dtor` (GCC: `-Wdeprecated-copy-dtor`) says so — seam 1 as a diagnostic, and outside `-Wall -Wextra`, which is why the 2009 class is green under the canonical flags. clang-tidy's `cppcoreguidelines-owning-memory` names every owning raw pointer for you — seam 2's inventory, mechanically, and without knowing which addresses your callers hold. LLVM's and Chromium's C++11 migrations were incremental and gated on a build of every caller; where Chromium rewrote by tool, the tool ran over the whole tree in one change, because a build of every caller was the only acceptance test there was — "callers unchanged" is a claim a build makes, at any scale. And the boundary the chapter stops at is where a whole ecosystem lives: Qt's d-pointer is Chapter 30's PIMPL applied to every public class so that its insides can change for the life of a major series — a binary built against 5.0 in 2012 still loads against 5.15, and each new major is the one place the ABI is allowed to break — which is the retrofit at the scale of a framework, and the reason that, when the teams *cannot* recompile, the first seam is the one that moves everything behind a pointer and the rest of this chapter happens on the far side of it.
