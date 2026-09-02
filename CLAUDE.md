# CLAUDE.md — project guide for AI-assisted work on Going Unmanaged

## What this project is

"Going Unmanaged — A Hands-On C++ Handbook for C# Developers." A curated,
exercise-driven handbook built by the maintainer (17y C# developer returning
to C++ for SDK work) together with an AI assistant. The canonical content is
the per-chapter files under `book/` — one file per chapter and appendix
(6 parts, 39 chapters, appendices A–H), indexed by `book/README.md`. The
single-file `going-unmanaged.md` is no longer checked in: it is a build
artifact produced by `scripts/build_book.sh`. Appendices run A–H with no
gap — E is the glossary (item 10), G the bridge catalogue (item 16's
lookup half: the mechanism survey and decision table; no C++ listings —
check_verbatim.sh enforces that no cpp fence lands there), H the choosing
procedures (item 17: which container, how to take a parameter, what to
return, value-or-pointer inside a collection — the opposite contract to
G, its every cpp fence pinned to `exercises/choosing/`, which asserts the
costs the page quotes).
Part VI ("The Real Codebase") is the home for appended chapters about what a
project has that an exercise does not — build systems, dependencies, testing,
concurrency, authoring an ABI boundary, reading tool output. Chapter 29
discharges the threading promises made in Ch 16/18; Chapter 30 is the
authoring side of Ch 16's Bestiary (which only teaches consuming those
shapes); Chapter 31 supplies the sanitizer reports Ch 24's Day 2 tells the
reader to study but never shows. Chapters 32–37 are the ticket-shaped
scenario chapters (ROADMAP items 11, 14 and 15, all DONE): symptom first,
no concept named in advance, diagnosis behind a spoiler fold. Ch 33 adds the inversion the
job supplies — the sanitizer report arrives attached to the ticket, and the
diagnosis is made on paper from the report alone before anything is built.
Ch 34 closes item 7 inside the format (padding, endianness, the overlay
cast) with the opposite inversion: the canonical flags stay green on all
three of its bugs, so the attached capture hand-decoded against the ICD is
the only oracle. Ch 35 closes the carried-over Bestiary Shape 3 gap:
FakeSDK ships a refcounted 2.0, two opposite ownership bugs cancel on one
object, and the fix is a type (adopt/share wrapper), not a patch. Ch 36
(item 14) is the performance ticket, whose attached profile appears to
ACQUIT — support's percentages are correct and irrelevant, because the
crime is 33 allocator calls per tick on a deadline thread; the judge is a
replaced-operator-new allocation counter, not a timing. Ch 37 (item 15)
is the crash-dump ticket — nothing to run, a stripped customer crash
report, fault address 0x10 read as null+offsetof, and the guilty frame
inlined out of the stack; the acceptance is a two-configuration matrix.
Chapter 38 (item 16's chapter half) is the bridge-out chapter — the
main-thread queue Ch 29 promised, a frozen command registry, and the
StubHostAdapter seam; two of its three breaks are hangs no sanitizer
names, so bridgelab's judge is a bounded wait on every invoke. Appendix
G is that chapter's lookup half — the survey of bridge mechanisms, led
by the host's own channel, and the decision table. Chapter 39 (item 9) is
the publishing half of P/Invoke — one signature written twice in two
languages and compared by nothing; interoplab judges it with no .NET
anywhere, because marshal.h stands in for the marshaller the way FakeSDK
stands in for a vendor.
README.md carries the origin story and contribution invitation; the book
itself stays free of meta-commentary.

NOTE (platform): LeakSanitizer is NOT supported on macOS/arm64 — a leaking
program under ASan reports nothing there, so a clean run says nothing about
leaks. Leak coverage comes from CI/Linux. Stated in Chapter 31 and in
Chapter 25's Finding 10.

## Layout

- `book/` — the book, canonical, one file per chapter and appendix:
  `NN-<slug>.md` for chapters 01–39, `A-`…`H-<slug>.md` for the appendices
  (digits sort before letters, so the listing is the reading order)
- `book/README.md` — front matter and the Contents; GitHub renders it when
  someone opens `book/`, so it is the reader's entry point
- `exercises/` — one directory per exercise, each with a TASK.md task card;
  `exercises/README.md` is the index (exercise ↔ chapter ↔ solution)
- `exercises/fakesdk/`, `exercises/fakedevice/` — also carry vendor-style code
  users must NOT edit (contracts quoted verbatim in chapters 17/18)
- `exercises/buildlab/` — Greeter.h/.cpp + main.cpp, the Chapter 23 starting
  point; built by build_all.sh so the scaffold stays green. Also
  `CMakeLists.txt`, Chapter 26's reference build description assembled from
  that chapter's snippets — build_all.sh configures/builds/runs it twice
  (default, then Debug + `-DGREETER_SANITIZE=ON`)
- `exercises/deplab/` — Chapter 27's lab (its *Try it*, steps 1–4), and the
  only one whose subject is entirely build description: `mathlib/` is the
  dependency, and one `app/main.cpp` is consumed three ways — vendored
  (`add_subdirectory`), fetched (`FetchContent` + a `file://` URL) and found
  (`find_package(mathlib CONFIG)` against an installed prefix) — so the three
  `consume-*/CMakeLists.txt` are the whole lesson and the app cannot tell
  them apart. Nothing here is quoted in the chapter, so check_verbatim.sh has
  no pairing to hold. Four rules are load-bearing and easy to undo by
  accident. (1) mathlib's install/export half is wrapped in a top-level
  guard: paths 1 and 2 reach it through `add_subdirectory`, which would
  otherwise make those the *consumer's* install rules and publish a private
  vendored dependency's config package out of the app's prefix. (2) The
  exported include directory is
  `$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>`, never a literal
  `include` — the two must agree, or an install-dir override splits them
  and the consumer dies at the `#include`, past configure.
  (3) The fetched path builds at BOTH tags and each run must report the
  version its own tag carries: building once proves only that the mechanism
  runs, and asking merely that the two outputs *differ* passes a pin that
  chose the wrong commit. (4) `MATHLIB_TAG` is a cache variable, so the
  chapter's "re-point GIT_TAG" means `-DMATHLIB_TAG=`, not editing the
  default — the file says so, because a reader who edits it in place sees
  nothing change. Its git use needs a git that can clone `file://`, which is
  `--require-git`; the two `.cpp` files are also built and run under the
  canonical flags in the flat section, since the CMake paths apply none
- `exercises/testlab/` — Chapter 28's `tiny_test.h` and `buffer_test.cpp`,
  verbatim from the chapter's listings (same discipline as the Fake* vendor
  code: editing one means editing the chapter in the same commit), plus a
  TASK.md. build_all.sh builds the suite with `-I solutions` and runs it
- `exercises/abilab/` — Chapter 30's three worked boundaries, plus a TASK.md:
  `Widget.h`/`.cpp` (PIMPL), `IScorer.h` + `scorer.cpp` (interface + factory),
  `engine.h`/`.cpp` (`extern "C"` façade), each with a `*_demo.cpp` caller.
  Same sync discipline as testlab — the headers and the two full listings are
  the chapter's, so editing one means editing Chapter 30 in the same commit.
  build_all.sh builds each as a separate binary of TWO translation units
  (implementation + caller): the caller seeing only the boundary header is the
  subject matter, so never merge a demo into one TU with its implementation
- `exercises/threadlab/` — Chapter 29's lab: a TASK.md and nothing else. It is
  the Chapter 18 device with a driver thread the reader builds, so it links
  `../fakedevice/`'s vendor code rather than copying it; the reference solution
  is `solutions/device_threaded_solution.cpp`
- `exercises/cookbook/` — Appendix F's recipe listings, one TU per domain
  (files, strings, timing, handles, lookups, paths, async, events, logging),
  each with a `main()` asserting what its recipes claim; build_all.sh builds
  and runs all nine. Same sync
  discipline as testlab: the recipe functions are quoted verbatim in the
  appendix, so editing one means editing `book/F-rosetta-cookbook.md` in the
  same commit (the mains are scaffolding and appear in no listing)
- `exercises/choosing/` — Appendix H's measurements, the other non-exercise
  appendix directory: `counted.h` (a copy/move-counting type plus the
  `CHECK` judge), `passing.cpp` (procedures 2–3) and `storing.cpp`
  (procedures 1 and 4), no TASK.md. Its banners name exactly which units
  Appendix H quotes, and check_verbatim.sh holds the pairing BOTH ways —
  editing a named unit means editing `book/H-choosing.md` in the same
  commit. Two rules are load-bearing and easy to undo by accident: the
  judge is `CHECK` (counts failures, sets the exit code), never `assert`,
  which a Release build compiles away; and build_all.sh builds
  `passing.cpp` a SECOND time under `-fno-elide-constructors`, because
  with NRVO on a returned temporary and a returned named local both
  measure zero and the page's guaranteed-vs-permitted distinction is
  never exercised
- `exercises/exitlab/` — Chapter 32's ticket lab. TASK.md carries the broken
  2.4.1 listings (book-only, they exist to fail); the committed files are
  the FIXED state, quoted verbatim in the chapter's fix section, and
  build_all.sh builds them TWICE with the translation units in opposite
  orders and runs both — order-independence is the fix's claim, and one
  build cannot prove it
- `exercises/reportlab/` — Chapter 33's ticket lab. TASK.md carries the
  attached sanitizer report and the broken 2.6.0 main (book-only, it exists
  to fail); the committed files are the FIXED state, quoted verbatim in the
  chapter's fix section, and build_all.sh runs the binary at 0 hot-plugs
  AND at 100 — growth-independence is the fix's claim, and one count
  cannot prove it
- `exercises/capturelab/` — Chapter 34's ticket lab. TASK.md carries the
  attached capture, the ICD table and the broken overlay parser (book-only,
  it exists to fail); the committed files are the FIXED state, quoted
  verbatim in the chapter's fix section, and build_all.sh asserts the
  decode against the chapter's hand-decoded values — the sanitizers are
  silent on this chapter's bug class, and the capture's second frame is
  deliberately unaligned
- `exercises/comlab/` — Chapter 35's ticket lab. FakeSDK2.h/.cpp is NEW
  vendor code (the refcounted 2.0 of Chapter 17's SDK — a separate drop,
  fakesdk/ unchanged; contract quoted verbatim in Ch 35, same rules as the
  other Fake* files); TASK.md carries the broken 2.0 port (book-only, it
  exists to fail); ref.h + main.cpp are the FIXED state, quoted verbatim
  in the chapter's fix section, and build_all.sh holds them to two judges
  at once — the binary asserts the vendor's live-object counter reaches 0
  after shutdown (catches a release too few), the sanitizers catch a
  release too many. check.sh links it via the `comlab` argument
- `exercises/perflab/` — Chapter 36's ticket lab. TASK.md carries the
  broken 2.1.0 Tick, the attached profile and the host's engine log (the
  broken shape is book-only, it exists to fail); meter.h/.cpp + main.cpp
  are the FIXED state, quoted verbatim in the chapter, and build_all.sh
  runs the harness at 50 AND 1000 ticks — the harness replaces operator
  new and asserts ZERO heap allocations, because the sanitizers are
  silent on an accidental copy and a timing assert would measure the
  runner instead of the code
- `exercises/dumplab/` — Chapter 37's ticket lab. TASK.md carries the
  broken 3.4.0 session.cpp and the attached customer crash report (the
  broken shape is book-only, it exists to fail — at -O2, so the reader
  can hold a post-mortem on the corpse); session.h/.cpp + main.cpp are
  the FIXED state, quoted verbatim in the chapter, and build_all.sh runs
  both device configurations — the crash lived only in the one the bench
  never had
- `exercises/bridgelab/` — Chapter 38's lab. TASK.md carries the three
  broken shapes (book-and-card, identical by rule — they exist to fail);
  the committed headers + main.cpp are the FIXED state, quoted in the
  chapter BY EXCERPT (check_verbatim runs both directions for this lab:
  card fences must be in the chapter, chapter fences must be in the lab),
  and build_all.sh builds it twice — canonical flags, then a second
  source in the probe-gated TSan section — because the three breaks
  split across the two builds. The harness allows no unbounded wait:
  every invoke takes a deadline, the lab's judge, since a hang would
  stop CI rather than fail it
- `exercises/interoplab/` — Chapter 39's lab, and the only one whose caller
  is imaginary: `plugin.h`/`plugin.cpp` are the published boundary and
  `main.cpp` plays the .NET marshaller through that header alone (two TUs,
  like abilab — a caller that can see the implementation is not a
  boundary). `marshal.h` models what the marshaller does to a struct, a
  string and a delegate; it deliberately does NOT model the collector,
  because a collected delegate is a use-after-free and broken programs
  stay book-only — the sink carries an `alive` flag so the header's
  documented callback window is asserted instead. The struct-misdeclaration
  judge is the load-bearing one: delete the size-field check and it is an
  ASan stack-buffer-overflow, not a wrong value
- `solutions/` — reference solutions for all exercises; plus `Buffer.h`, the
  Chapter 15 class extracted out of `buffer.cpp` so the testlab suite can
  include it (Chapter 28's structural point, applied)
- `scripts/build_all.sh` — builds AND runs every solution; the repo invariant.
  Its last three sections may skip: one builds `exercises/deplab/` three ways
  (Chapter 27), one configures, builds and runs `exercises/buildlab/`'s
  CMakeLists, the last rebuilds `solutions/device_threaded_solution.cpp` under
  `-fsanitize=thread` (a second build, because TSan and ASan do not combine).
  Without cmake on PATH, without a git that can clone a `file://` repository
  (deplab's FetchContent path only), or without a ThreadSanitizer that can
  compile *and start* a trivial program, each prints SKIPPED and stays green;
  `--require-cmake`, `--require-git` and `--require-tsan` (CI passes all three)
  refuse to skip
- `scripts/check.sh` — builds/runs a learner's own attempt under the canonical
  flags: one or more .cpp files, compiled in the order written (= link order,
  which Chapter 32's two-order test turns on), then an optional vendor
  argument (fakesdk/fakedevice/comlab), then run args; `SAN=thread` switches
  the sanitizer for the threadlab's second build, `SAN=none` removes it
  entirely and `OPT` (default 0) sets `-O`. Those last two are for the
  exercises whose subject is what the tools do NOT catch — Chapter 27's ODR
  diamond, whose step 5 cannot be shown by a build that warns — and the
  script drops its "sanitizers quiet" claim under `SAN=none`, because an
  exit 0 from an uninstrumented build is not evidence. An EMPTY `SAN` still
  gets the default: turning the sanitizers off is a thing you say, not a
  thing that happens to you
- `scripts/check.ps1` — check.sh's Windows/MSVC mirror (`cl /std:c++17 /W4
  /EHsc /fsanitize=address`, same source-list/vendor/run-args shapes; MSVC
  has no UBSan and no TSan, and the script says so). Smoke-tested by the
  buildlab-msvc CI job so it cannot rot on a Mac-based maintainer
- `scripts/check_verbatim.sh` — enforces every book↔code verbatim pairing
  mechanically: full-file containment (vendor headers, ticket-lab fixed
  files, solution folds), banner-stripped containment for testlab/abilab
  (the convention: a committed lab file may open with a `//` provenance
  banner the chapter listing omits; the verbatim contract covers everything
  below it), every Appendix F cpp fence in a cookbook TU, and the seven
  ticket/lab TASK cards' broken listings in their chapters — plus two
  reverse directions: every cpp fence in Chapter 38 must live in
  `exercises/bridgelab/`, and Appendix H is held to `exercises/choosing/`
  both ways (every fence on the page is in that directory, and every unit
  the lab's banners name is on the page whole — that lab has no TASK card
  to carry the reverse the way bridgelab's does) — plus one pairing whose
  code half is not a file under `exercises/` at all: Chapter 27's two ODR
  headers are an ill-formed program that no harness may commit, so
  `check_platform_claims.sh` generates them into a temp directory and
  asserts the chapter's claims about them, and the page is held to that
  script's heredocs. Run it after touching any quoted listing; adding a new
  quoted pairing means adding it to this script in the same commit. CI runs
  it in the book job
- `scripts/check_markup.sh` — enforces the alert and mermaid-fence shapes
  below over `book/` and the built single file, plus one typographic rule:
  no two `---` rules with only blank lines between them, which GitHub draws
  as two dividers with a gap rather than the single separator the source
  looks like, and which seventeen files had acquired invisibly. Run by CI,
  and worth running locally after touching either. Structure only, never
  mermaid grammar
- `scripts/check_mermaid.sh` — the other half: hands every chapter with a
  diagram to mermaid-cli and fails if one does not draw. Needs `mmdc`
  (`npm install -g @mermaid-js/mermaid-cli`); without it a local run says
  SKIPPED rather than passing, and CI's `--required` refuses to skip
- `scripts/check_platform_claims.sh` — runs the sanitizer demonstrations and
  asserts what the chapters promise *per platform*: ASan's exit code (134 on
  macOS, 1 on Linux), TSan's (134 / 66), whether LeakSanitizer reports at all
  (no on macOS/arm64), and whether a frame carries a column number (Ch 31's
  atos-vs-llvm-symbolizer point). It also holds Chapter 27's ODR diamond —
  both link orders link silently, the two orders disagree, and exactly one
  is caught, naming `GetTimeout` — the one section whose first two claims
  are about the linker rather than a compiler-rt runtime and so hold on
  every platform alike; its two headers are the chapter's own listings,
  pinned to the page by check_verbatim.sh. CI runs it on ubuntu AND macos with
  `--required`, because the platform overclaims it exists to catch are exactly
  what a one-platform check cannot see. The broken programs are generated into
  a temp dir, never committed — `solutions/` stays clean
- `scripts/build_book.sh` — concatenates `book/` back into the single-file
  book at `build/going-unmanaged.md` (gitignored); `--write-nav` regenerates
  the nav footers, `--check-nav` fails if one is stale
- `.github/workflows/ci.yml` — runs build_all.sh on every push/PR, plus a
  `platform-claims` job (check_platform_claims.sh on ubuntu and macos), a
  `buildlab-msvc` job (Chapter 26's CMakeLists under MSVC both ways, then
  Chapter 14's `/Zc:nrvo` claim: the chapter's own `cl /std:c++17 /W4 /EHsc`
  line must print exactly one extra move-construction in the RVO section, and
  adding `/Zc:nrvo` must print none — it is the one claim in the book resting
  on a vendor default rather than on the standard — then a check.ps1 smoke
  test: one plain build, one through the fakesdk vendor path, and one in the
  ticket labs' multi-TU form, two reportlab sources plus a run arg), and a
  `book` job: build_book.sh, --check-nav, check_markup.sh, check_verbatim.sh,
  check_mermaid.sh (which installs mermaid-cli, the job's one slow step),
  and a lychee link check
  (`--offline --include-fragments`: relative links and anchors are blocking,
  external URLs are not checked). The check covers the built single file too,
  on purpose — that is what catches a cross-file link the build does not
  rewrite into an in-page anchor
- `.github/workflows/release.yml` — on a `v*` tag, builds the single file and
  attaches it to the GitHub release

## Where chapter code lives (decided once; full text in CONTRIBUTING.md)

Part VI chapters carry code that is not an exercise solution. It goes under
`exercises/<lab-name>/`, next to that lab's task card, as buildlab and testlab
do; `solutions/` stays flat, stdlib-only `.cpp` files — with one amendment: a
header is permitted there exactly when a chapter requires the demo/test split
(Ch 28 forced the first, `solutions/Buffer.h`, because a class sharing a TU
with `main()` cannot be tested; duplicating it into the lab was rejected).
Everything verifiable is wired into `build_all.sh` (still the one invariant,
still ALL GREEN). A step needing a tool that may be missing locally (cmake now,
TSan later) copies check_mermaid.sh: SKIPPED locally, plus a `--require-<tool>`
flag CI passes so it can never skip there (`--require-cmake`, `--require-tsan`;
check_mermaid.sh's own predates the pattern and is just `--required`) — and the
probe does the thing rather than looking for the tool, since TSan can be
installed and still fail to start. A lab that revisits an SDK the repo already
has links that vendor code in place (threadlab). Deliberately broken
demonstration programs — Ch 31's sabotage runs, Ch 30's break-it-first steps,
Ch 29's Try it step 4 — stay book-only and unverified on purpose (ROADMAP item
5). Ch 26, 28, 29 and 30 are all done under this convention: the four-chapter
Part VI code debt is closed, and a future Part VI chapter reuses it.

## Hard invariants (never break these)

1. Every file in `solutions/` compiles clean and runs clean under
   `-std=c++17 -Wall -Wextra -fsanitize=address,undefined` (words.cpp and
   invalid.cpp use `-std=c++20`). Run `./scripts/build_all.sh` after ANY
   change to code; it must print ALL GREEN.
2. `exercises/*/Fake*.h|.cpp` are "vendor code": their public contracts are
   quoted verbatim in the book. Changing them requires updating Chapter
   17/18 (or 35, for comlab's FakeSDK2) in the same commit — and is almost
   never the right move.
3. Chapter numbering is load-bearing: the book cross-references chapters by
   number ("Chapter 6", "Finding 3 of Chapter 25"). Inserting a chapter means
   renumbering ALL later chapters AND every in-text reference, including
   inside code comments. Prefer appending; grep before and after:
   `grep -n "Chapter [0-9]" book/*.md`
4. No real vendor/product names in the book's SDK material (the point is
   generality). Open-source ecosystems named as study material are fine
   (libusb, PortAudio, SQLite, Qt, Unreal, STM32 HAL, COM as a technology).
5. Solutions never use anything beyond the standard library.
6. The single file stays reproducible from `book/`: after ANY change there
   run `./scripts/build_book.sh`, and `--write-nav` too if you added,
   removed, or renamed a chapter file. CI runs both.

## Content conventions

- New material (chapters, exercises, appendix sections) is reviewed against
  the twelve questions in CONTRIBUTING.md, "The questions every piece of
  material answers" (decided once; full text there): moment of need, which
  C# reflex it confronts, what wrong-that-looks-like-working looks like,
  the feedback loop that tells the reader they are wrong, the one-sentence
  principle for Appendix B, findability from a symptom, and what re-verifies
  each claim. Answers are mechanisms in the material, never intentions;
  corrections need only the last question. ROADMAP.md's "What earns a place
  here" section is the same list applied to whether an item belongs at all.
- Voice: honest, practical, first-person-curator; C# comparisons are the
  pedagogical spine ("in C# this would..."). British-neutral English.
- Every exercise chapter follows: *trains / vendor code (if any) / the task /
  reference solution / pitfalls / stretch goals*.
- Findings (Chapter 25) follow strictly: **Found in / The theory /
  broken-vs-fixed code / Habit**. Community findings via PR keep this shape.
- Recipes (Appendix F) follow strictly: **In C# / The recipe / Why it looks
  like this / Trap** — the trap a one-line `[!WARNING]`, the why
  cross-references to the owning chapter, numbers append-only like Findings.
- Key-principle quotes are in speakable first person ("I check every error
  code...") — they double as a cheat sheet (Appendix B mirrors them; keep
  the two in sync when adding one).
- Code style in the book: 4 spaces, `name_` members, comments explain WHY.
- Book heading scheme: H1 = parts (and the title/Appendices separators),
  H2 = chapters and appendices, H3 = sections. Unchanged by the split: a
  part's H1 (with its intro prose) lives at the top of that part's FIRST
  chapter file, and the `# Appendices` H1 at the top of
  `A-fundamentals-refresher.md`.
- One `---` between sections, never two. A second thematic break with only
  blank lines before it draws as a second divider with a gap, not a heavier
  one — legible enough that seventeen files carried one unnoticed.
  `check_markup.sh` enforces it; only `---` is checked, since that is the
  only spelling the book uses.
- Adding a chapter = a new `NN-<slug>.md` file plus its entry in
  `book/README.md`'s Contents. Links between files keep the GitHub anchor as
  a suffix — `](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake)`
  — because `build_book.sh` rewrites `](<file>.md#<anchor>)` back to
  `](#<anchor>)` for the single-file build. Same-file links stay `](#anchor)`.
- Every chapter file ends with a generated nav footer between
  `<!-- nav:begin -->` and `<!-- nav:end -->`. Never hand-edit one: run
  `scripts/build_book.sh --write-nav`. The single-file build strips them.
- Reference solutions in exercise chapters sit inside `<details>` spoiler
  folds ("Show the solution — do the exercise cold first"); keep that shape
  for new exercise chapters.
- Callouts are a `> [!TYPE]` marker line followed by a one-line blockquote
  body that opens with the bold label. GitHub draws them as alerts; the
  marker line is stripped of meaning everywhere else, which is why the label
  carries the weight. Type follows from the label, not from how strongly you
  feel about the sentence:
  - `[!TIP]` — **Key principle:**, **The stance to hold:**, **Habit:**. The
    book's advice, and by far the common case (36 of the 72 at last count).
  - `[!WARNING]` — **Trap:**, **Gotcha:**. Something that compiles, runs,
    and is wrong.
  - `[!IMPORTANT]` — the two non-negotiable rules only: Chapter 5's virtual
    destructor, Chapter 30's one rule. **Keep it rare.** The first pass
    typed 31 of 40 IMPORTANT and the colour stopped saying anything — three
    identical purple boxes closed Chapter 26 and told the reader nothing.
    A new callout is IMPORTANT only if breaking it is a bug, not a smell.
  - `[!NOTE]` — the C#-developer surprise: **The big reveal:**,
    **Surprise for C# devs:**.
  - **The bold label stays.** Appendix B mirrors the key principles by name,
    the labels keep the callouts greppable, and they are the whole callout
    in any renderer without alert support.
  - **Top-level only.** GitHub does not render an alert nested inside a list
    or a `<details>` fold. A callout that must live there stays plain bold
    with no marker — there are none today. Blank line before the marker.
- Diagrams are mermaid in a ```` ```mermaid ```` fence, rendered natively by
  GitHub and carried into the single file untouched. The rules:
  - **A diagram is additive.** It illustrates prose that already stands on
    its own — nothing is deleted or rewritten to make room, because mermaid
    does not render outside GitHub (the release single file included). At
    most one lead-in sentence.
  - **Basic `flowchart` and `sequenceDiagram` only**, no `style`/`classDef`
    and no hardcoded colours — GitHub themes mermaid for light and dark
    itself, and a hardcoded colour is unreadable in one of them.
  - **Fence unindented, blank line before it**, and never inside a
    `<details>` fold or a list.
  - **Look at it before committing; a green CI is not enough.** CI now draws
    every diagram (`check_mermaid.sh`), so a broken one cannot merge — but
    that only proves it renders, and every layout bug below rendered fine.
    Nothing automatable can tell you the picture is any good. Extract the
    block from the committed file, render it
    in a browser (`mermaid.render`, then `getBBox()`), and look at it:
    layout bugs are only visible drawn. The ones already paid for — a
    decision tree stacked into a 1000px column by `flowchart TD` when it
    wanted `LR`; crossed edges from declaring nodes in the wrong order
    (dagre places by declaration, so declare the node the reader starts
    from first); `subgraph`s rendering in reverse of declaration order; an
    `alt` with one branch reading as the whole story.
  - **Watch the width, not just the height.** GitHub scales a diagram to the
    container width, so a wider picture is smaller *text* for every reader.
    Prefer splitting a diagram at a seam the chapter already teaches over
    letting it grow.

## Known gaps / roadmap (good first tasks)

`ROADMAP.md` is the full ranked list of missing content, with evidence and a
sketch of what each contribution looks like. Everything on it APPENDS
(Chapter 40+, Appendix I+) — no item requires renumbering. Delivered items
stay on the list marked DONE so item numbers never shift. Short version:

- Tier 1 (load-bearing): CLOSED. Build systems/CMake was item 1 and is now
  Chapter 26; dependency management was item 2 and is now Chapter 27;
  testing was item 3 and is now Chapter 28; concurrency was item 4 and is
  now Chapter 29
- Tier 2: three items open — consolidated const-correctness (item 8), the
  framework shape (item 18: Bestiary Shape 5 is named in Chapter 16 and
  taught nowhere — two readers of the 2026-09 study stopped there, and
  Shape 4 has no lab either — though only its interrupt-context half is
  genuinely untaught, which is item 21 — so this item is about the shape
  with no treatment at all rather than the last shape without a lab; still
  a legitimate candidate for out-of-scope-with-a-sentence), and below
  the mutex (item 19: Chapter 29 and Chapter 36 between them state the
  deadline path's prohibition and never its alternative — their two opposite
  defaults for a foreign thread now cross-reference each other, which is all
  of the item that is done). All three were sequenced after item 9 (P/Invoke),
  which is **DONE as of 2026-09-02** — Chapter 39 — so all three are now
  unblocked and item 8 is the next in this tier. It also builds on item 17 —
  *Choosing* — which is now DONE as Appendix H plus the copy/move-counting
  `exercises/choosing/`: `const&` is one branch of that appendix's
  parameter procedure, so item 8 points at the procedure rather than
  re-deriving it. Scenario chapters were item 11 and are DONE — Chapters 32–35, then
  items 14 and 15 appended the performance ticket (Chapter 36 + perflab)
  and the crash-dump ticket (Chapter 37 + dumplab) in the same format,
  which stays open to new tickets by PR. Byte-level protocol work was
  item 7 and is now Chapter 34; authoring an ABI boundary was item 6 and
  is now Chapter 30; the debugging chapter was item 5 and is now
  Chapter 31
- Tier 3: C++/C# interop was item 9 and is now Chapter 39 + `exercises/
  interoplab/` — the publishing half of P/Invoke, judged with no .NET
  anywhere because `marshal.h` stands in for the marshaller the way
  FakeSDK stands in for a vendor. Item 9's scope note stays OPEN: a full
  treatment of being loaded BY a runtime (JNI, a Python extension, a Node
  addon) is the shape two readers actually wanted, and Chapter 39 only
  names it. SOLID without the runtime (item 13 — the
  reader's design vocabulary, un-fused from the .NET machinery; a
  gather-and-translate chapter like item 8), the retrofit (item 20, also
  after item 9 — no lab modernises working code whose callers must keep
  compiling: they start blank, or from buildlab's working trio, or from a
  ticket lab's already-broken code. The reader who is handed the native
  layer *because* it is old has no chapter, and the acceptance test is that
  the caller's TU is byte-identical before and after), the interrupt-context
  callback (item 21 — Bestiary Shape 4's second addition to Shape 1; its
  first, initialization order, is Chapter 32's whole subject and is done.
  The smallest item on the list, the only one filed with no reader
  evidence, and probably a section of item 19 rather than its own material:
  an ISR is item 19's deadline path with a harder deadline. Sequenced after
  19, and closable by it), and the bridge out
  was item 16 and is now DONE — Chapter 38 + stdlib-only
  `exercises/bridgelab/` (the main-thread queue under a bounded-wait
  judge), plus Appendix G, the survey of mechanisms and its decision
  table.
  The glossary was item 10 and is now Appendix E (letters run A–H with no
  gap). The Rosetta Cookbook was item 12 and is now Appendix F — Recipes
  1–8, then 9–13 (files, paths, async), then 14–16 (events, logging,
  timers), then 17 (UTF-8↔UTF-16); it grows by PR like the Findings log
  (Recipe template in CONTRIBUTING.md)
- Carried over: both DONE. The COM-style refcounting exercise (Bestiary
  Shape 3's missing lab) is Chapter 35 + `exercises/comlab/`; the
  threaded-callback lab is `exercises/threadlab/` plus
  `solutions/device_threaded_solution.cpp` under a TSan CI gate
- Deliberately out of scope (the section exists so the same suggestion does
  not arrive twice — read it before filing anything): **classroom
  scaffolding** — slides, rubrics, per-chapter lecture timings, a separated
  answer key — because it is a different product, not a missing chapter, and
  the CC-BY/MIT split already lets a trainer build it; and **general
  lock-free data-structure design**, the half of item 19 that is not ours.
  Closing a *numbered* item this way is a decision that belongs in an issue
  first; an entry that was never an item is recorded there directly
- Structural item: splitting the book per-chapter under `book/` with a build
  script that concatenates — DONE, see the ROADMAP entry

## Versioning

Semver-ish with chapter/Finding numbering as the public contract:
renumbering = MAJOR, appended content = MINOR, corrections = PATCH.
Releases are annotated git tags + a CHANGELOG.md entry. Numbering freezes
at v1.0. Full policy in CONTRIBUTING.md.

## Licensing

Dual, and the boundary runs *through* the chapter files: prose under `book/`
(and the single file built from it) is CC-BY 4.0; all code is MIT — `exercises/`,
`solutions/`, `scripts/`, `.github/`, and every code sample inside a chapter, so
a reader can paste a snippet without an attribution obligation. `LICENSE` is the
**unmodified** MIT text and must stay that way: GitHub's detector scores the
whole file against canonical MIT, and a preamble in it cost the repo its
detected license once already (it reported `NOASSERTION` until the preamble came
back out). The split lives in `NOTICE` instead. `LICENSE-CC-BY-4.0` is the
verbatim CC-BY legal text — never reword it either. `book/README.md`'s front
matter carries a license line because the release single file travels
without the repo. Contributed material lands
under the same split (CONTRIBUTING.md), so relicensing later would need every
contributor's consent.

## Working with the maintainer

The maintainer also uses the exercises for personal training. When asked for
help with an exercise ATTEMPT (as opposed to repo maintenance), default to
REVIEW mode: critique their code against the chapter's pitfalls; don't write
the solution for them unless they explicitly ask.
