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

**Everything on this list appends.** New chapters go at the end (Chapter 39
onward, in whatever order they land); new appendices continue from I. No item
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

**A second evidence source (2026-09-02): the reader study.** Items 18-20
below were not found by reading the book against the job — they were found
by reading it *as* eighteen different readers, each given the repository
cold, sent wherever their own problem took them rather than down the
Contents page, and required to cite a file for every claim. Where an item
below cites "the reader study", the evidence is that several readers
reached the same conclusion independently, and the count is stated. Two
cautions for anyone mining it further. First, the personas were
constructed, so a *single* persona's complaint is a hypothesis and only
convergence is evidence — every item here has at least two. Second, the
study's larger yield was corrections and cross-references, not new
chapters; those move no numbers, need no entry here, and are tracked as
issues under the `correction` label. Only the three gaps that need writing
became items — a fourth, the managed-runtime-owns-the-process topology, is
recorded as a scope note on item 9 rather than an item of its own.

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

**Still open from this item:** nothing in the chapter's own scope. What stays
book-only is step 4 of the *Try it* — the deliberate `delete ctx_` and the
removed alive flag — by the same rule as Chapter 31's sabotage runs: it exists
to fail. What the chapter *promises* and never builds is filed separately, as
item 16: the main-thread queue named in its *In the wild* bullet, and the
reentrancy deadlock named in the bullet under that one.

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

### 7. Byte-level protocol work — DONE (Chapter 34)

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

**Delivered as Chapter 34 — *Parse This Capture* — the third ticket
chapter, closing this item and an item 11 candidate in one contribution,
exactly as the paragraph above predicted.** Every element of the sketch is
in it: `sizeof` 12 against the ICD's 8 with the `offsetof` table beside the
document's, `#pragma pack(1)` staged as the half-fix that turns scrambled
values into mirrored ones (padding scrambles, endianness mirrors — the
first bug hides the second), the strict-aliasing illegality of the overlay
stated with the honest caveat that **no sanitizer checks it** — the
chapter's distinct lesson is that the canonical flags stay green on all
three of its bugs, so the oracle is the ICD plus a hand decode of the
attached capture. The lab is `exercises/capturelab/` (fixed state; the
broken overlay parser is book-only), asserted by `build_all.sh` against
the hand-decoded values, with the capture's second frame deliberately
unaligned. See item 11's delivered notes for the ticket-format details.

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

**Sequencing note (2026-08):** still worth doing and still open — but item 9
(P/Invoke) now comes first; see the note there for why the deep review
re-ordered them.

**And read item 17 before starting this one (2026-09).** They are
neighbours: `const&` is one branch of item 17's parameter procedure, and
const-correctness is the subject that branch belongs to. If item 17 lands
first this item can point at the procedure instead of re-deriving it, which
is an argument for taking that one first — a second deferral for this
entry, and the reason to plan the two together rather than in sequence by
accident.

### 11. Scenario chapters — tickets, not task cards — DONE (Chapters 32–35)

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
  lab's green state. **Now Chapter 32; see below.**
- **"Here is the report"** — an ASan report plus the source that produced
  it; the reader locates the bug from the report alone, and the fix is
  verified under `scripts/check.sh`. Chapter 31 taught the reading; this is
  the exam. **Now Chapter 33; see below.**
- Item 7's byte-level protocol work is this framing already (*"parse this
  capture"*), and the carried-over COM refcounting lab is *"the vendor
  upgraded the SDK, and the new API is refcounted"* — one contribution can
  close two entries. **Item 7 landed as Chapter 34, and the COM lab as
  Chapter 35 — the candidate list is closed.**

The settled rules hold unchanged: chapters append (36 onward now),
deliberately broken programs stay book-only or generated, and everything
green is wired into `build_all.sh`.

**Delivered so far:** Chapter 32 — *It Crashes on Exit* — the first ticket,
in exactly this shape: the symptom opens the chapter, the broken 2.4.1 code
lives in `exercises/exitlab/TASK.md` for the reader to recreate cold, the
diagnosis sits behind a spoiler fold, and the committed files are the fixed
state, which `build_all.sh` builds **twice with the translation units in
opposite orders** and runs both — order-independence is the fix's whole
claim, and one build cannot prove a claim about two. Two things the writing
verified the hard way, both kept in the chapter's pitfalls: libc++'s
container annotations un-poison a freed block on `push_back`, so the first
draft's vector-based logger crashed at exit and ASan said *nothing*
(Finding 10's file grows); and libc++'s `unique_ptr` nulls its pointer
during destruction where libstdc++ leaves it stale — so the lab's logger
holds a raw `char*`, keeping the demonstration one mechanism on both CI
platforms.

Chapter 33 — *Here Is the Report* — is the second, and it delivers the
"Here is the report" candidate above in the same shape with one inversion:
the sanitizer report arrives *with* the ticket (a nightly job nobody read),
and the reader's first job is the diagnosis on paper — no compiler until it
is written down. The committed lab (`exercises/reportlab/`) is the fixed
state, run by `build_all.sh` at **0 hot-plugs and at 100** because
growth-independence is the fix's whole claim. The writing verified its two
tempting non-fixes before asserting them, and both are kept as pitfalls:
storing a copy is sanitizer-clean and wrong from the first frame (the copy
freezes at pin time — value semantics, not a lifetime bug), and
`reserve(16)` runs clean at nine sensors and produces the identical report
at seventeen.

Chapter 34 — *Parse This Capture* — is the third, delivering item 7 inside
the format (one contribution, two entries closed, as both items predicted).
Its inversion: the toolchain goes silent. The first two tickets end with a
sanitizer naming the crime; here the canonical flags stay green on all
three bugs — padding, byte order, aliasing — and the attached evidence is
a twenty-byte bus capture plus the vendor's header table, so the oracle is
the reader's own hand decode. The committed lab (`exercises/capturelab/`)
is the fixed state, asserted by `build_all.sh` against the hand-decoded
values, with the capture's second frame deliberately off every four-byte
boundary. The staged half-fix was verified before being asserted:
`#pragma pack(1)` makes `sizeof` match and turns every multi-byte value
into its mirror image — and silences the one report UBSan had, since a
packed struct's alignment requirement drops to one.

Chapter 35 — *Still Live at Unload* — is the fourth and last of the listed
candidates, closing this item and the carried-over Bestiary Shape 3 gap in
one contribution. The vendor from Chapter 17 ships SDK 2.0 (a new vendor
drop, `exercises/comlab/FakeSDK2.*` — version 1.x's files did not change),
the mechanical port misreads the ownership convention in both directions
at once, and the two bugs cancel on one object: the host's counter says
**4 of 5** still live, and the object the port mistreated worst is the one
the counter cannot see. The staged patch was verified before being
asserted: fixing the leak alone makes the plain build print 0 and exit 0 —
it looks completely fixed — while promoting the over-release crash from
"two customers, sometimes" to every close, with the culprit line sitting
ON the report's freed-by stack (the deliberate inversion of Chapter 33).
The fix is a type, not a patch: `ref.h`'s adopt/share wrapper, held by
`build_all.sh` to **two judges at once** — the vendor's live-object
counter for a release too few, the sanitizers for a release too many.
This item is now DONE as listed; the ticket *format* stays open — new
scenario chapters arrive by PR against the CONTRIBUTING questions, graded
as ever on questions 9 and 10. (Items 14 and 15 have since appended
Chapters 36 and 37 in exactly this format — the performance ticket and
the crash-dump ticket, each with an inversion of its own.)

### 14. The performance ticket — DONE (Chapter 36)

**Missing:** performance and profiling, anywhere — the 2026 deep review's
competitive pass found zero hits for profiling or benchmarking, no
remaining roadmap item covering them, and called it the largest genuine
content gap against the reader's actual job: a C# developer arrives with a
dotTrace/BenchmarkDotNet reflex and the assumption "C++ is fast", then
meets accidental copies and allocation in hot paths, and "the plug-in
makes the host laggy" is a canonical month-two ticket. The `const auto&`
reflex was taught (Chapter 10) but never measured.

**A contribution looks like:** a ticket-format chapter — the host stutters,
profiler evidence attached in Chapter 33's inversion, an accidental copy
in a hot callback as the diagnosis, and a fix verified by an asserted
timing/allocation-count harness.

**Delivered as Chapter 36 — *The Host Stutters* — with one honest turn the
sketch did not predict.** Building the lab and measuring it showed the
accidental copies barely move the *mean* (~15–20% on the maintainer's
machine — a 4 KB block copies through a warm cache in microseconds), so a
chapter claiming "the copy made it slow" would have failed its own
verification standard. What the copies actually break is the *deadline*:
thirty-three allocator calls per tick on the host's real-time thread,
where malloc's lock and unbounded worst case turn a 21.3 ms budget into
occasional 24–26 ms misses no sampling profiler can catch in the act. So
the ticket became the first whose attached evidence appears to *acquit*
(support's percentages are correct and irrelevant), the fix stayed the
promised two ampersands, and the acceptance harness asserts the one number
that is a claim about every tick at once: **zero heap allocations**,
counted by a replaced `operator new`, run by `build_all.sh` at two session
lengths (`exercises/perflab/`). The C# bridge writes itself: the reader
already owns this discipline as "avoiding GC pressure"; only the spelling
of the allocation is new. One new Appendix B group (Deadline code),
mirrored in the same commit.

### 15. The crash-dump ticket — DONE (Chapter 37)

**Missing:** post-mortem debugging. Every ticket's evidence was
sanitizer-era and developer-side, while the artifact that actually arrives
from the field in SDK work is a crash report, core file, or minidump: a
Release-build stack of raw addresses, symbols to be supplied by you, no
sanitizer anywhere, and no repro. The deep review called it the natural
sequel to Chapter 31 and the last gap between the ticket arc and the real
inbox.

**A contribution looks like:** a ticket-format chapter — customer crash
report attached, no repro on the bench; symbols and symbolication, a
Release stack with a frame inlined out of existence, the fault-address
arithmetic, and the honest limits of post-mortem evidence.

**Delivered as Chapter 37 — *No Repro, Dump Attached* — every artifact in
it generated from a real crash on the maintainer's machine.** The fault
address is `0x10` — null plus `offsetof`, the member named from the
address before any tool runs (Chapter 33's arithmetic with the sign
flipped); naive symbolication of the crash PC names `Report()` at a line
that *cannot fault*, because the guilty helper was inlined — verified: the
debugger reconstructs it as a bracketed `[inlined]` frame, `atos -i` /
`addr2line -i` expand it, and the stripped binary shows the customer's
two-anonymous-frames view. The bug is capability-shaped (the calibration
pack exists only on bench units — the *bench bias*), so the fix's
acceptance is a configuration matrix: `build_all.sh` runs
`exercises/dumplab/` under both device configurations, because the crash
lived only in the one the matrix never had — and the canonical flags name
the bug's exact line in that configuration, which is the chapter's
coverage lesson. Two new Appendix B principles under Debugging, mirrored
in the same commit.

### 17. Choosing — signatures, containers, and what goes inside them — DONE (Appendix H)

**Missing:** the decision procedure for the highest-frequency choice in the
language. Which container; how to take a parameter; what to return; and
whether the elements go in by value or behind a pointer. Every function the
reader writes asks all four, and exactly one sub-branch of one of them —
what to hand back when the call can fail, Chapter 8 — is anywhere answered
as a *choice*.

**Evidence:** the fragments are everywhere, and almost every one of them is
an *instance* rather than a decision. Chapter 11 has a C#-to-C++ container
**translation table** — which container matches `Dictionary`; its Notes
column ranks two rows ("your default, 95% of the time", "almost never the
right choice") and leaves the others unranked, while the stability rules
that would decide those sit two sections later as a paragraph of prose under
"THE trap". Appendix A.5 has the classic triad (`Widget`, `Widget&`,
`const Widget&`) in three lines and stops; the other six shapes of a
parameter are scattered or absent. The sink is named twice and situated
never: Chapter 6 as "sink params take by value + move" in one line of a code
comment, Chapter 14 as a pitfall bullet on the Tracer's own constructor — a
definition with no procedure around it. Chapter 10 gives `string_view` as
the replacement for `const std::string&` and warns it can dangle in the very
next sentence; the two sit adjacent as facts and never resolve into the rule
that separates them — fine as a parameter, never as a member. Chapters 2 and
20 establish `vector<unique_ptr<Shape>>` as *the* polymorphic container on
the strength of slicing, while Chapter 33 independently teaches that a
`vector<std::unique_ptr<Sensor>>` holds its elements still across a
reallocation — the same recommendation, a completely different reason, and
no page that separates them. Returning is the most scattered: Chapter 8 owns
the fail-able branch outright — the bug/value/event decision, with ten
scenarios to run on paper — while Chapter 6 has the elision rules and
Chapter 33 the loan sentence for a returned pointer, and nothing assembles
the three into "what should this function hand back".

The cost is that the C# reflex has exactly **one** answer to all four
questions — pass the reference, return the reference, put objects in the
`List`, and let the collector sort out the rest — and it is wrong in four
different ways here. Two of them are silent: the accidental copy, which this
book's own Chapter 2 calls "one of the most common real-world C++ bugs", and
the view that outlives what it views. A reader who has finished all
thirty-eight chapters still cannot sit down and derive a signature.

**The spine, and why this is one page and not three.** The three surfaces
are one question asked at three scopes: *who owns this, how long does it
live, and who may see it?* A parameter is a loan for the duration of the
call — or a transfer, if it is a sink. A return is a new object — or a loan
whose term the caller cannot see. A container element is ownership for the
container's lifetime, **plus a promise about address stability**. Chapter 33
already coined the vocabulary for the middle case ("the loan sentence is
everywhere once you look for it"); this item generalizes it to the other two
and gives the reader one question to ask instead of four unrelated habits.
That question is also the habit the page has to leave behind, and the key
principle Appendix B mirrors in the same commit — speakable, in the
handbook's own first person: "Before I write a signature I ask who owns
this, how long it lives, and who may see it." A lookup page trains the
instance and not the habit unless one sentence survives being looked up
(question 8), and four decision procedures are exactly the shape most at
risk of failing it.

**A contribution looks like:** an appendix at the next free letter (**H**
today, provisional for the reason no chapter number is pre-assigned), built
as four decision procedures. Each is a compact `flowchart LR` trunk, a table
of the branches naming the C# reflex each one confronts, a use case and a
sentence of *why* per branch cross-referencing the chapter that owns the
mechanism, and one trap. The four: **which container** (lookup by key →
ordered → must addresses hold still), which absorbs Chapter 11's table by
reference and promotes that chapter's prose invalidation rules into the
stability column the table never had — and, while there, repairs the two
chapters that already cite a Chapter 11 "invalidation table" (Chapter 21)
and "gentler column" (Chapter 33) which do not exist; **how to take a
parameter**, whose **first** test is polymorphism, because asking the sink
question first routes a stored polymorphic argument into a by-value
parameter and slices it — the bug Chapters 2 and 20 exist to teach
(polymorphic and the function keeps it → `unique_ptr<Base>` by value;
polymorphic and it does not → `const Base&`; the function keeps a copy →
sink, by value and `std::move`; read-only contiguous → `span`, C++20, or
`string_view`; possibly absent → `const T*`, or `T*` where it is written
through; modifies the caller's object → `T&`; cheap to copy → by value;
otherwise `const T&`); **what to return** (fail-able → `optional`, or
`expected` where the codebase is C++23 — the branch Chapter 8 already owns,
so the page routes to it rather than restating it; polymorphic →
`unique_ptr<Base>`; a view into something you own → write the loan's term in
the header; otherwise by value); and **what goes in the container**
(polymorphic → `unique_ptr<Base>`; addresses must survive growth →
`unique_ptr<T>` or a node-based container; huge or immovable →
`unique_ptr<T>`; genuinely co-owned → `shared_ptr`, and justify it;
otherwise `T` by value) — three independent reasons for the same shape,
which is precisely what the book currently never separates.

**Why an appendix rather than a chapter**, since the subject is Part I–III
material. Two reasons, and the second decides it. The moment of need
(question 1) is the keyboard: this is consulted while writing a signature,
not read once in order — which is question 2 answered, and it is why the
page is built as procedures rather than narrative. The diagrams stay
**additive**, as CLAUDE.md requires: each trunk illustrates a procedure
already complete in its table, because mermaid does not render in the
release single file and the reader who downloads that file must still get
the whole page. And numbering is load-bearing, so a chapter would have to
append as 39, landing value-semantics material after six ticket chapters
and a bridge design chapter; an appendix carries no positional claim. The fragments **stay where
they are**, as in item 8: the page gathers and cross-references, and the
owning chapters gain a pointer to it so the reader in Chapter 11 wondering
which container to use is told the procedure exists.

**What re-verifies it:** `exercises/choosing/`, stdlib-only, wired into
`build_all.sh`, with its pairing added to `check_verbatim.sh` in the same
commit — Appendix F's discipline, and that script is what enforces it
(listings quoted verbatim, so editing one means editing the appendix in the
same commit).

The recommendations become asserted numbers, and that needs one thing the
book does not yet have. Chapter 14's Tracer is the right *shape* and the
wrong instrument: it **logs** every copy and move to stdout, and its two
statics count objects rather than operations — `counter_` is incremented
identically by the copy constructor and the move constructor, so nothing in
it can tell one from the other. The lab needs a counting variant —
`copies_` and `moves_` behind an accessor — living in `exercises/choosing/`
and quoted into the appendix, **not** an edit to Chapter 14 or
`solutions/tracer.cpp`.

Then the numbers, with the caveat that is the lesson. A sink taking by value
and moving is **one move and zero copies** *when the argument is a
temporary*; hand it an lvalue and it is one copy **and** one move — worse
than the `const&`-plus-assign it is measured against, at one copy — and hand
it a named rvalue (`std::move(x)`, which is how Chapter 6 spells its own
sink call) and it is **two** moves. The lab asserts all three rows, because
"by value and move" is a win on temporaries and a tie or a loss elsewhere,
and a single asserted row would teach the reflex the page exists to replace.
An `auto` loop copies N times where `const auto&` copies none. And
`vector<T>` element addresses **change** across a forced reallocation while
`vector<unique_ptr<T>>` pointee addresses **do not** — that last one is the
whole justification for procedure four, made checkable.

The other honesty constraint is at the return end: returning a *temporary*
can be asserted at zero copies and zero moves (mandatory elision, C++17),
but returning a **named** local may only be asserted at zero *copies* —
NRVO is permitted, not guaranteed, which is why Chapter 6 already splits the
two and why the MSVC job tests `/Zc:nrvo`. An appendix claiming NRVO would
contradict both. And the `span` and `expected` branches are the two the lab
cannot judge at all — C++20 and C++23 against a C++17 build — so they carry
their standard on the page, the way Chapters 10 and 8 already do.

**Two notes for whoever writes it.** The `&&` parameter is a route the
procedure sends the reader *away* from: in application code the sink idiom
is by-value-and-move, and `&&` overloads belong to library authors and the
Rule of Five (Chapter 6) — "do not reach for this, here is why" is the
honest branch, and leaving it out would let the reader think the omission
was an oversight. And the mermaid scar recorded in CLAUDE.md is directly on
this item's path: a decision tree is the exact diagram that was once stacked
into a 1000-pixel column by `flowchart TD` when it wanted `LR`. Four
diagrams on one page is a lot of width to spend, so split each trunk at a
seam the procedure already teaches and keep the leaves in the tables.

**Its relationship to item 8.** They are neighbours: `const&` is a branch of
the parameter procedure, and const-correctness is the subject that branch
belongs to. If this item lands first, item 8 can point at the procedure
rather than re-deriving it — which is an argument for taking this one first,
and a second re-sequencing of item 8, whose note already defers to item 9.
Whoever picks up either should read both entries before starting.

**Delivered (2026-09-01) as Appendix H — *Choosing: Signatures, Containers,
and Storage*** — four procedures in the sketched shape (LR trunks, leaves in
tables, the C# reflex named per branch), with `exercises/choosing/` turning
the advice into asserted numbers and `check_verbatim.sh` holding the page to
the code in both directions. **One correction the measuring forced, and it
is the entry's own claim that was wrong.** The sketch said a sink costs "one
move"; the harness says that is true only for a temporary. Passing
`std::move(x)` costs **two** — one move constructs the parameter, one moves
it into the member — and for an lvalue the caller keeps using, the sink
costs a copy *and* a move where plain `const&` costs the copy alone. So the
appendix states the cost as a three-row table per caller kind rather than a
slogan, and the honest rule is that the sink wins on temporaries and buys
its generality with one extra move on lvalues. **A second correction, from
a review of the first draft, went deeper than the table.** Counting only
copies and moves hides the cost that actually decides a setter: a by-value
sink allocates a fresh buffer on every call, while `const&` copy-assignment
reuses the member's — measured at 100 allocations against 0 over 100 calls.
So the harness gained a replaced `operator new` (item 14's instrument), and
the page states the narrower, honest rule: the sink wins on temporaries and
on rare calls, and `const&` wins on a hot path. The NRVO constraint recorded
here survived intact — zero copies and zero moves for a returned temporary,
zero copies with `moves <= 1` for a named local — but with NRVO on, both
shapes measure zero and the distinction is never exercised, so
`build_all.sh` builds `passing.cpp` a second time under
`-fno-elide-constructors`. The same review found the first draft's
parameter procedure asking the sink question before polymorphism, which
routes a stored base-class argument into a by-value parameter and slices
it — exactly what this entry warned about above; the delivered trunk asks
polymorphism **first**. Four key principles landed in Appendix B under
*Choosing signatures and storage*, including the organising one this entry
specified; Chapters 2, 6, 10, 11 and Appendix A.5 gained pointers, keeping
their fragments as this entry required, and Chapters 21 and 33 had their
phantom "Chapter 11 invalidation table" citations retargeted at the column
this appendix now supplies. `check_verbatim.sh` holds the page to the code
in both directions: forward, every cpp fence on the page is in
`exercises/choosing/`; backward, every unit the lab's banners name is on
the page whole — the reverse that bridgelab gets from its TASK card, which
a directory with no card had to get another way.

### 18. The framework shape — Bestiary Shape 5, taught

**Missing:** the fifth SDK shape is named and never taught.

**Evidence:** of Chapter 16's five shapes, two carry a training chapter and
an exercise — Shape 1 is marked *Trained in Chapter 17* on the page, Shape 2
*Trained in Chapter 18* — and Shape 3 later earned a whole ticket chapter and
lab of its own (35, `exercises/comlab/`). Shape 4 has no lab either, but it
is not stranded: the Bestiary calls it "Shape 1 wearing work boots", and
Chapter 8 sets its status enums beside Shape 1's error codes in the same
sentence, so its discipline is taught even where its lab is not. Shape 5 is
the one shape with no treatment anywhere past its own four sentences — a
C++-native framework that brings its own object model, such as Qt's
parent-child ownership or Unreal's GC for UObjects, closing on the advice
that it is a rite of passage best skipped. There is no lab to skip it *via*,
and no later chapter that names its problem. The cost is not the missing
pages, it is a direct contradiction the reader has to resolve alone:
a parent-owned raw pointer or a GC-tracked handle overrides the Rule of Five
that Chapters 6 and 15 spend two chapters drilling in, and no page says so —
so the reader applies a `unique_ptr` reflex to an object the framework
already owns and double-frees it. In the 2026-09 reader study two readers,
arriving from Unreal and from Qt respectively, independently named this
paragraph as the moment they stopped, both having come to the book *because*
of that stack. (Two is the convergence bar this file sets, not a record: the
study's largest single bounce was five of eighteen on the README's framing,
which is a correction rather than an item, and is issue #55.) One grepped
and confirmed the book carries no mapping at all for the vocabulary they use
daily.

**A contribution looks like:** Chapter 35's move, applied to Shape 5 — a
generic parented-ownership framework in the `Fake*` house style (invariant 4
bars product-specific SDK material; naming Qt and Unreal as *study material*
is fine, and Chapter 16 already does), where the reader's smart-pointer
reflex compiles, runs, and double-frees at teardown. The judge is the same
shape as comlab's: the framework's own live-object counter must reach zero,
and the sanitizers catch the release too many. The habit to leave behind is a
question, not a rule — *before you wrap it, ask what already owns it.*

**Or close it as out of scope**, which is a legitimate outcome for this one:
per-framework object models are arguably each vendor's own documentation.
If that is the decision, it still costs one sentence in Chapter 16 saying so
plainly, because "named and then dropped" reads worse to that reader than
silence would.

**Sequencing:** after item 9, alongside items 8 and 19. Nothing here blocks
on those; the ordering is reader demand, and P/Invoke's is larger.

### 19. Below the mutex — the deadline path's other half

**Missing:** what to do instead, once the book has said what not to do.

**Evidence:** Chapter 29 teaches threads, mutexes, atomics as counters, and
the callback-lifetime pattern; Chapter 36 teaches that on a deadline thread
you must not allocate and must not lock. Between them the reader is told the
prohibition and never the alternative. A grep across all 38 chapters and 8
appendices finds no `memory_order` beyond the default, no false sharing, no
`alignas`, and no bounded single-producer/single-consumer structure; Chapter
29's `std::atomic<int> counter{0}` with `++counter` never mentions that this
is `seq_cst` by default. Two readers in the 2026-09 study — one on a
real-time audio path, one moving a hot path off the CLR — hit this
independently and disengaged within a few lines of each other, where the
chapter turns from a threading model into device-callback lifetime.

The sharper half of the same evidence is a contradiction the book already
carries: Chapter 29's trampoline takes a `lock_guard` and calls `push_back`
on a foreign SDK thread, which is correct for a device callback and
disqualified the moment that thread has a deadline. Neither chapter mentions
the other. That cross-reference is a correction and should not wait for this
item.

**A contribution looks like:** the hand-off, not a survey — a bounded SPSC
queue between a worker or UI thread and a deadline thread, and what
`memory_order` actually buys over the default, measured the way Appendix H
and Chapter 36 measure rather than asserted. The allocation-counting harness
in `exercises/perflab/` is the judge that already exists.

**Scope gate.** General lock-free data-structure design is *not* ours, and is
filed below under [Deliberately out of scope](#deliberately-out-of-scope);
the deadline path inside a plug-in the reader ships is ours, because this
book already teaches that path and currently stops one step short of usable.

**Sequencing:** after item 9, alongside items 8 and 18. The sharper half does
not wait for any of them — the Chapter 29 ↔ Chapter 36 cross-reference is a
correction, and is issue #54.

---

## Tier 3 — distinctive to this handbook

Material no general C++ book would carry, which is precisely why it belongs
here.

### 9. Going back the other way — C++/C# interop

**Missing:** P/Invoke, marshalling, and the round trip home.

**Sequenced first (2026-08).** The tier said "distinctive"; the 2026 deep
review's competitive pass said "underpriced" and re-sequenced it: this is
now the next *major* chapter investment, ahead of item 8. The reasoning is
the reader, not the topic — exposing the native code back to C# (a test
harness, an internal tool, a UI) is a near-certain need within months for
exactly this book's reader; the callback-lifetime lessons of Chapters 22
and 29 have a direct and harder-biting analogue when the callee is a
garbage-collected delegate; and no competitor covers the round trip from
the native side. Item numbers never shift, so it stays filed here — the
number is its address, not its priority.

**Confirmed by the reader study (2026-09-02), and now the clearest content
mandate on this list.** Four of eighteen simulated readers rated the absence
a *blocker* rather than a gap, independently and for four different reasons:
the returning C++ developer modernising a native layer he will have to
expose again; the .NET engineer who owns an existing P/Invoke layer and two
live bugs in it; the corporate trainer whose client will ask for that module
first; and the Java/JNI engineer — whose vote is the one to discount when
pricing *this* chapter, because the chapter as scoped does not serve her, for
the reason the scope note below sets out. Three votes for the chapter, then,
and a fourth for the shape beside it. `DllImport`,
`MarshalAs`, `CharSet`, `SafeHandle` and `LibraryImport` return zero hits
across the whole book. The interop reader's own summary is the entry this
item should have opened with: *for a book that opens with "the scary word in
the P/Invoke docs", the round trip home isn't written yet.*

**One scope addition from the same study.** The Java/JNI reader reached this
material by a route nobody planned for: a managed runtime that already owns
the process and has loaded her native code — the mirror image of item 16's
topology, and a shape neither that item nor Chapter 16's five covers. The
bindings author (pybind11, N-API) arrived at the same missing shape from a
different language. That is not this chapter's subject and should not
enlarge it, but a short "the other direction" section here, or a third family
in Appendix G (which today opens on the host's own channel, then runs Family
A and Family B), would serve two reader segments for a page — and both of
them found the book before they found its gap.

**Evidence:** one passing mention of P/Invoke, nothing on marshalling. A
developer with 17 years of C# behind them, now working against a native SDK,
very plausibly ends up exposing that native code to C# — a test harness, an
internal tool, a UI. This handbook is the one place that reader would look.

**A contribution looks like:** a chapter on the `extern "C"` surface (which
connects to item 6), what marshals cleanly and what does not, string
conversion at the boundary, who owns memory that crosses it, and callback
lifetime when a delegate is handed to native code. The Chapter 22 lambda
lifetime lesson has a direct analogue here, and it bites harder.

**Not to be confused with item 16**, which looks like the same subject and is
not. This item is the in-process round trip for code you own; item 16 is the
out-of-process bridge to a host you do *not* own, and needs no P/Invoke at
all. They are independent, and this one goes first — item 16 carries the full
split.

### 10. A glossary — DONE

**Missing:** Appendix E.

**Evidence:** zero hits for "glossary". The book uses TU, ODR, RVO, ABI and
vtable in the ordinary course of explaining things, and the reader's future
colleagues will use ADL, POD, CRTP and SFINAE without introduction.

**A contribution looks like:** one page, alphabetical, one or two sentences
per term, each pointing at the chapter where the idea actually lives. Terms
the book already uses come first; terms the reader will merely *hear* come
second, marked as such.

**Delivered as Appendix E — *Glossary* — in exactly the sketched shape:**
two alphabetical groups (some thirty-five terms the book uses, then the
recognition-only five: ADL, CRTP, linkage, POD, SFINAE), one or two
sentences each, every entry ending in the owning chapter's link so the
definition is the on-ramp rather than the destination. Two small calls the
sketch left open, decided in the writing: the symptom-to-page half of
"where do I look this up" was **not** duplicated here — Chapter 31's
symptom index already owns it, and the appendix's intro hands off to it —
and `volatile` landed in the *uses* group rather than the *hear* group,
because Chapter 29 turned out to already teach it (it is the C# reflex
trap of the second group's kind, but with a chapter to point at). The
appendix letters now run A–F with no gap, which retires the "letters skip
one until item 10 lands" caveat recorded under item 12.

### 12. The Rosetta Cookbook — DONE

**Delivered:** Appendix F — *The Rosetta Cookbook*, seeded with Recipes 1–8:
read a file, split, join, build a string, format, time a call, wrap a C
handle in a deleter, and `find` versus the inserting `[]`. The shape this
entry sketched survived contact intact: an index table from the C# name to
the recipe, one strict shape per recipe (**In C# / The recipe / Why it looks
like this / Trap**, the trap a one-line `[!WARNING]`), whys that
cross-reference the owning chapter rather than re-teach, and Finding-style
numbering — recipes append and are never renumbered, so a citation of
"Recipe 7" stays right. A second batch followed at once — Recipes 9–13:
write a file, `Path.Combine`, the `File.Exists`/`Directory.Exists` pair,
`Directory.GetFiles`, and `Task.Run`/`await` as `std::async` — closing the
`std::filesystem` gap (one incidental Chapter 8 appearance in the whole
book) and discharging Chapter 29's model into a recipe, blocking-destructor
trap included. A third batch — Recipes 14–16: expose an event (the C# leak
inverted: nothing keeps a dead subscriber alive, so the dangling side is
yours to manage), diagnostics that survive a crash (Chapter 28's buffering
lesson, made a habit), and the timer the standard library does not have —
plus a fourth battery paragraph in Chapter 27: SQLite as Bestiary Shape 1
in production, the Chapter 17 discipline under a different header. The
listings compile: `exercises/cookbook/` holds one translation unit per
domain (files, strings, timing, handles, lookups, paths, async, events,
logging), each with a `main()` asserting what its recipes claim, built and
run by `build_all.sh` under the canonical flags. It landed as Appendix F with E
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
in CONTRIBUTING.md. Candidate next recipes, in likely order of need:
`DateTime.Now` (timestamps, as distinct from Recipe 6's intervals), trim and
case conversion, environment variables, random numbers, and
`FileSystemWatcher` — the last as an honest-answer recipe on Recipe 5's
no-interpolation precedent: no portable equivalent exists, the stdlib
fallback is polling `fs::last_write_time`, and the real answers are
platform APIs. Each admitted only under question 11's double filter.

### 13. SOLID without the runtime

**Missing:** the translation of the reader's design vocabulary. SOLID was
born in C++, and C# inherited the principles fused to machinery — reflection,
DI containers, interfaces that cost nothing, mocks fabricated at runtime. A
seventeen-year C# developer does not hold the principles; they hold the
principles welded to `IServiceCollection` and Moq. Nothing in the book
un-fuses them, and month one is design reviews conducted in exactly this
vocabulary — the reader who proposes extracting an interface where the C++
answer is a template seam, or reaches for a container that is not there, is
paying the cost this entry exists to name.

**Evidence:** zero hits for SOLID, Liskov, "single responsibility",
"open/closed", "dependency injection" and "dependency inversion" — while the
fragments are everywhere, taught without the names. Appendix B already
carries the seam principle ("there is no reflection, so there are no runtime
mocks — I design the seam first, as an interface or a template parameter");
the Rule of Zero (Chapter 6) is single responsibility applied to resources —
one resource or none; slicing (Chapter 20) and the non-virtual destructor
(Chapter 5) are Liskov violations C# cannot even express, mechanical rather
than aspirational; Chapter 30's you-cannot-insert-a-virtual-method is
open/closed enforced by the linker and the shipped binary; and the
Bestiary's function-pointer-plus-context is dependency inversion in C
clothing, practised since Chapter 18. The one page that assembles them does
not exist.

**A contribution looks like:** a chapter in item 8's gather-and-translate
shape — cross-reference, don't duplicate; the fragments stay where they are.
Principle by principle: what the reader believed, which machinery it was
welded to, what replaces the machinery here, and the mechanical failure mode
that raises the stakes. Three through-lines carry it. The cost model
inverts: abstraction has a visible price, and C++ offers three tools where
C# had one — the pure-virtual interface, the template parameter,
`std::function` — so choosing is the new skill. Every dependency edge gains
an ownership annotation: DI without a GC means owner-or-borrower is the
first line of the signature, teardown order is a design artifact, and the
captive-dependency bug becomes a dangling reference a sanitizer can name.
And DI without a container is constructor parameters wired in `main()` —
which was the principle all along, with the plug-in twist that the host *is*
the container, delivering dependencies through init structs and context
pointers. C++ container libraries exist and are culturally marginal; one
honest sentence covers them. What stays prose-only, per
that same filter, is now delivered as prose: Chapter 27's *The batteries C#
included* names the libraries the ecosystem converged on (nlohmann/json,
pugixml, libcurl/cpr) and states the shock outright — the standard library
has no HTTP client and no sockets at all. JSON/XML *parsing* remains
third-party territory; the practical JSON coverage is an item 11 candidate
ticket — a hand-rolled mini-parser, stdlib-only.

### 16. The bridge out — serving a foreign client from inside the host — DONE (Chapter 38 + Appendix G)

**Missing:** what happens when the interesting half of the plug-in — the UI,
the business logic, the automation surface — is not going to be written in
C++, and the host's contract is *"C++, in my process, on my thread"*.

**Evidence:** the book promises this twice and stops both times. Chapter 29's
*In the wild* names the mechanism in a subordinate clause — "the SDK's own
dispatch-to-main mechanism if it has one, or a queue your main-thread code
drains. This is C#'s synchronization context, except nothing does it for
you" — and never builds one. The bullet under it names reentrancy and the
deadlock it causes, and never shows either. Meanwhile step 3 of that same
chapter's *Try it* — the lab whose task card is `exercises/threadlab/` —
already has the reader build a mutex-guarded job queue drained by the
polling thread, so the machinery is in their hands with no page saying what
it generalizes to. Chapter 16's Shape 1 *is* this host — a
desktop-application plug-in SDK, error codes, owned payloads — and notice
what the Bestiary does *not* say there: Shape 1 describes an API surface and
never names a calling thread, so the affinity that decides this whole
subject reaches the reader only as Chapter 29's bullet above. Chapter 30
then authors a binary boundary inside the process without ever asking what
changes when the boundary moves out of it.

The cost of hitting this unprepared is a specific wrong turn, and it is the
one a C# developer takes first: load a runtime into the host and write the UI
there. That answer fails for reasons the reader cannot see from the managed
side — one runtime per process, so whichever plug-in loads first wins; a
shared crash domain, so an unhandled managed exception takes the user's
unsaved document with it; and windows the host does not know about, which do
not dock, do not save with the layout, and do not go modal when the host
does. By the time those show up the UI is written.

**A contribution looks like:** a chapter appended at the end of Part VI, plus
a lab — and, separately, an appendix. The split is the entry's one real
design decision, and it follows question 2. The chapter is the part that can
be taught and verified: thread affinity as the single invariant every bridge
obeys, the main-thread queue and its waker, reentrancy and self-deadlock,
refusing work rather than queueing it silently, a domain model owned by the
bridge rather than the vendor's structs on the wire, and an `IHostAdapter`
seam with a stand-in behind it — Chapter 28's principle, and the fourth
stand-in in the repo after FakeSDK, FakeDevice and comlab's FakeSDK2
(Chapter 28 names only the first two because it predates comlab). Call it
`StubHostAdapter`, not `FakeHostAdapter`, and deliberately: hard invariant 2
globs `exercises/*/Fake*.h|.cpp` as vendor code quoted verbatim in the book
and almost never right to change, while this one is the opposite — a double
the reader owns and is meant to extend. The appendix (the next free letter,
**G** today — provisional for the same reason no chapter number is
pre-assigned) is the part that is looked up rather than read and that ages
on someone else's schedule: the survey of mechanisms — runtime in-process
versus server-in-the-host versus reusing the host's own automation channel —
and a decision table. Lead the appendix with the host's existing channel,
because "the host may already have solved this" is the shortest path that
works and belongs before the custom ones.

**The lab is what makes this landable stdlib-only**, and the insight is that
the transport is the part that does not need teaching: threads are a
transport. `exercises/bridgelab/` needs no socket and no third-party
dependency — a fake event loop on the main thread, client threads posting
commands, a `StubHostAdapter` that asserts the calling thread id and can go
modal — built twice by `build_all.sh` under the canonical flags and then
under `-fsanitize=thread`, reusing the probe and `--require-tsan` bargain the
threadlab step already established.

Three broken-vs-fixed pairs are available without inventing any, and each
fails in a different way: a `drain()` that skips a job while the host is
modal and never completes its future (the client spins forever, and every
unit test is green because nothing is modal in a test); an `invoke()` called
from the main thread, which blocks on a future only the main thread can
complete — a hang that appears the day someone moves a client in-process;
and a handler registry read from a transport thread while another registers,
which ThreadSanitizer names outright.

**Only the third of those has a sanitizer, so the lab brings its own judge**
— the move Chapters 34, 35 and 36 each had to make when the canonical flags
went quiet, and the reason those chapters have a hand-decoded capture, a
live-object counter and an allocation counter respectively. Here the judge
is a **bounded wait**: every `invoke()` in the committed harness takes a
deadline, and `main()` asserts that `wait_for` returned
`std::future_status::ready` and not `timeout`. That converts both hangs into
a failed assertion with a line number, and it is stdlib-only. It is also not
optional. `build_all.sh` has no timeout of its own, so an unbounded wait in
a committed program does not fail the repo invariant — it stops it, and CI
with it, until a job-level timeout fires with nothing naming the cause. The
broken halves stay book-only, as in every other ticket lab and as in
Chapter 29's step 4 above: the committed files are the fixed state, and the
broken listings live in `TASK.md` and the chapter, where they exist to fail.

Third-party stacks may be named in prose, as Chapters 16 and 27 name libusb
and libcurl, but no listing may use one and none may reach `solutions/` —
invariant 5 is not bent for this. Where the lab's own files live is a
separate question, and *Where chapter code lives* settles it: the committed
fixed state goes in `exercises/bridgelab/`, as in every other ticket lab,
and both `build_all.sh` steps build it there. Do not read the threadlab
precedent as pointing the other way — its worked solution sits at
`solutions/device_threaded_solution.cpp` and that is the file the TSan
section builds today, so reusing that section's bargain means teaching it a
second source, not adding a file to `solutions/`.

**Its relationship to item 9, since they look like one item and are not.**
Item 9 is the in-process round trip for code you own: P/Invoke, marshalling,
string ownership, the lifetime of a delegate handed to native code. Item 16
is the out-of-process bridge to a host you do *not* own, and its whole thesis
is that the shim has no runtime in it at all. They are independent — nothing
here needs P/Invoke — but item 9 goes first: it is the more universal need,
and the deep review already promoted it to the next major chapter.

**Two honest notes for whoever writes it.** The moment of need (question 1)
is month six and the design review, not week one — which is the right end of
Part VI for it and an argument for keeping the survey out of the reading
path, but it should be said on the page rather than left for the reader to
discover. And there is a ticket hiding in here for item 11, which stays open
to new tickets by PR: *the client shows a spinner forever* — attached
evidence, a queue that waits politely while a modal dialog is open, and a
fix that is a distinct error code rather than a longer timeout.

**Delivered so far (2026-08-31): the chapter half.** Chapter 38 — *The
Bridge Out* — plus `exercises/bridgelab/`, in the shape this entry
committed to: the three breaks quoted in TASK.md and the chapter (the
polite drain, the main-thread self-wait, the registry race), the committed
fixed state built by `build_all.sh` under the canonical flags AND as a
second source in the probe-gated TSan section, `StubHostAdapter` under the
name argued for above, and the bounded-wait judge — every harness invoke
carries a deadline, and the harness's checks convert both hang-shaped
breaks into a failed line. Two things the writing added to the sketch: the
*dropping* variant of the broken drain turned out not to hang at all — a
destroyed `packaged_task` stores `broken_promise`, so the client gets a
`std::future_error` on a transport thread with no handler, which is its own
lesson and is in the chapter — and the reentrancy guard gained an asserted
second half (the job deferred by a nested `Drain` must run one turn later:
kept, not dropped). Three key principles landed in Appendix B under
*Bridging a host*; the glossary gained *thread affinity*; Chapter 31's
symptom index gained the spinner row; Chapter 29's two IOU bullets now
point here.

**Delivered (2026-09-01): the appendix half, closing the item.** Appendix
G — *The Bridge Catalogue* — in the sketched order: the host's own channel
leads, because "the host may already have solved this" belongs before the
custom options; Family A follows with its shared costs priced once (and
Chapter 35 carrying the COM depth rather than this page repeating it);
Family B as catalogue entries — mechanism, price, when it wins — with one
topology diagram; then the decision table and the questions that collapse
it to a row. One shape decision worth recording: the appendix contains
**no C++ listings at all** — its only fences are the topology diagram and
one JSON discovery record; every compilable listing the subject needs
already lives in Chapter 38 and its lab, and a page with nothing to
compile is a page `build_all.sh` owes nothing to (`check_verbatim.sh`
pins the shape: a cpp fence landing in Appendix G fails the book job). The chapter now points at the appendix
from its families section and its transport-menu bullet, so the reading
path and the lookup path meet where they should.

### 20. The retrofit — modernising code you may not rewrite

**Missing:** no exercise in the book modernises working code whose callers
must keep compiling.

**Evidence:** the labs start from three places and none of them is this one.
Most begin at a blank file; `exercises/buildlab/` hands over a working
Greeter trio to break on purpose; the six ticket labs (32-37) hand over code
that is already broken and ask for the diagnosis. Not one hands over code
that *works*, has callers, and must still have them at the end — which is the
retrofit, and the word "legacy" appears four times in the whole book, every
time in passing (`01:154`, `09:11`, `19:73`, `G:84`). But the arrival path
this handbook is written for frequently *is* the retrofit: the C# veteran is
handed the native layer precisely because it is old, and the ticket says
modernise it without breaking the callers. Two readers in the 2026-09 study:
the returning-C++ persona — C++03 until 2007, seventeen years of C#, now
holding a "modernise the native layer" mandate — reported exactly this, and
the Qt lead's month-three worry is its team-scale version. The
book teaches the Rule of Five by having you write a class from nothing
(Chapter 15); it never has you introduce ownership into a class that already
works, has callers, and must keep them.

**A contribution looks like:** a ticket-shaped chapter in the 32-37 format
whose starting point is a working raw-pointer class the reader is *not
allowed* to rewrite — one seam at a time, each step green under the
sanitizers, callers untouched. The acceptance test is unusually clean for
this book and is the reason the chapter is worth writing: the caller's
translation unit is byte-identical before and after, so the harness compiles
the original caller against the modernised implementation. That is a claim a
build can check, which is the standing bar for anything here. It also
connects directly to item 6's material — the retrofit stops at the ABI
boundary, and Chapter 30 already says why.

**Sequencing:** after item 9. The same reader wants both, and P/Invoke has
three independent votes for the chapter as scoped to this one's two.

---

## Known gaps carried over

Already recorded in [CONTRIBUTING.md](CONTRIBUTING.md) and still open:

- **A COM-style refcounting lab — DONE.** Bestiary Shape 3 had no exercise
  (nor, still, do Shapes 4 and 5 — see item 18); it is now Chapter 35 and
  `exercises/comlab/`, delivered inside item 11's ticket framing exactly as
  the candidate line predicted — *"the vendor upgraded the SDK, and the new
  API is refcounted."* See item 11's delivered notes.
- **A threaded-callback lab** — **DONE.** It was a FakeDevice stretch goal, then
  Chapter 29's *Try it*; it is now `exercises/threadlab/` with
  `solutions/device_threaded_solution.cpp` and a ThreadSanitizer step in CI.
  See item 4 above.

## Deliberately out of scope

Items closed as out of scope live here with the reasoning, so the same
suggestion does not arrive twice. Closing a *numbered item* this way is a
decision that belongs in an issue first; an entry that was never an item —
like the one below — is recorded here directly, because the work of the
section is to answer a recurring suggestion, not to bury a promise.

### Classroom scaffolding — slides, rubrics, lecture timings, a separated answer key

**Asked for by two readers in the 2026-09 study, and both were right that it
is missing:** a corporate trainer building a five-day course for thirty .NET
engineers, and a university instructor evaluating the book for a second-year
systems module. Between them they wanted a synchronous day-by-day timetable,
per-chapter lecture minutes, comprehension checks for the nineteen chapters
that carry no exercise (`exercises/README.md` indexes the other nineteen), a marking rubric for the half `check.sh`
cannot score — the predictions and the sabotage write-ups, which is where the
learning actually happens — and a placement diagnostic. A filename search for
slides, quizzes, assessments or an instructor guide returns nothing, correctly.

**Out of scope because it is a different product, not a missing chapter.**
This handbook's pedagogy is deliberately solo, spaced and on-the-job: Chapter
24 schedules retrieval at Day 14 and again around Day 30, against a real
codebase, for one reader with a job. The instructor's own summary of why it
did not fit — a month of spaced solo retrieval versus thirty people who go
back to their desks on Friday — is exactly right, and it is a property of the
design rather than a gap in it. The same reasoning already sits in
[CONTRIBUTING.md](CONTRIBUTING.md#the-questions-every-piece-of-material-answers)'s
first question: material earns its place because the SDK-work transition
presents the problem in the first months. A lecture timetable does not fail
that test, it is not taking it.

**And the licence exists precisely so someone else can build it.** The prose
is CC-BY 4.0 and every code sample is MIT, so a trainer can lift a chapter
into a deck, or an instructor into a lab sheet, with one line of attribution
and no permission. That is not a consolation — it is the intended division of
labour, and the trainer who scored the book 6/10 as course material still
answered yes to building on it. What she took was already here: the exercise
index with its time estimates, Chapter 1's ownership flowchart, Chapter 28's
suite where every assertion passes and the code still double-frees, and the
ticket-lab format in which the harness *is* the rubric.

**One neighbouring request is refused for a different reason, and should not
be granted by accident.** The instructor also wanted the reference solutions
separated from the task chapters, because `book/15-exercise-the-buffer.md`
carries its full answer two scrolls below the question. That is not
scaffolding, it is the book's own structure, and the `<details>` spoiler fold
is a deliberate convention: this book's reader is working alone, with no
demonstrator to ask, and needs the answer *available and deferred*. A marker
needs it absent. Those are different readers, and this book serves the first
one. If a course wants a clean question sheet, `exercises/*/TASK.md` is
already that file.

### General lock-free data-structure design

**Named as out of scope by item 19, and recorded here so the boundary is
findable from both ends.** Writing a correct lock-free queue, stack or map
from first principles — the ABA problem, hazard pointers, epoch reclamation,
the memory-order proofs that make any of it true — is a research literature
with its own books, and nothing in this handbook's job description asks the
reader to produce one.

**Out of scope because the reader's job is to *use* one.** Item 19's gap is
the deadline path inside a plug-in that ships: a bounded SPSC queue between a
worker thread and a real-time callback, and what `memory_order` buys over the
default. That is a hand-off with two known ends and a measurable claim, which
is why it is an item. The general problem has neither, and a chapter
attempting it would fail this file's own first test — the SDK-work transition
does not present it in the first months. The C# comparison holds the line
neatly: this book's reader used `ConcurrentQueue<T>`, they did not write it.

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
