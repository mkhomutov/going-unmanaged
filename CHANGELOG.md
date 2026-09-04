# Changelog

Notable changes to the handbook, newest first.

**How versions work here:** the book's chapter and Finding numbers are the
public contract — people cite them, so they version like an API.
**MAJOR** = renumbering or restructuring (existing numbers change meaning);
**MINOR** = appended content (new chapters, Findings, exercises);
**PATCH** = corrections that move no numbers. Details in
[CONTRIBUTING.md](CONTRIBUTING.md). Numbering freezes at v1.0 — until then,
numbers may still move.

## [Unreleased]

Sixteen topics a reader asked the book about, read against every page and
found covered, scattered, or absent. This is the series of additions that
closes the gaps; each appends, and no number moves.

### Added

- **Chapter 10** gains `std::variant` — the tagged union with the compiler on
  your side, against both the C `kind`-plus-`union` of vendor event structs
  and the class hierarchy a C# developer would write — and a table of what
  `std::optional` is *not* (no `optional<T&>`, no `?.`, and `*` on an empty
  one is quiet undefined behavior), plus a paragraph on `decltype` beside
  `auto`.
- **Chapter 6** gains *Value categories in one table*: lvalue, prvalue and
  xvalue, the binding table for `T&`/`const T&`/`T&&`, and four traps that
  compile clean — the named rvalue reference that is an lvalue, `std::move`
  on a const object, `return std::move(local)`, and lifetime extension
  through a member but not through a call. Two of the four are priced by
  `exercises/choosing/passing.cpp`, quoted whole and pinned both ways by
  `check_verbatim.sh`.
- **Appendix H**, procedure 4, gains the branch that is not a box: a closed
  set of unrelated types is a `vector<variant<...>>`, asserted by
  `exercises/choosing/storing.cpp`.
- **Appendix F**: Recipe 18 (find an element, an index, or a substring —
  the three spellings of "not found"), Recipe 19 (a value that may be
  absent), Recipe 20 (switch on the kind of a message, with the
  `overloaded` idiom). `exercises/cookbook/alternatives.cpp` is new.
- **Chapter 8** gains *Living in both dialects: the translation layer* —
  a throw becomes a value at a module boundary and a value becomes a throw
  again only at the top; a C++17 `Result<T, E>` over `std::variant`; what
  C++23's `and_then`/`transform` collapse; and three pieces of exception
  vocabulary the chapter had used without introducing (a custom exception
  type, catch order, `exception_ptr`). Its `Result` and `load_config` are
  Recipe 22's, held whole on both pages; its chaining listing is the
  cookbook's one C++23 file, `exercises/cookbook/expected.cpp`, built as
  its own probe with `--require-expected` in CI.
- **Appendix F**: Recipe 21 (throw and catch your own exception type),
  Recipe 22 (return a value or an error — a C++17 `Result<T, E>` over
  `std::variant`, with `optional` and `std::expected` as its neighbours),
  Recipe 23 (test for an empty string, and for no string at all —
  a `std::string` cannot be null, and the null `const char*` from a C API
  is the one that can). `check_platform_claims.sh` now asserts what that
  null does under each standard library.
- **Chapter 9** points at Recipe 23; **Appendix B** one new principle.
- **Chapter 12** gains *What goes in the header, and what goes in the
  .cpp*: the decision table by kind of entity, `.h` versus `.hpp`, and the
  own-header-first include order with its reason (and its one exception,
  a precompiled header). **Chapter 23**'s breakage 6 gains its second
  half — a header that borrows an include from its own .cpp and fails
  in the consumer with `use of undeclared identifier 'std'`.
- **Appendix A.8**, *Naming: there is no house style, so learn to read
  three*: the standard-library, Google/LLVM and framework dialects in one
  table, the members row's three spellings, the reserved-identifier rule
  that no default flag enforces. **Appendix D**'s first-week question
  points at it; **CONTRIBUTING** records the book's own convention;
  **Appendix B** two new principles.
- **Chapter 26** gains *Compile-time switches, and the one that must be
  global* — the four tools in preference order, and the macro that changes
  a struct's layout in one translation unit: a silent link, an answer that
  depends on link order, and, unlike Chapter 27's diamond, no order the
  sanitizers catch. `check_platform_claims.sh` holds all three claims on
  both CI platforms, and `exercises/buildlab/` gains a `GREETER_AUDIT`
  option whose PUBLIC reach `build_all.sh` reads back from the compile
  database. Also *A layout that survives*: the directory tree to copy on
  day one, with `exercises/deplab/mathlib/` as the built example.
- **Appendix F**: Recipe 24 (compile a diagnostic out of Release — `assert`
  as `[Conditional("DEBUG")]`, and the side effect that vanishes with it;
  `logging.cpp` is built twice, once under `-DNDEBUG`). **Appendix B** one
  new principle.
- **Chapter 40 — CMake for the Plug-in** (ROADMAP item 22), and
  `exercises/pluginlab/`: the plug-in as a MODULE library, the SDK that
  ships no config package and the hand-written find-module that consumes
  it, symbol visibility and the export table as the judge — with the
  chapter's own finding, that hidden visibility covers what you compile and
  not the static library you link — `CMAKE_MSVC_RUNTIME_LIBRARY`,
  generator expressions explained once, presets, and deplab's
  install/export file quoted whole at last. `build_all.sh` installs the
  drop, builds the module and the host, loads one with the other and reads
  the export table back; the `buildlab-msvc` job does the same under
  Visual Studio. **Appendix B** three new principles.
- **Chapter 1** names custom deleters and `enable_shared_from_this`;
  **Chapter 11** the algorithm-versus-member `find` trap; **Chapter 14** a
  fourth Tracer experiment (a const source moves as a copy); **Appendix I**
  `cbegin` and `std::as_const`; **Appendix E** entries for decltype,
  ownership, variant and xvalue; **Appendix B** two new principles.

## [0.8.0] — 2026-09-03

The release the readers asked for. Every previous version was steered by the
roadmap; this one began with eighteen constructed reader personas taking the
repository cold, each sent wherever their own problem led rather than down the
Contents page, and required to cite a file for every claim. Their verdict was
not what the roadmap predicted: trust scored 7.6 out of 10 and relevance 6.4,
which is one finding rather than two — nobody doubted the book, they doubted
whether it was *for* them, and several readers it plainly is for could not
find the chapter that would have saved them. So the corrections here are
mostly roads between chapters that were each already correct.

The same study settled what to write next, and both of its answers landed.
**Chapter 39** is the round trip home: P/Invoke from the side this book is
about, where a signature is declared twice in two languages and compared by
nothing. **Appendix I** gathers const from the five places that taught pieces
of it, on one sentence — const describes a path, not an object. Both were the
oldest open content items on the list, and both were confirmed by readers
before they were written.

Three labs, and two of them judge in ways nothing here had judged before.
`exercises/interoplab/` verifies a managed boundary with no .NET anywhere,
because `marshal.h` stands in for the marshaller the way FakeSDK stands in
for a vendor. `exercises/constlab/` asserts that **five compilations fail**,
which the subject forced: a const violation never reaches a binary, so a
harness that only ever compiles things cannot check the one thing that page
is about. `exercises/deplab/` closes the last Part VI code gap, and holds a
version pin to two tags because building once proves the mechanism runs and
only building twice proves the pin chose the version.

MINOR: one chapter, one appendix and three labs appended, plus corrections
throughout; no existing chapter, Finding, Recipe or appendix letter changed
meaning. Appendix letters now run A–I with no gap.


- **New: Chapter 39 — The Round Trip Home** (MINOR — an appended chapter;
  closes ROADMAP item 9 and issue #23). The publishing half of P/Invoke,
  written from the side this book is about: a signature declared twice in
  two languages and compared by nothing, which is Chapter 27's ODR diamond
  with the linker removed. Blittable as the palette; the leading size field
  doing a second job as the only runtime check that two hand-written
  declarations agree; three string lengths and the header sentence that
  picks one; caller-allocates as the shape that deletes the which-heap
  question rather than answering it; `SafeHandle` as the managed
  counterpart to Chapter 1's guard; and the delegate a collector may take
  while native code still holds its thunk — Chapter 22's lesson with no
  evidence on the managed side. `exercises/interoplab/` judges all of it
  under the canonical flags with no .NET involved: `marshal.h` stands in
  for the marshaller the way FakeSDK stands in for a vendor, because every
  mistake in the chapter is observable from the native side. Three key
  principles join Appendix B, the glossary gains *P/Invoke*, *blittable*
  and *marshalling*, Chapter 31's symptom index gains the mojibake and
  managed-caller-crash rows, and Chapter 30 now points forward to this
  chapter when the consumer of its boundary is C#.
- **New: Appendix I — Const-Correctness** (MINOR — an appended appendix;
  closes ROADMAP item 8 and issue #21). const gathered into one subject
  from the five places that taught pieces of it. The organising idea is
  that **const describes a path, not an object** — the same object is
  writable to its owner and read-only to everyone it was lent to, which C#
  cannot express and which is why `IReadOnlyList<T>` is a separate
  interface. From there: the interface splitting in two for free, bitwise
  versus logical const and `mutable`'s narrow job, the overload pair and
  why the const one returns a const reference, transitivity as the entire
  cost of retrofitting, the three places the reflex misfires, `const_cast`'s
  one honest direction, and const-is-not-constexpr. Appendix A.5 keeps the
  syntax and points forward; the parameter decision stays Appendix H's.
  `exercises/constlab/` judges it by **five compilations that must fail** —
  the first check in this repository to assert a build is refused, because
  a const violation never reaches a binary. Appendix letters now run A–I
  with no gap; three key principles join Appendix B, and the glossary gains
  *const-correctness* and *mutable*.

- **New: `exercises/deplab`** (MINOR — an appended exercise; closes issue
  #10). Chapter 27's *Try it* was the last Part VI exercise outside the
  `exercises/` convention, and steps 1–4 now have a finished form: one
  `mathlib` dependency and one `app/main.cpp` consumed three ways —
  vendored via `add_subdirectory`, fetched via `FetchContent` and a
  `file://` URL, and found via `find_package(mathlib CONFIG)` against an
  installed prefix. The app source never says where the library came from
  and the three `consume-*/CMakeLists.txt` are the whole difference, so
  the lab's subject is build description and nothing else. The third path
  is not in the chapter's Try it: it is the *producing* half of the
  `find_package(VendorSDK REQUIRED)  # a config package, if it ships one`
  line the chapter shows and never asks anyone to make, and nothing in the
  repo exercised it. The chapter's Try it gains that step, so it now reads
  "three ways, then break it on purpose"; its ODR-diamond and
  which-order-does-ASan-catch steps renumber 4→5 and 5→6 and stay a hand
  exercise, because the prediction is the entire exercise and a committed
  answer would spoil it in the time it takes to read a filename.
  build_all.sh grows a `--require-git`, since the fetched path needs a git
  that can clone a `file://` repository and can be refused one while still
  being installed.
- **Corrections from the eighteen-reader study** (PATCH — none moves a
  number). The one technical defect the study found: Chapter 2 declared a
  `Shape` with no virtual destructor and then used it as
  `vector<unique_ptr<Shape>>` in a listing commented *"correct polymorphic
  container"* — the undefined behavior Chapter 5 exists to name, in prose
  no CI check could see, and correct in the five other places the repo
  declares that hierarchy. Beyond it the findings were structural.
  **Routing:** Chapters 16 and 18 contained no reference to Chapter 29, so a
  reader who stopped at 18 shipped a synchronous wrapper that is a
  use-after-free the moment a driver thread calls back; Chapters 29 and 36
  taught contradicting defaults for the same callback and did not mention
  each other; Chapter 31's symptom index assumed you already had a stack.
  All now carry terminal links and a triage step — *terminal*, because a
  second reader reported the existing inline cross-references as noise once
  he stopped reading linearly. **The landing page:** the AI-origin paragraph
  arrived before any of its evidence, which was the study's single largest
  bounce point at five readers of eighteen, so the verification links now
  sit inside that paragraph along with an honest statement of what they do
  not cover; two time estimates for the same exercise disagreed by 9×; and
  the invitation to become a co-author now says first that the list is one
  name long. **Five clarifications:** small-string optimisation named in
  Appendix H and Chapter 9 (the measurement rig defeats it deliberately and
  said so only in a code comment), ARMv6-M as the one family where Chapter
  34's "nothing will warn you" is false, the GIL as the other shape of
  thread affinity in Chapter 38, forwarding references in Chapter 6, and
  Chapter 29's race transcript attributed to the toolchain that produced it.
- **Tooling: two new classes of check** (PATCH). `check_platform_claims.sh`
  now asserts Chapter 27's ODR link-order claim — that both orders link
  silently, that they *disagree*, and that exactly one is caught — with no
  hardcoded exit code, since the caught order aborts on macOS and exits 1 on
  Linux. It lives there rather than in a lab because the demonstration is an
  ill-formed program whose failure is the lesson, and `build_all.sh`'s
  contract is that programs succeed. `check_markup.sh` gained a typographic
  rule: no two horizontal rules with only blank lines between them, which
  GitHub draws as one heavier divider and which seventeen files had acquired
  invisibly. The check landed before the seventeen fixes on purpose.
- **ROADMAP: items 18–21 and the first out-of-scope entry.** The framework
  shape (Bestiary Shape 5, named in Chapter 16 and taught nowhere — the
  study's most-cited bounce point, and filed with an explicit
  out-of-scope option), below the mutex, the retrofit, and the
  interrupt-context callback. Classroom scaffolding — slides, rubrics,
  lecture timings — is recorded as deliberately out of scope with its
  reasoning, together with the neighbouring request it should not be
  confused with: separating reference solutions from task chapters, which
  is refused for a different reason, because a reader working alone needs
  the answer available and deferred where a marker needs it absent.

## [0.7.0] — 2026-09-01

The bridge out of the process, and the four decisions before it. Chapter 38
closes a promise Chapter 29 made and left standing for nine chapters — the
main-thread queue a foreign client needs to drive a host — and Appendix G
is its lookup half, the survey of every mechanism for connecting the two.
Appendix H is a different kind of debt: the book had taught the pieces of
every signature in six places and the *choice* in none, so the four
questions every function asks — which container, how to take a parameter,
what to return, what goes inside the collection — now have procedures, and
`exercises/choosing/` holds their costs to counted numbers. Two labs, two
new verbatim contracts that are exact opposites of each other, and a
release in which the measuring changed the advice more than once. MINOR:
one chapter, two appendices and two labs appended; no existing chapter,
Finding, Recipe or appendix letter changed meaning.

- **New: Chapter 38 — The Bridge Out** (MINOR — an appended chapter; the
  chapter half of ROADMAP item 16). Pays
  Chapter 29's nine-chapter IOU — "a queue your main-thread code drains" —
  in full: thread affinity as the one invariant, a queue that completes
  every job it accepts (refusal is a result, HOST_BUSY, never silence),
  the inline path for the caller that must never wait on itself, and a
  registry frozen before the first transport thread. `exercises/bridgelab/`
  holds the fixed state under both sanitizer builds; two of its three
  breaks are hangs, so the judge is a bounded wait — every harness invoke
  carries a deadline, and a timeout fails with a line number instead of
  stopping CI. Three key principles join Appendix B (*Bridging a host*),
  the glossary gains *thread affinity*, Chapter 31's symptom index gains
  the spinner row, and Chapter 29's two IOU bullets now point forward.
- **New: Appendix G — The Bridge Catalogue** (MINOR — an appended
  appendix; closes ROADMAP item 16 together with Chapter 38). The lookup
  half of the bridge material: the survey of mechanisms led by the
  host's own automation channel, Family A priced against its shared
  in-process costs (Chapter 35 carries the COM depth), Family B as
  mechanism/price/when entries with a topology diagram, the decision
  table, and the questions that collapse it to a row. Deliberately no
  C++ listings — only the diagram and a JSON discovery record; every
  compilable listing the subject needs lives in Chapter 38 and
  `exercises/bridgelab/`, and `check_verbatim.sh` enforces the
  no-cpp-fence shape. Appendix letters now run A–G with no gap.
- **New: Appendix H — Choosing: Signatures, Containers, and Storage**
  (MINOR — an appended appendix; closes ROADMAP item 17). The four
  decisions in every signature — which container, how to take a
  parameter, what to return, and whether collection elements go in by
  value or behind a pointer — as procedures rather than translation
  tables, on the observation that all four are one question at three
  scopes (who owns this, how long does it live, who may see it). It
  separates the three independent reasons the book had been giving for
  the same `vector<unique_ptr<T>>` recommendation — slicing (Ch 2, 20),
  address stability (Ch 33), and move cost — which no page had ever
  told apart. `exercises/choosing/` checks the costs it quotes, and the
  measuring corrected the advice twice. In copies and moves: a sink costs
  one move only from a temporary, two from `std::move(x)`, and a copy
  *plus* a move from an lvalue the caller keeps — so the page states a
  table per caller kind instead of a slogan. In allocations, which that
  tally cannot see: a by-value sink allocates on every call where `const&`
  reuses the member's buffer (100 against 0 over 100 calls), so the page
  sends hot paths to `const&` and the harness gained a replaced
  `operator new`. The parameter procedure asks about polymorphism
  **first**, because asking the sink question first routes a stored
  base-class argument into a by-value parameter and slices it. Judged by a
  counting `CHECK` rather than `assert` (which a Release build compiles
  away), and `passing.cpp` is built a second time under
  `-fno-elide-constructors`, since with NRVO on the page's
  guaranteed-vs-permitted return distinction measures zero either way.
  Four key principles join Appendix B; Chapters 2, 6, 10, 11 and Appendix
  A.5 gain pointers and keep their fragments, and Chapters 21 and 33 have
  their phantom "Chapter 11 invalidation table" citations retargeted at
  the stability column this appendix supplies.
- **Tooling: two opposite verbatim contracts, and a second sanitizer
  source.** `check_verbatim.sh` now pins Appendix G to hold **no** cpp
  fence at all (its shape is a recorded decision, so a listing landing
  there fails the book job) and Appendix H to hold **only** fences that
  `exercises/choosing/` compiles — the same both-directions rule Chapter
  38 takes against `exercises/bridgelab/`. `build_all.sh` gained the two
  labs, and the probe-gated ThreadSanitizer section gained a second
  source rather than a second section, because bridgelab's registry-race
  break has no judge without it.

## [0.6.0] — 2026-08-05

The deep review's last tier, delivered — and the judge learns to count
past one file. Two more tickets take the arc to six by adding the two
kinds of evidence the first four never met: the profile that appears to
*acquit* (36) and the crash report that is the only artifact there is
(37). The glossary closes the appendix gap, and `check.sh`/`check.ps1`
now take several translation units, so every ticket lab hands the reader
the same one-line judge as the rest of the book. MINOR: two chapters,
two labs and an appendix appended; no existing chapter, Finding, Recipe
or appendix letter changed meaning.

- **New: Chapter 36 — The Host Stutters** (MINOR — an appended chapter;
  closes ROADMAP item 14, the performance ticket). The attached profile
  is the ticket's trap: support's percentages — 6.7% of the thread, 0.2%
  of the wall clock — are *correct and irrelevant*, because a mean can
  only acquit a claim about throughput and this ticket is about a
  deadline. The crime is 33 allocator calls per tick on a deadline
  thread; the fix is two ampersands. The judge is the honest part: a
  replaced `operator new` counting allocations on the deadline path,
  asserting **zero at 50 ticks and at 1000**, because the sanitizers are
  silent on an accidental copy and a timing assert would measure the
  runner, not the code. `exercises/perflab/` holds the fixed state; the
  broken meter is book-and-card only.
- **New: Chapter 37 — No Repro, Dump Attached** (MINOR — an appended
  chapter; closes ROADMAP item 15, the crash-dump ticket). Nothing to
  run: a stripped customer crash report, worked as paperwork — fault
  address `0x10` read as null-plus-`offsetof` before any tool opens, the
  guilty frame *inlined out of the stack* so naive symbolication names a
  line that cannot fault, and symbols as a release-day artifact you
  either archived or do not have. Acceptance is **both device
  configurations** — the crash lived only in the one the bench never
  had. Every artifact in the chapter was generated from a real crash at
  `-O2`. `exercises/dumplab/` holds the fixed state.
- **New: Appendix E — Glossary** (closes ROADMAP item 10 and seeded
  issue #24). ~35 terms the book actually uses, plus the
  recognition-only five (ADL, CRTP, linkage, POD, SFINAE), every entry
  pointing at its owning chapter. Appendix letters now run A–F with no
  gap.
- **Tooling: the one-line judge takes several translation units.**
  `check.sh` and `check.ps1` accept multiple `.cpp` files before the
  vendor argument, compiled in the order written — which is the link
  order, the thing Chapter 32's two-order test turns on. The multi-TU
  ticket cards and chapters now print the judge wherever the canonical
  flags apply; the deliberately raw compiler lines stay (exitlab's plain
  build and dumplab's `-O2` build exist to reproduce the customer's
  silence, which the judge by design cannot). The buildlab-msvc job
  smoke-tests the new form — two sources plus a run argument — with an
  exit-code guard after every line, because a pwsh step only reports its
  last line's verdict.
- Smaller appends from the same review tier: Chapter 8's bug/value/event
  classification drill (ten scenarios, verdicts behind a fold, two
  deliberate context flips); Chapter 24's "Week 2 and beyond" retention
  schedule and "Reproduce it cold" closers on all six ticket chapters;
  README's team-lead section (what to assign, how to verify the labs
  happened, review-don't-rescue pairing); an Appendix D refresh (named
  guides with the division of labour, C++ Insights, the conference
  archives, the EMC++ C++14 caveat); Appendix B's new Deadline-code
  group and two Debugging principles, with the symptom index two rows
  longer. ROADMAP re-sequences P/Invoke (item 9) ahead of
  const-correctness; appends now land at Chapter 38+ / Appendix G+.

## [0.5.0] — 2026-08-04

The ticket arc, completed. v0.4.0 delivered the first two scenario chapters;
this release delivers the last two and closes ROADMAP item 11 outright —
along with item 7, both carried-over items, and two of the five seeded
launch issues. The four tickets now cover the four ways diagnostic work
actually arrives: produce the evidence (32), read the attached evidence
(33), work without tools (34), and repair a misread convention with a type
(35) — each lab proven in CI by exactly the thing one build cannot show.
MINOR: two chapters, two labs and a third vendor SDK appended; no existing
chapter, Finding, Recipe or appendix letter changed meaning.

- **New: Chapter 34 — Parse This Capture** (MINOR — an appended chapter;
  closes ROADMAP item 7 *inside* item 11's format, one contribution for two
  entries, exactly as both entries predicted). The ticket is a bring-up,
  not a regression: the vendor's viewer decodes every capture from the new
  bus analyzer, our ingest calls them all malformed, and the header's kind
  field reads 165 on a frame the viewer calls kind 1. Attached: a
  twenty-byte hex capture and the vendor's ICD table ("8 bytes, network
  byte order, no padding"). The shipped parser transcribed the table into a
  struct and `reinterpret_cast` it onto the bytes — Chapter 9's
  "serialization/interop only" question mark, cashed.
  - **This ticket's inversion: the toolchain goes silent.** The canonical
    flags stay green on all three bugs — padding, byte order, aliasing —
    so the oracle is the reader's own hand decode of the capture, done on
    paper before the compiler contributes anything but confirmation.
  - **Two-stage diagnosis, every number run-verified.** The overlay
    scrambles: `sizeof(Header)` 12 against the ICD's 8, offsets 0/4/8/10
    against the document's 0/1/5/7, and the 165 findable in the dump — it
    is the next frame's sync byte. `#pragma pack(1)` then mirrors:
    sequence 16777216 (`0x01000000` — the hand decode's 1, backwards),
    length 512 (the mirrored 2). *Padding scrambles, endianness mirrors —
    and the first bug hides the second.* Verified bonus pitfall: packing
    silences the one report UBSan had, since a packed struct's alignment
    requirement drops to one — the unaligned frame-2 overlay draws a
    genuine misaligned-address report unpacked and runs quietly packed.
  - **The fix: the wire gets offsets, the struct gets the results.** Named
    offsets cited to the ICD; readers that spell the wire's byte order
    with shifts — no host detection, no `#if`, correct on any host;
    structs survive only as destinations. `exercises/capturelab/` holds
    the fixed state (the broken overlay parser is book-only); `build_all.sh`
    asserts the decode against the hand-decoded values, with the capture's
    second frame deliberately at offset 10 — offset-independence is part
    of the claim, and one aligned frame cannot prove it.
  - One new key principle, in a new Appendix B group — **Wire formats** —
    mirrored in the same commit. Closed seeded issue #20.
- **New: Chapter 35 — Still Live at Unload** (MINOR — an appended chapter;
  closes ROADMAP item 11 and the carried-over Bestiary Shape 3 gap in one
  contribution: *"the vendor upgraded the SDK, and the new API is
  refcounted"*, delivered as written). Chapter 17's vendor ships SDK 2.0 as
  a **new vendor drop** — `exercises/comlab/FakeSDK2.*`, with `fakesdk/`
  untouched, because version 1.x did not change and neither do its files.
  Payloads become shared, reference-counted objects, and the mechanical
  port misreads the migration notes' two sentences in opposite directions
  at once: it never releases what it acquired, and releases what it peeked.
  - **The run-verified centrepiece is the cancellation.** The two bugs
    cancel on the active Thing — its leak (+1) and its over-release (−1)
    sum to zero — so the host's counter reports **4 of 5** still live, and
    the object the port mistreated worst is the one the counter cannot
    see. Fixing the leak alone makes the plain build print 0 and exit 0 —
    it looks completely fixed, because the dangling decrement corrupted
    the counter's own arithmetic — while promoting the over-release crash
    from "two customers, sometimes" to every close, with the culprit line
    sitting ON the report's freed-by stack: the deliberate inversion of
    Chapter 33, where the crime was a decision no stack could name; here
    it is an event, and the report hands it over.
  - **The fix is a type, not a patch.** `ref.h`'s `ThingHandle`: the
    migration notes' two sentences as two named constructors — `adopt` for
    references you were handed owning, `share` for borrows you choose to
    keep — copy retains the *claim* (the Chapter 15 Buffer's Rule of Five
    with one substitution), copy-and-swap, and a destructor that pays on
    every path, including the `continue` that silently leaked. The fixed
    port contains not one `Retain` or `Release`: Shape 3's "never call
    Release by hand", turned from advice into a greppable property.
  - **Two judges in CI, one per direction of a refcount mistake.** The
    binary asserts the vendor's live-object counter reaches 0 after
    shutdown (a release too few — the direction no macOS sanitizer names),
    and the sanitizers catch the opposite (a release too many — the
    direction no counter can be trusted about, as the fold demonstrates),
    with copies of the handle in play. `check.sh` grows a `comlab` vendor
    option. One new key principle in Appendix B's C-style SDK group;
    Chapter 16's Shape 3 gains a one-sentence forward pointer. Closed
    seeded issue #22.
- **Project: item 11 closed as listed, format left open** (PATCH-level
  docs). ROADMAP's item 11 heading reads DONE (Chapters 32–35) with the
  note that the ticket *format* stays open — new scenario chapters arrive
  by PR against CONTRIBUTING's questions, graded as ever on questions 9
  and 10; item 7 and both carried-over items are marked DONE; the append
  point moved to Chapter 36 across ROADMAP, the chapter issue form and
  CONTRIBUTING's file range. `exercises/README.md` now counts seven
  directories holding their reference in the open, four of them tickets,
  and states the vendor-drop rule comlab added: an upgraded SDK is a new
  drop, never an edit to the old one. CLAUDE.md's hard invariant 2 names
  Chapter 35 alongside 17/18 for the `Fake*` sync rule.

## [0.4.0] — 2026-08-03

The book crosses the bridge its exercise chapters could not: from task cards
to tickets. Every Part V exercise announces its diagnosis before the symptom
— pedagogically right, and exactly what a job never does. This release
delivers the first two scenario chapters of ROADMAP item 11, one for each
way work actually arrives: as a symptom you must produce the evidence for
(Chapter 32), and as a symptom with the evidence already attached and
unread (Chapter 33). MINOR: two chapters and two labs appended; no existing
chapter, Finding, Recipe or appendix letter changed meaning.

- **New: Chapter 32 — It Crashes on Exit** (MINOR — an appended chapter and
  the first instance of the ticket shape: the symptom opens the chapter, no
  concept is named in advance, the broken code lives in the lab's task card
  for the reader to recreate cold, and the diagnosis sits behind a spoiler
  fold like any reference solution). The ticket: a segmentation fault
  *after* `exit`, in `__cxa_finalize`, on the customer's machine only —
  since the release that added one audit line to a destructor. The bug is
  the **static initialization order fiasco**, asserted twice in the book
  (Chapter 28's "a namespace-scope vector would be a bet on initialization
  order", Chapter 30's "static initialization across modules is not
  ordered") and demonstrated nowhere until now: across translation units
  the standard refuses to order construction, destruction is the reverse of
  whatever order the linker produced, and the program's fate turns on which
  of two object files came first on a link line nobody chose. The fix is
  construct-on-first-use plus the load-bearing second half — touch your
  dependencies in your constructor, so reverse destruction becomes a
  consequence instead of a bet.
  - **The acceptance test has structural teeth.** `exercises/exitlab/`
    holds the fixed state, and `build_all.sh` builds it **twice with the
    translation units in opposite orders** and runs both — order
    independence is the fix's whole claim, and one build cannot prove a
    claim about two.
  - **Two pitfalls were paid for during the writing** and are kept in the
    chapter: the first draft's `std::vector<std::string>` logger crashed at
    exit and ASan said *nothing* — libc++'s container annotations un-poison
    a freed block on `push_back` before constructing into it (Finding 10's
    file grows); and libc++'s `unique_ptr` nulls its pointer during
    destruction where libstdc++ leaves it stale, which would have split the
    demonstration into two per-platform mechanisms — so the lab's logger
    holds a raw `char*`, one mechanism on both CI platforms.
  - **The fiasco's first act is shown too:** make the auditor's constructor
    log as well, and the bad link order now fails *before* `main`, with
    AddressSanitizer's `initialization-order-fiasco` detector naming it
    outright under `check_initialization_order=1:strict_init_order=1`.
  - One new key principle, in a new Appendix B group — **Static
    lifetime** — mirrored in the same commit.
- **New: Chapter 33 — Here Is the Report** (MINOR — an appended chapter;
  ROADMAP item 11's "Here is the report" candidate, delivered as specified:
  an ASan report plus the source that produced it, and the reader locates
  the bug **from the report alone**). The second ticket adds the inversion
  the job supplies: the report arrives attached — a nightly sanitizer job
  that had been red since the feature merged, unread — and the rule is *no
  compiler until the diagnosis is written down*. Chapter 31 taught the
  reading; this is the exam. The bug: a `Sensor*` pinned into a
  `std::vector` before a growth-triggering `push_back` — Chapter 11's trap
  and Chapter 21's Task 3 arriving as a month-one ticket, a hot-plugged
  ninth sensor away from a dashboard reading 0.0 that no eight-sensor rig
  can reproduce.
  - **The exam's lesson: the guilty line is in none of the report's three
    stacks.** The stacks name events — a read, a free, an allocation; the
    bug is a decision (keeping a pointer across a mutation), and no stack
    ever points at a decision. The region arithmetic does real work: `40
    bytes inside of 128-byte region`, with `sizeof(Sensor) == 16`, names
    element 2's `last` — the pinned sensor, the very number on the
    dashboard — from an address and a struct definition. The quoted report
    is genuine (macOS/AppleClang), with freed-by and allocated-by both
    walking eight libc++ frames before landing on `Registry::add`, a
    function that only ever adds.
  - **Both tempting non-fixes were built before being asserted**, and are
    kept as pitfalls: storing a copy (`Sensor watched = *reg.find(3);`) is
    the C# reflex, sanitizer-clean, and wrong from the *first* frame — the
    copy freezes at pin time, value semantics rather than a lifetime bug;
    and `reserve(16)` runs clean at nine sensors and reproduces the
    identical report at seventeen — the same bet with a higher table limit.
  - **The acceptance test, same reasoning as exitlab's:**
    `exercises/reportlab/` holds the fixed state (the broken 2.6.0 main
    lives in the task card and the chapter — it exists to fail), and
    `build_all.sh` runs it at **0 hot-plugs and at 100**, because
    growth-independence is the fix's whole claim and one count cannot prove
    a claim about all of them.
  - One new key principle — the loan: store the key, borrow at the point of
    use, and document how long the pointer lives — mirrored in Appendix B's
    STL group in the same commit.
- **Project: item 11 is half delivered, and everything knows it**
  (PATCH-level docs; no book content beyond the chapters above). ROADMAP's
  item 11 carries delivered-notes for both tickets and keeps item 7's
  capture and the COM upgrade as the remaining candidates; the append point
  moved to Chapter 34 in the ROADMAP intro, the chapter issue form and
  CONTRIBUTING's file range; `exercises/README.md` gains the two ticket
  rows and now counts five directories holding their reference in the open,
  with the ticket-shaped pair's shared rule stated once — each fix's claim
  is exactly what one build cannot prove; CLAUDE.md mirrors all of it.

## [0.3.0] — 2026-08-03

The book gains its third index. It was indexed by concept (the Contents) and
by bug (the Findings log, Chapter 31's report shapes); it is now also indexed
by *task*, keyed on the C# API being reached for — a new Appendix F with
sixteen compiled, CI-asserted recipes — plus the named-library prose the
appendix's own admission rules exclude, and the review bar all of it was
written against. MINOR: an appendix was appended and recipe numbers join the
public contract; no existing chapter, Finding or appendix letter changed
meaning. Appendix F deliberately skips E, which stays reserved for the
glossary (ROADMAP item 10) — letters append like numbers, and none moved.

- **New: Appendix F — The Rosetta Cookbook** (MINOR — a new appendix;
  Recipes 1–16 are numbered, citable, and append-only like Findings). The
  page for the mid-task minute: an index table from the C# name to the
  recipe, then one strict shape per recipe — **In C# / The recipe / Why it
  looks like this / Trap** — with whys that cross-reference the owning
  chapter rather than re-teach, traps as one-line `[!WARNING]`s, and the
  headers each recipe needs. Chapter 11's LINQ table is cited as the
  format's in-book precedent and serves as the collections domain's index.
  Landed in three batches:
  - **Recipes 1–8, the week-one seed:** `File.ReadAllText`, `string.Split`
    (with the trailing-empty-field difference from C# stated), `string.Join`,
    `StringBuilder` (`std::string` *is* the builder — the rare reflex to
    unlearn outright), `string.Format` (the honest no-interpolation-in-C++17
    answer), `Stopwatch`, `using`/`IDisposable` as `unique_ptr` with a custom
    deleter ("deleter" had zero hits in the whole book), and `TryGetValue`
    (`find` versus the inserting `[]`).
  - **Recipes 9–13, the filesystem batch and async:** write a file (the
    flush-and-check, because destructors cannot report a full disk),
    `Path.Combine` (`/=` versus the gluing `+=`), the
    `File.Exists`/`Directory.Exists` pair, `Directory.GetFiles` (listing
    order is unspecified), and `Task.Run`/`await` as
    `std::async`/`.get()` — including the trap that the future blocks in its
    destructor, so fire-and-forget silently serializes the program.
    `std::filesystem` had appeared exactly once in the book before this.
  - **Recipes 14–16, the authoring side:** expose an event (token-based
    unsubscribe, because `std::function` has no `operator==` — and the C#
    event leak *inverted*: nothing keeps a dead subscriber alive), print a
    diagnostic you will actually see (Chapter 28's buffering lesson made a
    habit; `std::endl` is a flush), and the repeating timer the standard
    library does not have (the host's tick is the timer in plug-in work;
    otherwise a worker thread whose destructor join *is* the `Stop()`).
  - **Every listing compiles and is asserted.** `exercises/cookbook/` holds
    one translation unit per domain — nine in all — each with a `main()`
    asserting what its recipes claim, trap demonstrations included, built
    and run by `build_all.sh` under the canonical flags. The listings are
    quoted verbatim (checked byte-for-byte), under the testlab sync
    discipline: editing either side means editing both in the same commit.
    The Recipe template sits in CONTRIBUTING.md beside the Finding template,
    and the PR checklist knows the shape.
  - **A latent bug found by the wiring:** `build_book.sh` hardcoded the
    appendix letters as `[A-D]` in both its file glob and its link rewriter,
    so a new appendix silently vanished from the single-file build — nav OK,
    markup OK, one file short. Both patterns now read `[A-Z]`; the tell was
    the file count in the build output.
- **New: Chapter 27 — "The batteries C# included"** (MINOR — an appended
  section; no number moved). What the appendix's admission rules exclude,
  said in prose instead of left silent: `System.Text.Json`, `XDocument` and
  `HttpClient` are in the box in C#, and every one is a dependency decision
  here — with the starkest fact as a Surprise-for-C#-devs callout: the C++
  standard library has no networking at all, not just no `HttpClient` but no
  sockets. The names the ecosystem converged on, all open-source study
  material under the naming rule: nlohmann/json and RapidJSON; pugixml and
  TinyXML-2, with the note that this line of work mostly *reads* XML
  (`.vcxproj`, Qt `.ui`) rather than parses it; libcurl — a Bestiary-shaped
  C API, so consuming it is exactly the Chapter 17/18 skill — plus cpr and
  Boost.Beast; and SQLite as the data battery, Bestiary Shape 1 in
  production, meaning Chapter 17 already trained the reader for it. Closes
  by stating why none of them appears in the exercises: the offline,
  standard-library-only rule doing its job.
- **Project: the questions every piece of material answers** (PATCH-level
  docs; no book content changed). The handbook's four goals — learn, change
  the mindset, practice, help solve real problems — written down as twelve
  review questions in CONTRIBUTING.md, each answered *by a mechanism in the
  material*, never by an intention in the PR description; the Finding
  template shown as the list instantiated. ROADMAP gained the matching gate
  (what earns an item a place, and what "done" has to answer), CLAUDE.md
  mirrors the list, the chapter and new-exercise issue forms ask for a
  sketch against it, and the PR checklist asks for the walk. Every entry
  below this line was written against that list — it shipped first on
  purpose.
- **Project: ROADMAP items 11–13** (PATCH-level docs). Item 11, *scenario
  chapters — tickets, not task cards*: the bridge between exercise and job,
  with "It crashes on exit" (the static-order fiasco, asserted twice in the
  book and demonstrated nowhere), "Here is the report", and item 7 / the COM
  lab named as candidate tickets. Item 12, *the Rosetta Cookbook* — opened,
  then delivered by this release, its entry now carrying the audit verdicts
  (REST and JSON/XML parsing stay prose-only; the hand-rolled mini-JSON
  parser is an item 11 candidate; `FileSystemWatcher` is an honest-answer
  recipe candidate). Item 13, *SOLID without the runtime*: the reader's
  design vocabulary un-fused from the .NET machinery it arrived welded to —
  zero hits for the names against full fragment coverage without them. Also
  corrected in passing: the ROADMAP intro's stale "Chapter 27 onward" is now
  32, matching the issue forms.

## [0.2.1] — 2026-08-03

A pre-announcement accuracy audit, applied in full and then audited itself.
Five rounds of corrections across the book — the findings the audit confirmed,
the four its two verifiers split on, the low-severity candidates it swept but
never adversarially checked, and finally the defects the corrections themselves
introduced — plus dual licensing, the contributor funnel, and CI that now runs
the book's platform-specific claims rather than citing them.

No chapter, Finding or appendix number changed meaning, and the two key
principles whose wording changed were mirrored in Appendix B in the same
commit. That is what makes the whole release PATCH: every claim in it is a
correction to something already numbered, and nothing was appended or moved.

- **Corrections: seven defects in the audit corrections themselves** (PATCH —
  no number moved, no key principle changed). A pass back over the four
  correction rounds, re-running the claims against a compiler and vendor
  documentation rather than re-reading the prose. The substantive rewrites all
  held — Chapter 21's `container-overflow`, Chapter 25's Finding 10, Chapter
  28's three-stack report, the `/Zc:nrvo` and `/GL`/`/LTCG` details — but seven
  edits introduced or left problems of their own:
  - **Chapter 3** — the 512 KB secondary-thread stack is macOS's; glibc sizes
    a new thread from `RLIMIT_STACK`, the same ~8 MB the main thread gets, as
    Chapter 29 already said. And the `/RTCu` fix contradicted the sentence
    above it: the example never prints `-858993460`, because `/RTCu` stops the
    program first.
  - **Chapter 6** — the C++17 elision correction was anchored to
    `Take(MakeBuffer())`, a line demonstrating overload selection rather than
    return-value elision. The two mechanisms are now stated separately.
  - **Chapter 10** — the C++17 caveat named `std::format` as a C++20 exception
    "above"; the chapter never shows it.
  - **Chapter 27** — "v140–v145" implies a v144 that does not exist.
  - **Chapter 28** — the `-O1` rebuild offered as proof of the destructor's two
    ABI entry points does not reproduce: the frames are inlined away entirely
    and the summary line moves. Also a list severed by an unclosed
    interjection, and a transcript line number (`:17` twice, should be `:17`
    and `:18`).
- **Corrections: the audit's 36 low-severity candidates, all verified and
  applied** (PATCH — no number moved). The audit swept these but did not
  adversarially verify them, so each was checked here first; all held. Two key
  principles changed wording, each mirrored in Appendix B in the same commit.
  - **Chapter 1** — the destructor guarantee does not survive an uncaught
    exception or `std::exit`; "zero runtime cost" for `unique_ptr` is the
    phrasing experts attack, and is now the Core Guidelines' accurate version.
  - **Chapter 2** — `class` and `struct` also differ in default *base* access.
  - **Chapter 3** — per-platform stack sizes; MemorySanitizer runs on Linux,
    NetBSD and FreeBSD; C# heap placement is language semantics, and recent
    runtimes stack-allocate non-escaping objects.
  - **Chapter 4 + Appendix B** — a default member initializer can also
    initialize `const` and reference members; what they can never be is
    *assigned*.
  - **Chapter 5** — virtual calls devirtualize and inline when the dynamic type
    is provable.
  - **Chapter 6** — since C++17, elision for a prvalue return is mandatory, not
    an optimization (matching the Chapter 14 correction already made).
  - **Chapter 7** — C#'s `where` clauses predate concepts by 15 years, not 20.
  - **Chapter 8** — `throw()` never named which exceptions a function throws;
    that was the dynamic exception specification. Unwinding is two-phase.
  - **Chapter 9** — `std::wstring` is standard C++, and UTF-16 only where
    `wchar_t` is 16 bits.
  - **Chapter 10** — ranges and `std::format` are C++20, so not "all of this".
  - **Chapter 11** — LINQ's `OrderBy` is stable; `std::stable_sort` is the match.
  - **Chapter 12** — ODR is once per *translation unit*, with the class/inline
    /template exception that makes headers work at all.
  - **Chapters 14 and 25** — the Tracer prints from five special members plus an
    ordinary constructor, not six; `<cstdio>` added for `std::snprintf` to both
    the listing and `solutions/tracer.cpp`.
  - **Chapter 15** — a `unique_ptr<int[]>` member deletes your copy operations.
  - **Chapter 18** — removing `Rebind()` is a use-after-free ASan reports, not a
    silent miss; a footnote on `extern "C"` linkage for the trampoline.
  - **Chapter 19** — `EOF` is the one defined exception to the `<cctype>` rule.
  - **Chapter 24** — Day 3's `std::erase_if` needs C++20, which Day 0 now says.
  - **Chapter 25** — unwinding before `std::terminate` is implementation-defined;
    MSVC's debug heap fills `0xcd`; `const` on a member function is shallow, so
    the single accessor compiles; `At(2) = 7` is always ill-formed.
  - **Chapter 26** — CMake usage requirements are transitive past one hop.
  - **Chapter 27** — binding redirects are .NET Framework only.
  - **Chapter 28** — frameworks give crash *attribution*, not isolation, on POSIX.
  - **Chapter 29** — per-platform thread stacks; `map::contains` is C++20, so
    the C++17 spelling is used; TSan exits 66 on Linux, not 134.
  - **Chapter 30** — cross-module `new`/`delete` is UB when the allocators
    differ, not unconditionally.
  - **Appendix B + Chapter 27** — binary compatibility is per-ecosystem and
    fragile rather than absent.
- **Corrections: the four disputed audit items, all upheld on re-verification**
  (PATCH — no number moved, no key principle changed). These were the ones the
  audit's two verifiers split on, so each was re-checked from scratch:
  - **Chapter 3** — `/RTC1` is `/RTCs` *plus* `/RTCu`, so a Visual Studio Debug
    build stops on the chapter's own uninitialized read rather than silently
    showing `-858993460`. The silent pattern is what you get once a variable is
    aliased, or for members and array elements — which is the case the chapter
    goes on to discuss.
  - **Chapter 9** — dropped "interned" from the C# string recap; interning is
    not a property of the type, and by default the C# compiler does not even
    guarantee it for literals.
  - **Chapter 24** — Day 2 no longer promises that ASan catches all three
    deliberate breaks. The non-virtual destructor produces no sanitizer report
    at all on macOS/arm64; it is a compiler warning and then a leak.
  - **Chapter 25, Finding 1** — names the small-string optimization. The
    Tracer's own strings are short enough that the "allocates a brand-new heap
    block" step allocates nothing; the Finding's argument is unchanged.
- **Corrections: the remaining fifteen accuracy fixes from the same audit**
  (PATCH — again no chapter, Finding or appendix number moved). One key
  principle changed wording, mirrored in Appendix B in the same commit:
  - **Chapter 3** — the heap example labelled `w`, the *stack* Widget from the
    line above, as living on the heap; and use-after-move was listed as
    unconditional UB, when a moved-from standard library object is valid but
    unspecified and only precondition-violating operations are undefined.
  - **Chapter 9** — a C-style cast can never be a `dynamic_cast`; the one
    checked cast is exactly the one it cannot do.
  - **Chapter 11** — `std::array` is not "stack-allocated": its elements are
    stored inline wherever the object lives. A runtime-sized C# `T[]` maps to
    `std::vector`.
  - **Chapter 12** — .NET does have binary-compatibility concerns. What it
    lacks is compiler-ABI mismatch, which is what the sentence now says.
  - **Chapter 16** — HIDAPI registers no callbacks at all and ASIO's carry no
    context, so neither has Shape 2's defining feature; both are now named as
    partial members rather than examples.
  - **Chapter 21** — `push_back` always invalidates the past-the-end iterator,
    so `reserve` alone does not fix the broken range-`for`.
  - **Chapter 27** — MSVC has guaranteed binary compatibility across v140–v145
    since 2015; the ABI-churn example is now pre-2015 MSVC and the `/GL`
    carve-out.
  - **Chapter 28** — four: `std::source_location` has supplied the call site
    since C++20; the four `[ ok ]` lines are visible at a terminal and vanish
    only when redirected; ASan's exit 134 is macOS-only; Catch2 v3 is not a
    single-header drop-in.
  - **Chapter 31** — ASan's exit code and its missing column numbers are both
    macOS artefacts, not sanitizer behavior.
  - **Appendices A and B** — clang's opt-in `unsigned-integer-overflow` check
    does report the legal wrap; "no sanitizer will ever warn me" became "my
    `-fsanitize=address,undefined` build stays silent about it".
- **Corrections: six accuracy fixes from a pre-announcement audit** (PATCH — no
  chapter, Finding or appendix number moved, and no key principle changed).
  Each was verified against the standard, vendor documentation, or a compiler
  before editing:
  - **Chapter 7** — the bolded thesis taught an erasure model of C# generics.
    .NET generics are reified: the runtime specializes per value type and
    shares one instantiation across reference types. The real contrast is
    *when* and *from what*, which is now what the sentence says.
  - **Chapter 14** — NRVO was presented as guaranteed C++17 elision. C++17
    mandates elision only for prvalue returns; `MakeTracer` returns a named
    local. Also flagged that the chapter's own `cl /std:c++17` line does not
    enable `/Zc:nrvo`, so MSVC prints a move the annotated output omits.
  - **Chapter 21** — `vector::erase` frees nothing, so ASan cannot report
    `heap-use-after-free` for Task 1. It reports `container-overflow`, whose
    giveaway is an allocation stack and no "freed by" stack.
  - **Chapter 25, Finding 10** — the `std::exchange` self-move walkthrough was
    arithmetically inverted: `x = std::exchange(x, v)` leaves `x` unchanged, so
    the printed code double-frees and ASan reports it loudly. The silent
    outcome the Finding is about belongs to the two-statement steal, which is
    now shown alongside it. The Finding's conclusion, number and Habit are
    unchanged.
  - **Chapter 28** — the narration misread its own double-free report. The
    excerpt was one stack, not two deletes; the doubled `~Buffer` frames are a
    single destructor's two ABI entry points. All three stacks are now shown.
  - **Chapter 30** — delete-through-virtual-destructor was stated backwards.
    The deleting destructor is emitted with your class and calls *your*
    `operator delete`, not the caller's. `Destroy` is still the advice, for
    the accurate reason.
- **Project:** dual licensing, made explicit before the first outside
  contribution lands — the book text under CC-BY 4.0 (new
  `LICENSE-CC-BY-4.0`), all code under MIT, including every code sample inside
  a chapter, so a pasted snippet carries no attribution obligation. `LICENSE`
  is the unmodified MIT text and the split is stated in a new `NOTICE` file —
  a scoping preamble inside `LICENSE` breaks GitHub's license detection, which
  is worth knowing before anyone tries it again. README, CONTRIBUTING and the
  book's front matter say the same thing (PATCH-level repo change; no book
  content changed).
- **Project:** the contributor funnel — issue forms for the four contribution
  kinds (Finding, correction, new exercise, new chapter), a PR checklist
  naming every check CI runs, and the labels to match (PATCH-level repo
  tooling; no book content changed).
- **Project:** CI now *executes* the book's platform-specific claims instead of
  citing them (PATCH-level repo tooling; no book content changed). The audit
  above kept finding the same error class — behavior observed on one machine
  written down as the rule — and the corrections for the Linux half had to be
  sourced from compiler-rt documentation, because there was no Linux to run
  them on. Two new checks close that:
  - `scripts/check_platform_claims.sh` runs the sanitizer demonstrations and
    asserts what each chapter promises for the platform it is on: ASan's exit
    code (134 on macOS, 1 on Linux), TSan's (134 / 66), whether LeakSanitizer
    reports at all (no on macOS/arm64), and whether a frame carries a column
    number (Chapter 31's atos-vs-llvm-symbolizer point). CI runs it on ubuntu
    **and** macos, because a platform overclaim is invisible from one platform
    by construction. The broken programs are generated into a temp directory,
    never committed, so `solutions/` stays clean.
  - `buildlab-msvc` now also checks Chapter 14's `/Zc:nrvo` claim — the
    chapter's own `cl /std:c++17 /W4 /EHsc` line must print exactly one extra
    move-construction in the RVO section, and adding `/Zc:nrvo` must print
    none. It is the one claim in the book resting on a vendor default rather
    than on the standard, so it is pinned from both sides.

  All five claims held on first run. What changed is that they are now provable
  rather than merely correct.

## [0.2.0] — 2026-08-02

Navigation and usability, two corrections, the first six appended chapters, a
new exercise, the split of the book into per-chapter files, and GitHub-native
rendering; no existing chapter or Finding number changed meaning. That last
point is what makes this MINOR rather than MAJOR: the version contract is about
what a *number* refers to, and nothing renumbered — only the file the text lives
in changed.

- **New: two mind-shift gaps closed, and the indexes finished** (MINOR — a
  new appendix section is appended content; the rest is index polish, and no
  chapter, Finding or appendix number moved). Four small additions that share
  one subject: the things a C# developer trips on because the managed runtime
  never let them happen.
  - **Chapter 3 — uninitialized values as a first-class UB citizen.** The UB
    greatest-hits list omitted one of the actual greatest hits, and the one the
    reader is least equipped for: in C# fields are zeroed by the runtime and
    definite assignment makes reading an unassigned local a *compile error*, so
    this bug has never once happened to them and no reflex exists for it. Now a
    list entry and a section of its own — indeterminate values, why Debug shows
    0 on Linux and macOS but `0xcccccccc` under MSVC's `/RTC1` while Release
    shows garbage (the chapter's own works-on-my-machine signature), the fact
    that Address and UB sanitizers do not report an uninitialized read at all
    (that is MemorySanitizer, Linux-and-clang-only), the three places the rule
    reaches past a plain local (a member missed by the initializer list,
    `new T[n]` without braces, an API struct), and the habit, including
    treating a *might be used uninitialized* warning as a certainty, under
    whichever of the three compilers' spellings you meet it. Chapter 4,
    Finding 7 and the `= {}` idiom are cross-referenced
    rather than re-taught, and the plug-in angle stays where it was, closing
    the UB section.
  - **New: Appendix A.7 — signed, unsigned, and `size_t`.** Appended after A.6;
    A.1–A.6 are untouched. `size_t` is unsigned where C#'s `Count` and `Length`
    are `int`, and `uint` is a type C# developers are steered away from — so
    the reader meets unsigned arithmetic in the first loop they write, with no
    instinct for it. The two daily collisions: `int i < v.size()` and its
    `-Wsign-compare` warning, with both fixes (match the counter, or drop the
    index entirely and use a range-for — Chapter 2's `const auto&` reflex); and
    `size() - 1` on an empty container, which wraps to 18446744073709551615 and
    turns a guard that should reject every index into one that accepts every
    index. The asymmetry is the reason the section exists: unsigned wrap is
    *defined* and signed overflow is UB, and being legal is exactly what makes
    the wrap the more dangerous of the two — UBSan reports the signed overflow
    and has nothing to say about the wrap. The broken guard compiles clean
    under `-Wall -Wextra`, runs clean under Address and UB sanitizers, and the
    only symptom is the answer. `std::ssize` and the `std::cmp_less` family are
    named in one sentence as the C++20 relief. Every snippet was compiled under
    the canonical flags: the broken loop draws the quoted warning verbatim, the
    fixes are silent, and both guards were run to confirm the wrong answer and
    the right one. One new key principle, mirrored in Appendix B under
    errors/casts/strings/UB in the same commit; README's Appendix A descriptor
    line records the new topic.
  - **The Contents gains taglines for Chapters 1–13.** Parts V and VI have
    carried them since the split and Parts I–IV never did, so the reading order
    told you a title and nothing about the payoff. Each of the thirteen now
    names the trap or the stakes in the same style — Chapter 5's is the missing
    keyword and every skipped derived destructor, Chapter 12's is why the error
    came from the linker. Chapters 24 and 25 keep their bare titles, since a
    practice plan and a living log are what they say they are.
  - **`exercises/README.md` has no dashes left in the Time column.** The Tracer
    and Buffer rows carried "—"; they are now ~60 min and ~90 min, sized from
    Chapter 24's plan, where each is a day's centrepiece and the Buffer's
    ordering mistakes are heap corruption rather than style nits. The Bestiary
    reading row says ~20 min. Every other row was checked against the time its
    chapter or task card states, and all of them already matched.

- **New: Chapter 8 expanded into the error-handling worldview** (MINOR —
  substantial appended content; nothing deleted, no chapter or Finding number
  moved). The chapter showed both mechanisms correctly and explained neither
  the split nor how to choose, which left the actual mind-shift — a C#
  developer's instinct is exceptions-first, one mechanism, always available —
  untouched. Five new sections around the existing text, which stays as it
  was:
  - **"Why half the ecosystem says no"** — `-fno-exceptions` as a second
    dialect of the same syntax (most game engines, most embedded targets,
    LLVM, Google's style guide), its four reasons, and the practical
    consequence: which world you live in is a property of the build, so
    finding out is a day-one task. There is no `-fno-exceptions` for the CLR.
  - **"What a throw actually costs"** — table-based exceptions honestly: a
    happy path that costs nothing (*cheaper* than an error code, which pays a
    branch per call), a throw path that is cold, unbounded and
    non-deterministic, and unwind tables in the image whether or not you ever
    throw. This is what makes "exceptions are for exceptional" an engineering
    statement rather than style advice, and a throw in a hot loop a design bug
    that profiles as a latency spike rather than a wrong answer.
  - **"Between the two poles: the standard vocabulary"** — `std::error_code`
    and `std::system_error` with `std::filesystem`'s dual overloads as the
    visible example (the standard library shipping both forms of every
    function being the clearest admission that neither mechanism won),
    `std::optional` as "absence is not an error", and `std::expected<T, E>`
    labelled C++23 in the text, with the house `Result`/`StatusOr`/`Outcome`
    types the reader is far more likely to meet first.
  - **"Choosing: is the failure a bug, a value, or an event?"** — the section
    the expansion exists for: assert for a caller's bug, a returned value for
    an expected failure, a throw for the rare non-local one and for
    constructors, which have no return channel (Chapter 18's static factory,
    explained rather than merely used). The table's right-hand column is the
    point — in C# all three rows are one keyword, and
    `ArgumentNullException` *is* the assert case.
  - **"In the wild: C-style SDKs"** — the section every other Part II chapter
    had: status-enum encodings that are not always zero-for-success, the
    Bestiary's failure-contract question ("touched or untouched on failure?")
    and Chapter 17's documentation trap, and translation at the boundary in
    both directions, with the entry-point `catch (...)` and why it is not
    paranoia.
  - **Two new key principles, mirrored in Appendix B** in the same commit as
    CONTRIBUTING requires: the bug/value/event decision, and finding out which
    dialect a codebase speaks before writing in it. Every snippet was compiled
    under the canonical flags (the `expected` one under `-std=c++2b`); none of
    it enters `build_all.sh`, which is untouched.

- **New: Chapter 30's three worked boundaries are under CI** (PATCH-level —
  verification wiring for code the chapter already printed; the demo mains are
  new, thin, and exist to be run, and no text was rewritten and no number
  moved). The chapter derived PIMPL, a pure-virtual interface with a factory,
  and an `extern "C"` façade from one rule, printed each of them, and left all
  three where nothing could check them. They are now `exercises/abilab/`:
  `Widget.h`/`.cpp`, `IScorer.h` and `engine.h` verbatim from the listings,
  `scorer.cpp` and the rest of `engine.cpp` completing what the chapter
  excerpts, and a caller for each. Everything compiled clean under `-Wall
  -Wextra` first time; two defects no warning could see needed fixing (below),
  in the chapter and the files together, so the repository's copies and the
  chapter's listings are identical. `scripts/build_all.sh` builds and runs all
  three under the canonical flags with `halt_on_error`, since they assert values
  rather than survival. ROADMAP item 6's "still open" paragraph is resolved,
  and with it the four-chapter Part VI debt — 26, 28, 29 and 30 — that the
  *Where chapter code lives* convention was written to settle. The convention
  now stands for whatever Part VI gains next rather than for a backlog.
  - **Two translation units per binary, and that is the subject matter.** Each
    demo links a boundary's implementation against a caller compiled against
    the header alone, so what the header hides is genuinely unavailable to the
    caller. Merged into one TU the three would still build, still run, still
    pass — and prove nothing. CONTRIBUTING.md now says so, as the one line
    this chapter added to the convention.
  - **The claims are asserted, not narrated.** `sizeof(Widget) ==
    sizeof(void*)` as a `static_assert` — the premise the chapter's relink
    experiment rests on, checkable in a single build where the experiment
    itself is not. `!std::is_destructible_v<IScorer>`, so "the protected
    destructor makes `delete scorer` a compile error" is a compile-time check
    rather than a sentence. And every `Engine_*` code the header documents,
    including the null-parameter path and a `Destroy` that runs exactly once.
  - **`final` on the implementation class, in code the chapter only
    describes.** `Scorer::Destroy` is `delete this`, and without `final` that
    is `-Wdelete-non-virtual-dtor`: a class with virtual functions and a
    non-virtual destructor is a hazard for anything deriving from it. Nothing
    can derive from this one. The warning is right, and the fix is the design.
  - **Two fixes to the published headers that no warning could have found.**
    `IScorer.h` and `engine.h` were the only headers in the repository without
    an include guard, and a header written to be *published* is precisely the
    one that gets included twice: twice, `IScorer.h` was a redefinition error
    and `engine.h` a `-Wtypedef-redefinition` warning under C99, which
    undercuts the "consumable by C" line it opens with. Both now carry
    `#pragma once` — matching `Widget.h`, and the `FakeDevice.h` the chapter
    tells the reader to lay `engine.h` beside. Separately, `engine.h`'s error
    table said code 2 meant "the call did nothing": true of `Engine_Create` and
    `Engine_Score`, which write no output parameter, and false of
    `Engine_Destroy`, which has already begun freeing. A reader who took that
    sentence at its word and retried a failed destroy would double free, which
    is a poor thing to teach in the chapter about failure contracts.
  - **The break-it-first half stays book-only**, like Chapter 31's sabotage
    runs: the `Naive` layout break, the pure-virtual method inserted at the top
    of an interface, the relink against a changed `Impl`. Each needs a caller
    binary that was deliberately *not* rebuilt, and the first two exist to
    fail.

- **New exercise: the threaded callback** (MINOR — a new exercise with its
  reference solution, which is appended content rather than wiring, unlike the
  two entries below it). Chapter 29's *Try it* was the lab in chapter form and
  nothing else: no task card, no threaded code in `solutions/`, and so no TSan
  anywhere in CI. It is now `exercises/threadlab/` — a task card and nothing
  else, because the lab is the Chapter 18 device with a driver thread the reader
  builds out of `Device_Poll`, so it links `exercises/fakedevice/`'s vendor code
  where it lives and copies none of it. The reference solution,
  `solutions/device_threaded_solution.cpp`, is the chapter's step 3 in full
  rather than the naive version: a poller thread owning every `Device_*` call
  (the header documents nothing about cross-thread use, so by Chapter 16's rule
  it is not thread-safe), a mutex-guarded job queue carrying open, register,
  unregister and close, and the teardown the chapter derives — Sink with mutex
  and alive flag, a heap `weak_ptr` as the SDK context, the flag published
  before an unregistration that is now *deferred*. Deferring it is what earns
  the exercise: it turns a device that only ever calls back on one thread into
  the non-quiescing SDK the fix exists for. `scripts/build_all.sh` builds and
  runs it under the canonical flags with everything else, and again under
  `-fsanitize=thread` in a section of its own, since the two sanitizers do not
  combine; `scripts/check.sh` grows a `SAN` variable so a learner can run their
  own attempt both ways. ROADMAP item 4 and the carried-over threaded-callback
  gap are both closed; item 6's note now records Chapter 30 as the last of the
  four.
  - **The chapter's never-freed context needed its sanctioned variant, not its
    listing.** Chapter 29 leaves the callback contexts deliberately unfreed and
    offers, in one sentence, the alternative of owning them at file scope and
    clearing them after the SDK's thread is joined. The reference solution has
    to take that route: LeakSanitizer runs at exit on Linux — where CI runs —
    and would report every context as a leak, turning the new gate red. "Do not
    free it in the destructor" and "do not leak it" are both satisfiable, but
    only in that order, and joining first is the only moment the ordering is
    knowable.
  - **The gate was tested by breaking it.** Putting `delete ctx_` back at the
    end of the destructor — the chapter's own step 4 — gives
    `heap-use-after-free` under AddressSanitizer on ten runs out of ten, and
    **nothing at all** under ThreadSanitizer on six. That is the chapter's last
    pitfall reproduced on this program: running one sanitizer and calling it
    covered would have shipped the bug. Step 4 itself stays book-only, like
    Chapter 31's sabotage runs, because it exists to fail.
  - **`--require-tsan`, and a probe that does the thing.** The new section
    follows the `--require-cmake` bargain — SKIPPED on a machine without the
    tool, never skippable in CI — with one difference now written into
    CONTRIBUTING.md: it decides by compiling *and running* a trivial
    instrumented program, because ThreadSanitizer can be perfectly well
    installed and still fail to start.

- **New: Chapter 28's test harness and suite are under CI** (PATCH-level —
  verification wiring for code the chapter already printed; no text rewritten
  and no number moved). The chapter's claim is that testability is structural —
  a class sharing a translation unit with `main()` cannot be linked into a test
  binary that has its own — and it was true of this repository's own Buffer,
  which sat in `solutions/buffer.cpp` next to the demo. So the class moved,
  byte-identical, to `solutions/Buffer.h`; `buffer.cpp` keeps its `main()` and
  includes it. The Chapter 15 reference solution is two files now, which is the
  Chapter 28 lesson applied rather than described, and duplicating the class
  into the lab was the alternative — two copies of a class the book teaches
  would drift. The harness and suite land as `exercises/testlab/` (`tiny_test.h`
  and `buffer_test.cpp`, verbatim from the listings, plus a task card), and
  `scripts/build_all.sh` builds the suite with `-I solutions` under the
  canonical flags and runs it, printing its tally, with a non-zero exit as red.
  Both files compiled clean under `-Wall -Wextra` with no adjustment, so the
  repository's copies and the chapter's listings are identical; the green output
  matches the one printed in the chapter line for line. What stays book-only is
  the chapter's closing demonstration — the broken move constructor that leaves
  every assertion passing while ASan reports a double free — because, like
  Chapter 31's sabotage runs, it exists to fail. ROADMAP item 3's "still open"
  paragraph is resolved, and item 6's closing note now records Chapters 29 and
  30 as what remains.
  - **Convention amended: a header in `solutions/`.** The rule written down
    with Chapter 26 said `solutions/` stays flat and stdlib-only `.cpp` files.
    It stays flat and stdlib-only, but a header is now permitted there exactly
    when a chapter requires the demo/test split — Chapter 28 being the case
    that forced it. CONTRIBUTING.md and CLAUDE.md both say so.
  - **Correction: the suite now covers move assignment.** Chapter 28's *Try it*
    asked for a self-move test and the printed suite had none — six tests that
    between them never called `operator=(Buffer&&)`, the one member that frees a
    block by hand before stealing. Two were added, to the chapter's listing and
    the lab file in the same commit: `MoveAssignFreesTheOldBlock` and
    `SelfMoveIsHarmless`. Eight tests now, and the chapter's printed output says
    so. The second one earns its place the chapter's own way — delete the
    self-move guard from `Buffer.h` and ASan reports a heap-use-after-free
    naming the test, where the assertions alone would have read stale values and
    passed.
  - **Chapter 15 says where its solution lives.** Its fold now opens by noting
    that the listing is one file, as the reader's attempt will be, while this
    repository's copy is the two Chapter 28 splits it into. The code is
    unchanged; only the file it sits in is.
- **New: Chapter 26's CMake material is under CI** (PATCH-level — tooling and
  verification, no text rewritten and no number moved). The chapter's snippets
  were the only code in the book `scripts/build_all.sh` did not verify; they
  now exist assembled, as `exercises/buildlab/CMakeLists.txt` — the shape the
  chapter ends on, library plus executable, warnings PRIVATE, sanitizers
  behind `GREETER_SANITIZE` and carried by the INTERFACE target — and the
  script configures, builds and runs it twice: default, then Debug with
  `-DGREETER_SANITIZE=ON`, and reads the flags back out of the compile database
  afterwards, because a configuration that built and ran proves the build works
  and not that the switch did anything. What is covered is that destination
  shape; the forms the chapter passes through on the way to it — the first
  single-executable build, the sanitizer flags before they move onto their own
  target — live in no file and stay unverified, which is the honest limit of
  "the snippets are under CI". A second CI job configures and builds the same
  file with MSVC on Windows, both ways, because the `if(MSVC)` branches had
  never been run by anything; the first run found that an instrumented binary
  will not start outside a Developer Command Prompt, which the file now says.
  Without cmake on PATH that step prints SKIPPED and
  the run stays green, the bargain `check_mermaid.sh` already makes with
  `mmdc`; CI passes the new `--require-cmake`, which refuses to skip, so the
  step can never quietly vanish there. The Greeter sources are untouched, and
  the chapter gains one sentence in *Try it* pointing at the reference — write
  yours first.
  - **Decided once: where chapter code lives.** Four Part VI chapters carry
    code CI does not build, and closing them one at a time would have
    re-litigated the same question four times. CONTRIBUTING.md now has a
    *Where chapter code lives* section (mirrored compressed in CLAUDE.md):
    code a chapter builds and runs goes under `exercises/<lab-name>/` as
    buildlab does, `solutions/` stays flat and stdlib-only, everything
    verifiable is wired into `build_all.sh`, a step needing a tool that may be
    missing locally SKIPS locally but never in CI, and deliberately broken
    demonstration programs stay book-only and unverified on purpose. ROADMAP
    item 1's "still open" paragraph is resolved and item 6's closing note
    records that Chapters 28-30 now apply a convention rather than choose one.
- **Changed: the book renders GitHub-native** (PATCH-level — formatting and
  six illustrations, no text rewritten and no number moved). The forty
  top-level callout blockquotes gained a `> [!TYPE]` line so GitHub draws
  them as alerts: 29 TIP (key principle, the stance to hold, Habit — the
  book's advice, which is what TIP means), 7 WARNING (Trap, Gotcha),
  2 IMPORTANT held back for the two non-negotiable rules (Chapter 5's rule
  to recite, Chapter 30's one rule), 2 NOTE (the big reveal, the surprise
  for C# devs). Type carries meaning only if it varies: a book whose every
  callout is IMPORTANT has spent the colour and bought nothing, so the
  common case is the one that reads as advice. The bold labels stay —
  Appendix B mirrors the key principles by name, and the labels keep the
  callouts greppable and meaningful in renderers with no alert support.
  Every callout in the book is top-level, so all forty converted; one
  nested inside a `<details>` fold would have been left as plain bold,
  since GitHub does not render alerts inside other elements.
  **Six mermaid diagrams** were added next to the prose they illustrate,
  never replacing it: the compilation pipeline in its two halves — the
  per-.cpp trip through preprocessor, translation unit and compiler, then
  the single link step — each labelled with the class of error it produces
  (Chapter 12, the pair the book most wanted), the "who owns this object?"
  decision and the cycle question that only the shared branch raises
  (Chapter 1), CMake's configure-then-build against MSBuild's one step
  (Chapter 26), and the destructor-versus-driver-thread teardown as a
  sequence diagram (Chapter 29). GitHub renders these natively and
  `build_book.sh` carries them into the single file unchanged.
  - **New: `scripts/check_markup.sh`**, run by the `book` job after the
    build, so it covers the single file as well as `book/`. It enforces the
    shape both features need on GitHub: an alert marker is one of the five
    known types, sits at column 1 outside any `<details>` fold, has a blank
    line before it and a blockquote body after it; a mermaid fence is
    unindented, preceded by a blank line, and likewise outside a fold. Every
    one of those mistakes renders wrong on GitHub while producing no build
    error, no broken link and a diff that looks fine, so nothing else in CI
    was ever going to catch them.
  - **New: `scripts/check_mermaid.sh`**, the other half — it hands every
    chapter containing a diagram to mermaid-cli and fails if one does not
    draw, so a diagram with a syntax error can no longer merge green and
    render as an error box for the reader. Rendering rather than parsing:
    a block can parse and still throw on the way to a picture. Pinned to
    the mermaid major version GitHub itself renders with. Locally the script
    reports SKIPPED without `mmdc` installed rather than passing; CI runs it
    with `--required`, which refuses to skip. What no check can tell you is
    whether the picture is any *good* — every layout bug fixed on this
    branch rendered perfectly well — so new diagrams still get looked at.
- **Changed: the book is now one file per chapter.** `book/going-unmanaged.md`
  (4,256 lines) became `book/01-ownership-and-raii.md` …
  `book/31-reading-what-the-tools-tell-you.md` plus `A-`…`D-<slug>.md` for the
  appendices, with `book/README.md` carrying the front matter and the
  Contents — GitHub renders it when you open `book/`. Each file ends with a
  generated prev/next/Contents footer. **Not one word of the book text
  changed**: the split was made at line boundaries and verified by
  concatenation. The Contents and the three in-text anchor links that now
  cross a file boundary became relative links that keep their original
  anchor, so citations by chapter number and by anchor both still work.
  - **New: `scripts/build_book.sh`** rebuilds the complete single file at
    `build/going-unmanaged.md`, rewriting those links back to in-page anchors
    and stripping the nav footers; `--write-nav` regenerates the footers from
    the file order and `--check-nav` fails if one is stale. Its output was
    verified byte-identical to the checked-in single file before that file
    was removed — from here the single file is a build artifact and a release
    download, not repository content.
  - **CI** gains a `book` job: build the single file, check the nav footers,
    and run lychee over every markdown file in the repo with `--offline
    --include-fragments`, so relative links and `#anchors` are blocking while
    external URLs are never fetched and cannot fail CI. A new Release
    workflow attaches the built single file to each `v*` tag.

- **New: Chapter 31 — Reading What the Tools Tell You.** Chapter 24's Day 2
  tells the reader to read sanitizer reports "until they make sense"; the
  book never showed one. This does: report *shapes* as a diagnostic index
  (three stacks / two / one / none), an annotated heap-use-after-free walked
  line by line, UBSan's report-and-exit-0 default and the two ways to make
  it fatal, how `-O2` deletes the frames you need, and watchpoints. Notes
  that LeakSanitizer is unsupported on macOS/arm64, so leak coverage there
  comes from CI — the finding that prompted the Finding 10 caveat below.
  Three key principles added to Appendix B; README's and CONTRIBUTING's lists
  record it, and ROADMAP item 5 is marked DONE. `scripts/check.sh` was
  verified against the UBSan behavior the chapter documents and is correct
  as written.
- **New: Chapter 30 — Authoring an ABI Boundary.** The other side of the
  Chapter 16 Bestiary, which teaches consuming vendor shapes but never
  shipping one. API vs ABI; the one rule (nothing whose layout your compiler
  chose may cross), with the corollaries about `std::string` in signatures,
  exceptions, and whoever-allocates-frees; the three techniques — PIMPL,
  pure-virtual interface, `extern "C"` façade — with a table for choosing;
  and versioning a published boundary. The C façade section closes the loop:
  the header the reader derives *is* FakeDevice's. Three key principles
  added to Appendix B; README's and CONTRIBUTING's lists record it, and
  ROADMAP item 6 is marked DONE — noting that this is the fourth Part VI
  chapter whose code CI does not build, which is now worth closing as one
  piece of work rather than four.
- **New: Chapter 29 — Concurrency.** Discharges the promises made in
  Chapters 16 and 18: the C#-to-C++ concurrency mapping (no runtime, no
  pool, no `await`), `std::thread`'s join-or-terminate obligation and
  `jthread`, ThreadSanitizer as the third sanitizer, locks as RAII, and the
  callback-from-a-driver-thread problem worked through to a correct
  teardown — weak reference, alive flag, unregister, and a callback context
  that is deliberately never freed. Centrepiece: a textbook data race that
  prints the right answer on every run at `-O2`, found by TSan in one. Three key
  principles added to Appendix B; Appendix D's Williams entry now points
  here first. **This closes Tier 1 of the roadmap** — README's and
  CONTRIBUTING's lists say so, and ROADMAP item 4 is marked DONE while
  recording that the threaded-callback lab it was meant to pair with is
  still open: the chapter's exercise has no task card under `exercises/`,
  and no threaded code reaches `build_all.sh`, so CI runs no TSan job.
- **New: Chapter 28 — Testing.** Why C++ test frameworks are made of macros
  (no reflection, no `[CallerLineNumber]` — only the preprocessor sees source
  text); a working framework in forty lines of standard library; testability
  as a *structural* property, since code reachable only from a .cpp with
  `main()` cannot be linked into a test binary; a Rule-of-Five suite for the
  Chapter 15 Buffer; the real frameworks and CTest; and why there are no
  runtime mocks — with FakeSDK and FakeDevice named as the pattern the book
  has been demonstrating all along. Centrepiece: a break that leaves every
  assertion passing while ASan reports a double-free. Three key principles
  added to Appendix B; README's and CONTRIBUTING's lists record it, and
  ROADMAP item 3 is marked DONE for the chapter while staying explicit that
  the suite is not yet in `build_all.sh` — and that wiring it up first needs
  the Chapter 15 Buffer extracted to a header, which is the chapter's own
  structural point applied to this repository.
- **New: Chapter 27 — Dependency Management.** Why C++ has no NuGet (a
  compiled binary is valid for exactly one compiler, standard library,
  configuration and architecture, so libraries ship as source); the four
  strategies — vendored, fetched-and-pinned, package manager, SDK-provided;
  why header-only libraries are disproportionately common; and the diamond
  problem, where two versions of one library in a binary is a *silent* ODR
  violation whose answer changes with link order. Three key principles added
  to Appendix B; README's contents list and ROADMAP item 2 (now marked DONE)
  record it.
- **New: Part VI — The Real Codebase, and Chapter 26 — Build Systems and
  CMake.** Why a build system exists (header-dependency tracking is the
  answer to Chapter 23's breakage 7), CMake as a *generator* rather than a
  build tool, a worked CMakeLists for the Chapter 23 Greeter trio, targets
  and PRIVATE/PUBLIC propagation, build types with the handbook's sanitizer
  flags behind an option, SDK config packages, and the pitfalls. Part VI is
  the home for future appended chapters. Three key principles added to
  Appendix B; Chapter 13 gains a forward pointer; the book's "how to use it"
  note, README's contents list, and ROADMAP item 1 (now marked DONE) all
  record the new part.
- **Correction (FakeSDK.h banner):** the header's banner comments still named
  a real vendor SDK, against the book's own no-real-vendor-names rule; Chapter
  17 already quoted the sanitized wording, so the disk file now matches the
  book's verbatim quote. In the same commit, Chapter 25's intro stopped calling
  itself "this appendix" — it is a chapter.
- **Correction (Chapter 25, Finding 10):** the Finding described ASan's leak
  detection running at normal program exit without noting that
  **LeakSanitizer is not supported on macOS/arm64** — where a leaking program
  reports nothing at all, so a clean run means nothing about leaks. Added the
  caveat and where Mac users should get leak coverage instead (CI/Linux). The
  Finding's own thesis is that a clean sanitizer run is not a correctness
  proof, so this is the same lesson in a new form rather than an exception
  to it.
- **Book:** the Contents now links to every part, chapter, and appendix;
  headings form a real hierarchy (parts H1, chapters H2, sections H3) so
  outline views work; reference solutions in exercise chapters are collapsed
  behind spoiler folds, making the "do it cold" rule structural; Part V opens
  with a pointer to the Chapter 24 practice plan.
- **Exercises:** every exercise now has a task card —
  `exercises/README.md` is the index (exercise ↔ chapter ↔ time ↔ solution);
  the Build-Model Lab ships its Greeter starting point;
  `words_sample.txt` moved to `exercises/words/`.
- **Tooling:** `scripts/check.sh` builds and runs a learner attempt under the
  canonical flags; `build_all.sh` also builds the buildlab scaffold.
- **Project:** [ROADMAP.md](ROADMAP.md) records the ranked list of content
  gaps — build systems, dependency management, testing, and concurrency
  first — with the evidence for each and a sketch of the contribution that
  would close it. Every item appends, so none of them moves a number.
  README.md and CONTRIBUTING.md point at it as a contribution category.

## [0.1.0] — 2026-07-31

Initial public import.

- The book: five parts, 25 chapters, appendices A–D, in one file
  (`book/going-unmanaged.md`).
- Ten exercises with reference solutions, including two vendor-style fake
  SDKs (FakeSDK: error codes + owned payloads; FakeDevice: opaque handles +
  C callbacks). All solutions build and run clean under
  `-Wall -Wextra -fsanitize=address,undefined`.
- CI (`scripts/build_all.sh` on every push and PR, Linux + macOS).
- Chapter 25 numbering fixed before anything was tagged: the log had two
  Finding 6s; "The Tracer as a permanent diagnostic tool" is now Finding 11.
  All existing cross-references were already consistent with the remaining
  numbers, so nothing else moved.
