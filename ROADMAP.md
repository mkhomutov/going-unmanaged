# Roadmap — what the handbook is still missing

This file is the standing answer to "what should I write?". It is a list of
**gaps found by reading the book against the job it claims to prepare you
for**, ranked by how much they cost a reader who hits them unprepared. Each
entry says what is missing, the evidence in the current text, and what a
contribution would concretely look like.

Nothing here is a promise or a schedule. Items get done when someone does the
work and writes it up; an item may also be closed as "deliberately out of
scope" — that decision belongs in an issue, and the entry moves to
[Deliberately out of scope](#deliberately-out-of-scope) below with the reason.

Delivered items stay where they are, marked **DONE** with a pointer to the
chapter that closed them. Item numbers get cited in issues and commit
messages, so they never shift.

**Everything on this list appends.** New chapters go at the end (Chapter 32
onward, in whatever order they land); new appendices continue from E. No item
here requires renumbering, so every one of them is a MINOR release. If you
think an item genuinely belongs *inside* an existing part, open an issue
first — see the numbering rules in [CONTRIBUTING.md](CONTRIBUTING.md).

Before starting a large item, open an issue saying so, so two people don't
write the same chapter twice.

## What earns a place here — and what "done" has to answer

An item makes this list on evidence: the job this book prepares you for
presents the problem, the current text does not cover it, and the entry can
say what it costs the reader who hits it unprepared — that cost is the
ranking. *Is this ours to teach?* is the gate. A subject can be important
C++ and still not belong here, if the SDK-work transition never presents it;
that is what [Deliberately out of scope](#deliberately-out-of-scope) below
is for.

Delivering an item is more than covering the topic. The handbook's goals —
learn, change the mindset, practice, solve real problems — come with a
standing set of questions any new material must answer with a mechanism in
the material itself: which C# reflex it confronts, where the reader feels
that reflex fail, what wrong looks like when it looks like working, what
tells the reader they are wrong, how they find the page from a symptom, what
one habit survives a year later, and what re-verifies every claim when
toolchains move. The full list lives in
[CONTRIBUTING.md](CONTRIBUTING.md#the-questions-every-piece-of-material-answers)
— sketch a contribution against it before writing, because review will read
it against the same list. The chapters that closed items 1–6 are worked
examples of those questions answered; steal their moves.

---

## Tier 1 — the load-bearing gaps

These are places where the book's own scope, or its own promises, are not yet
met. They cost the reader in week one.

### 1. Build systems — DONE

**Delivered:** Chapter 26 — *Build Systems and CMake*, opening the new Part VI.
The gap was that Part IV is called *The Build and the Toolchain* and stopped at
raw `clang++` and `cl` invocations, leaving a reader who meets a
`CMakeLists.txt` on day one with nothing.

The chapter covers why a build system exists at all (header-dependency
tracking — the answer to Chapter 23's breakage 7), CMake as a *generator*
rather than a build tool with `.csproj`/MSBuild as the spine of the
comparison, a worked CMakeLists for the `exercises/buildlab/` Greeter trio,
targets and PRIVATE/PUBLIC/INTERFACE propagation, Debug vs Release, the
handbook's sanitizer flags behind a `-DGREETER_SANITIZE=ON` option,
`find_package` for SDK config packages, and IDE-native project files as the
alternative reality many SDK shops actually live in.

**Still open from this item:** no work, but the coverage has an edge worth
naming. What CI builds is the shape the chapter *ends* on, assembled in
`exercises/buildlab/CMakeLists.txt`: the script configures, builds and runs it
twice — default, then Debug with `-DGREETER_SANITIZE=ON` — and reads the flags
back out of the compile database, because a configure that built and ran proves
the build works and not that the switch did anything. Without cmake on PATH the
step prints SKIPPED and the run stays green; CI passes `--require-cmake`, which
refuses to skip.

The forms the chapter passes *through* on the way there are in no file and are
not checked: the first single-executable `add_executable(greet main.cpp
Greeter.cpp)`, and the sanitizer flags in their pre-refactor shape, on `greeter`
with `PUBLIC` rather than on their own INTERFACE target. Verifying each would
mean a directory of half-finished builds, which teaches worse than it protects —
so the honest claim is that the destination is under CI and the steps toward it
are proofread. Anyone editing those snippets should assume nothing catches them.

That PR also settled where chapter code lives, for the three chapters below that
still owe the same debt — see *Where chapter code lives* in CONTRIBUTING.md.

### 2. Dependency management — DONE

**Delivered:** Chapter 27 — *Dependency Management*. The gap was that "there is
no NuGet" is one of the genuine shocks of the transition, and "how do I add a
library" was a week-one question the handbook could not answer.

The chapter leads with *why* rather than the mechanics, because the reason
explains every strategy that follows: a NuGet package ships IL and runs
anywhere the runtime does, while a compiled C++ library is valid for exactly
one compiler, one standard library, one configuration and one architecture — so
the ecosystem ships source you build yourself, and everything matches by
construction. Then the four strategies (vendored, fetched-and-pinned, package
manager, SDK-provided), why header-only libraries are disproportionately
common, the SDK as the dependency you do not control, and the diamond problem
— where two versions of one library in a binary is a *silent* ODR violation
whose answer changes with link order.

The book's own rule survived intact: the chapter teaches the landscape and adds
no dependency to the repo, and its exercise has the reader write the dependency
themselves so it works offline and stdlib-only.

**Still open from this item:** nothing blocking. If the repo ever does take a
third-party dependency, the vendoring conventions this chapter describes
(record the version and any local patch next to the code) should become a
CONTRIBUTING rule rather than only chapter advice.

### 3. Testing — DONE

**Delivered:** Chapter 28 — *Testing*. The gap was that the feedback loop on
offer was "compile, run, read stdout", for a reader arriving from a world where
xUnit or NUnit is table stakes.

**It took a different route than this item sketched, deliberately.** Rather than
vendoring doctest or Catch2, the chapter has the reader *build* a test framework
in forty lines of standard library, and only then introduces the real ones. The
reason is that the macro machinery is the lesson: C++ has no reflection and no
`[CallerLineNumber]`, so a framework must capture the expression text and the
call site at compile time, and that is *why* every C++ test framework looks the
way it does. Writing one makes the shape obvious; installing one hides it. The
side effect is that the ground-rule question below never had to be answered —
no third-party code entered the repo, and **solutions use the standard library
only** still holds unqualified.

The chapter also lands two things the sketch did not anticipate: testability is
*structural* (the Chapter 15 Buffer cannot be tested where it lives, because a
.cpp with `main()` cannot link into a test binary that has its own — so the
Chapter 26 library split is the precondition for testing anything), and
assertions alone are not enough in C++ — a break that leaves every assertion
passing while ASan reports a double-free, because ownership bugs produce no
wrong values.

**The repo wiring that was open here is now done.** The harness and suite live
at `exercises/testlab/` — `tiny_test.h` and `buffer_test.cpp`, verbatim from the
chapter's listings — and `scripts/build_all.sh` builds the suite under the
canonical flags and runs it, with a non-zero exit as red. The decision it needed
went the honest way: the Buffer was extracted to `solutions/Buffer.h`, so
`solutions/buffer.cpp` keeps the demo's `main()` and the test binary brings its
own, rather than the class being duplicated into the lab. The Chapter 15
solution is two files now, which is the chapter's own lesson applied to this
repository — and the reason `solutions/` may hold a header at all, an amendment
recorded in *Where chapter code lives* in CONTRIBUTING.md.

What that does **not** cover is the chapter's closing demonstration — the
deliberately broken move constructor whose every assertion passes while ASan
reports a double free. Like Chapter 31's sabotage runs it exists to fail, so it
stays book-only by the same rule: a green run would mean it stopped working.

**Still open from this item:** nothing.

The ground-rule analysis below still applies if anyone later vendors a real
framework, so it is kept rather than deleted: **solutions use the standard
library only** (CONTRIBUTING.md), and a vendored framework header would be the
only third-party code the repo carries. The precedent is in `build_all.sh` —
`exercises/buildlab/` is built and run there while being scaffolding rather than
a solution. A test framework is the same kind of thing: the header belongs under
`exercises/`, `solutions/` stays stdlib-only and independently buildable without
it, and the test binary is a third category the script builds to keep it green.
Expect one practical snag — a single-header framework will not survive
`-Wall -Wextra` silently, so include it with `-isystem` and keep the strict
flags meaning what they mean for our code.

### 4. Concurrency — DONE

**Delivered:** Chapter 29 — *Concurrency*. This was the item the book owed most
plainly: Chapter 16 tells you to ask *"what thread calls me back?"*, Chapter 18
answers that silence means "a thread that isn't yours" and names the two
requirements its exercise excludes, and nothing ever supplied the vocabulary.

The chapter maps the C# model onto the C++ one (no runtime, no pool, no
`await`; `std::thread` as a real OS thread; `lock_guard`/`scoped_lock` as RAII
the reader already owns; `Interlocked` versus `std::atomic`), covers
`std::thread`'s join-or-terminate obligation and `jthread`, and works the
callback-from-a-driver-thread problem through to a correct teardown: a control
block with its own lifetime, a weak reference handed to the SDK, an alive flag
published under the lock, unregister — and the context deliberately never
freed, because nothing you write can be sequenced against the SDK's load of
that pointer. Broken and fixed versions were both run under ThreadSanitizer
*and* AddressSanitizer; the lifetime bug is the one only ASan names reliably.

**One thing came out differently than this item sketched.** The plan was to
show that a data race is undefined behaviour rather than a wrong answer, via
the usual demonstration — a racy counter losing updates, or a non-atomic flag
the optimizer hoists into an infinite loop. Neither reproduced on
clang/arm64: no hoist at `-O0`, `-O1` or `-O2`, and the four-thread racy
counter printed exactly the right total on every run, because the optimizer
collapsed each loop into a single addition. That is a sharper lesson than the
one intended — *you cannot find this bug by running the program* — and it is
what the chapter is built around, with TSan finding in one run what the program
would never show. TSan is introduced as the third sanitizer, including that it
cannot be combined with ASan and so needs its own build.

**The lab that was open here is now done**, and with it the carried-over gap
below. Chapter 29's "Try it" was the lab in chapter form and nothing else; it now
has a task card at `exercises/threadlab/` and a reference solution at
`solutions/device_threaded_solution.cpp`, which `scripts/build_all.sh` builds and
runs twice — under the canonical ASan/UBSan flags with everything else, and again
under `-fsanitize=thread` in a section of its own, because the two sanitizers do
not combine. That section follows the cmake precedent with one addition: the
probe compiles *and runs* a trivial instrumented program, since ThreadSanitizer
can be installed and still fail to start, and CI passes `--require-tsan`.

Two things the wiring settled that the chapter only asserts. The reference
solution has to use the file-scope registry the chapter offers in passing —
holders owned at file scope and released after the poller is joined — rather than
the simpler never-freed variant of the listing, because LeakSanitizer runs at
exit on Linux and would report every context as a leak; "do not free it in the
destructor" and "do not leak it" are both satisfiable, but only by that one
ordering. And running the sabotage confirmed the chapter's last pitfall on this
program: `delete ctx_` in the destructor is a `heap-use-after-free` ASan names on
every run and **TSan does not report at all**. One sanitizer is genuinely half
the check here.

**Still open from this item:** nothing. What stays book-only is step 4 of the
*Try it* — the deliberate `delete ctx_` and the removed alive flag — by the same
rule as Chapter 31's sabotage runs: it exists to fail.

---

## Tier 2 — high value, smaller

### 5. A real debugging chapter — DONE

**Delivered:** Chapter 31 — *Reading What the Tools Tell You*. The gap was that
Day 2 of the practice plan tells the reader to "read its reports until they
make sense" while the book never printed one, asking for fluency in a format
they had never seen.

Rather than one annotated report, the chapter leads with report **shapes** as a
diagnostic index — three stacks is a use-after-free, two is a buffer overflow,
one is a leak, none-with-a-column-number is UBSan — so a glance classifies the
bug before a word is read. Then the heap-use-after-free walked line by line:
the question each stack answers, the offset line that names the member without
knowing its name, `inside of` versus `after` as the tell between a lifetime bug
and an indexing bug, and why to start reading at frame `#1`. Watchpoints get a
real lldb transcript, with the reflex stated plainly: stop adding print
statements to find who wrote a value, and note that prints are actively harmful
for Chapter 29's bugs because they change timing.

**Three findings from writing it, all worth keeping:**

- **UBSan reports and exits 0.** Its default is report-and-continue, so a
  script checking only the exit code calls a run with undefined behaviour a
  pass. Both fixes are documented (`halt_on_error=1`, `-fno-sanitize-recover`),
  and `scripts/check.sh` was verified against this — it already sets
  `halt_on_error=1` and is correct.
- **`-O2` deletes the frames you need.** Inlining removes the very functions
  that named who freed and who allocated the block, so diagnose at `-O0 -g`.
- **LeakSanitizer is unsupported on macOS/arm64**, which is what prompted the
  Finding 10 caveat now in Chapter 25.

**Still open from this item:** nothing specific to debugging, and nothing to do
about the code either — that is now a rule rather than a hedge. The chapter's
demonstration programs exist to fail, so unlike the other Part VI code noted
under items 1, 3, 4 and 6 they stay out of `build_all.sh` on purpose: a green
run would mean the sabotage stopped working. *Where chapter code lives* in
CONTRIBUTING.md states it as settled — do not try to make them green.

### 6. Authoring an ABI boundary — DONE

**Delivered:** Chapter 30 — *Authoring an ABI Boundary*. The gap was that the
term `ABI` appeared exactly once in the whole book, unexplained, while
Chapter 16's Bestiary taught the five vendor shapes entirely from the consumer
side — leaving the reader, whose actual job is to ship a plug-in someone else
loads, with no account of the side of the table they are on.

The chapter separates API from ABI, states the one rule that generates every
other (nothing whose layout your compiler chose may cross the boundary), and
derives the corollaries from it: no standard-library types in exported
signatures, no exceptions across the line, whoever allocates frees, and no
inline function that touches private state. Then the three techniques — PIMPL,
pure-virtual interface plus factory, `extern "C"` façade — with a table for
choosing by what callers must match, and how to version a published boundary
including what that leading size field in real SDK headers is for.

Two things it does that the sketch did not ask for. The failure is
*demonstrated* rather than described: adding a **private** member moves
`sizeof` from 32 to 40, and a caller that was not rebuilt under-allocates while
the constructor writes past it — with the wrinkle that the sanitizer only sees
it if the *caller's* translation unit is instrumented, which across a real DLL
boundary nobody does. And the `extern "C"` section closes the book's own loop:
the header the reader derives from first principles is `FakeDevice.h`, which is
why it looked that way since Chapter 18.

**Still open from this item:** nothing. The chapter's three worked boundaries
are now `exercises/abilab/` — `Widget.h`/`.cpp` and `IScorer.h` and `engine.h`
verbatim from the listings, `scorer.cpp` and the rest of `engine.cpp`
completing what the chapter excerpts, and a caller for each — built and run by
`scripts/build_all.sh` under the canonical flags. Each is a separate binary of
*two* translation units, which is the subject matter rather than a build
detail: the caller compiles against the boundary header alone, so what the
header hides is genuinely unavailable to it. The callers assert what the
chapter claims: `sizeof(Widget) == sizeof(void*)`, that `IScorer`'s protected
destructor makes `delete scorer` a compile error
(`!std::is_destructible_v<IScorer>`, checked at compile time), and every
`Engine_*` code the header documents including the null-parameter path.

What stays book-only is the chapter's break-it-first half — the `Naive` layout
break, the pure-virtual method inserted at the top of an interface, the relink
against a changed `Impl`. Each needs a caller binary that was deliberately
*not* rebuilt, and the first two exist to fail, by the same rule as Chapter
31's sabotage runs.

**This was the last of four Part VI chapters carrying the same debt, and the
debt is now closed.** The Chapter 26 PR wrote the convention down (*Where
chapter code lives*, CONTRIBUTING.md — code under `exercises/<lab-name>/`,
wired into `build_all.sh`, SKIPPED locally but never in CI when a step needs a
tool that may be absent) and closed item 1 under it; item 3 closed next, adding
`exercises/testlab/` and the one amendment it required (a header in
`solutions/` when a chapter forces the demo/test split); item 4 followed with
`exercises/threadlab/`, a reference solution, and the convention's first
`--require-<tool>` flag for a sanitizer rather than a build tool; this item
closes it with `exercises/abilab/`, and added one line of its own — where the
separation between translation units *is* the lesson, keep it. Chapters 26, 28,
29 and 30 are all verified on every push, and the convention now stands for
whatever Part VI gains next rather than for a backlog.

### 7. Byte-level protocol work

**Missing:** struct layout, padding, packing, endianness, alignment.

**Evidence:** zero hits for alignment, endianness, and `union`; one for
`sizeof`. Chapter 18 is a *device* lab. Parsing bytes off a wire or a bus is
what device work is, and struct padding plus byte order are the two canonical
bugs — both invisible from C#, where layout is the runtime's problem.

**A contribution looks like:** a short chapter, or a stretch goal on the
FakeDevice exercise, where a packed header struct is parsed from a byte
buffer: what `sizeof` reports and why, what a packing pragma does and costs,
host versus network byte order, and why `reinterpret_cast` over a buffer is a
strict-aliasing trap rather than a clever shortcut.

This item wears item 11's ticket framing naturally — *"parse this capture"*
— and a single contribution can close both entries.

### 8. Consolidated const-correctness

**Missing:** const as one coherent subject.

**Evidence:** it is taught in fragments — const members in Chapter 4, trailing
const and `const char*` versus `char* const` in Appendix A and B, `const auto&`
as a reflex in Chapter 10, const overloads in the Buffer chapter. Every piece
is present and no page assembles them. C# has no equivalent concept at all
(`readonly` is not it), and const shows up in every C++ code review the reader
will ever sit in.

**A contribution looks like:** a chapter that gathers the existing fragments
and adds what is missing — const as an interface contract, const member
functions, `mutable`, and why const-correctness is retrofitted with pain but
free if it is there from the start. Cross-reference rather than duplicate;
the fragments stay where they are.

### 11. Scenario chapters — tickets, not task cards

**Missing:** the bridge between exercise and job. Every exercise in the book
announces its diagnosis before the symptom — the chapter shape opens with
*what it trains*, so the reader always knows which trap is coming. The job
inverts that: work arrives as a symptom attached to code that already
exists, and no page in the book starts from that side.

**Evidence:** the exercise-chapter structure itself (*trains / task /
solution / pitfalls*) — pedagogically right for Part V, and exactly what a
ticket never gives you. Chapter 31 now supplies annotated sanitizer reports
but never hands over an unsolved one. And the first candidate below is a
genuine coverage hole: the static-order fiasco is asserted twice in passing
— Chapter 28's "a namespace-scope vector would be a bet on initialization
order", Chapter 30's "static initialization across modules is not ordered" —
and never demonstrated, while "why does it crash on exit?" is a canonical
month-one ticket that is invisible from C#, where module initialization is
the runtime's problem.

**A contribution looks like:** a Part VI chapter framed as a ticket. The
symptom opens the chapter, the code it happened to lives under
`exercises/<lab-name>/`, the reader is told to diagnose before reading on,
and the walkthrough sits behind a spoiler fold like any reference solution.
Against CONTRIBUTING's questions the load-bearing ones are 9 and 10: the
title and opening lines are what the reader will one day search for, and the
chapter is graded on the verification ritual it transfers, not on whether
the reader reaches the fix. Candidates, each its own chapter:

- **"It crashes on exit"** — static initialization and destruction order
  across translation units. The broken program follows the
  `check_platform_claims.sh` precedent (generated into a temp dir, never
  committed); the fixed shape — Chapter 28's function-local static — is the
  lab's green state.
- **"Here is the report"** — an ASan report plus the source that produced
  it; the reader locates the bug from the report alone, and the fix is
  verified under `scripts/check.sh`. Chapter 31 taught the reading; this is
  the exam.
- Item 7's byte-level protocol work is this framing already (*"parse this
  capture"*), and the carried-over COM refcounting lab is *"the vendor
  upgraded the SDK, and the new API is refcounted"* — one contribution can
  close two entries.

The settled rules hold unchanged: chapters append (32 onward), deliberately
broken programs stay book-only or generated, and everything green is wired
into `build_all.sh`.

---

## Tier 3 — distinctive to this handbook

Material no general C++ book would carry, which is precisely why it belongs
here.

### 9. Going back the other way — C++/C# interop

**Missing:** P/Invoke, marshalling, and the round trip home.

**Evidence:** one passing mention of P/Invoke, nothing on marshalling. A
developer with 17 years of C# behind them, now working against a native SDK,
very plausibly ends up exposing that native code to C# — a test harness, an
internal tool, a UI. This handbook is the one place that reader would look.

**A contribution looks like:** a chapter on the `extern "C"` surface (which
connects to item 6), what marshals cleanly and what does not, string
conversion at the boundary, who owns memory that crosses it, and callback
lifetime when a delegate is handed to native code. The Chapter 22 lambda
lifetime lesson has a direct analogue here, and it bites harder.

### 10. A glossary

**Missing:** Appendix E.

**Evidence:** zero hits for "glossary". The book uses TU, ODR, RVO, ABI and
vtable in the ordinary course of explaining things, and the reader's future
colleagues will use ADL, POD, CRTP and SFINAE without introduction.

**A contribution looks like:** one page, alphabetical, one or two sentences
per term, each pointing at the chapter where the idea actually lives. Terms
the book already uses come first; terms the reader will merely *hear* come
second, marked as such.

### 12. The Rosetta Cookbook — DONE

**Delivered:** Appendix F — *The Rosetta Cookbook*, seeded with Recipes 1–8:
read a file, split, join, build a string, format, time a call, wrap a C
handle in a deleter, and `find` versus the inserting `[]`. The shape this
entry sketched survived contact intact: an index table from the C# name to
the recipe, one strict shape per recipe (**In C# / The recipe / Why it looks
like this / Trap**, the trap a one-line `[!WARNING]`), whys that
cross-reference the owning chapter rather than re-teach, and Finding-style
numbering — recipes append and are never renumbered, so a citation of
"Recipe 7" stays right. The listings compile: `exercises/cookbook/` holds
one translation unit per domain (files, strings, timing, handles, lookups),
each with a `main()` asserting what its recipes claim, built and run by
`build_all.sh` under the canonical flags. It landed as Appendix F with E
left for the glossary, so the letters skip one until item 10 lands. The
collections domain was already indexed — Chapter 11's LINQ table — and the
appendix points there rather than competing.

**One thing the wiring caught.** `build_book.sh` had the appendix letters
hardcoded as `[A-D]` in both its file glob and its cross-file link rewriter,
so the new appendix silently vanished from the single-file build — nav
footers "OK", markup "OK", one file short. Both patterns now read `[A-Z]`,
so the next appendix letter cannot repeat this. The tell was the file count
in the build output; worth a glance on any appendix-adding PR.

**Still open from this item:** nothing blocking — the cookbook is a living
surface like the Findings log, and it grows by PR under the Recipe template
in CONTRIBUTING.md. Candidate next recipes, in likely order of need: write a
file, `DateTime.Now` (timestamps, as distinct from Recipe 6's intervals),
trim and case conversion, `Path.Combine`, environment variables, sleep, and
random numbers — each admitted only under question 11's double filter.

---

## Known gaps carried over

Already recorded in [CONTRIBUTING.md](CONTRIBUTING.md) and still open:

- **A COM-style refcounting lab.** Bestiary Shape 3 is the only one of the
  five shapes without an exercise. Item 11 names it as a candidate ticket —
  *"the vendor upgraded the SDK, and the new API is refcounted."*
- **A threaded-callback lab** — **DONE.** It was a FakeDevice stretch goal, then
  Chapter 29's *Try it*; it is now `exercises/threadlab/` with
  `solutions/device_threaded_solution.cpp` and a ThreadSanitizer step in CI.
  See item 4 above.

## Deliberately out of scope

Nothing yet. Items closed as out of scope move here with the reasoning, so
the same suggestion does not arrive twice.

---

## Structural item (not content) — DONE

Splitting the book into per-chapter files under `book/` with a script that
concatenates them. Worth doing **only if contributor volume justifies it** —
one file is a real feature while the book has few editors, since it makes
grep, cross-reference checking, and the numbering invariant trivial to verify.
Revisit when merge conflicts in `going-unmanaged.md` become a regular event.

**DONE** — "Split the book into per-chapter files": `book/` now holds one file
per chapter and appendix with `book/README.md` as the Contents;
`scripts/build_book.sh` rebuilds the single file (a release artifact, no
longer checked in) and owns the generated nav footers. No chapter number
moved. grep still works across the book with `book/*.md`.
