# CLAUDE.md — project guide for AI-assisted work on Going Unmanaged

## What this project is

"Going Unmanaged — A Hands-On C++ Handbook for C# Developers." A curated,
exercise-driven handbook built by the maintainer (17y C# developer returning
to C++ for SDK work) together with an AI assistant. The canonical content is
the per-chapter files under `book/` — one file per chapter and appendix
(6 parts, 31 chapters, appendices A–D and F), indexed by `book/README.md`. The
single-file `going-unmanaged.md` is no longer checked in: it is a build
artifact produced by `scripts/build_book.sh`. Appendices run A–D plus F —
E waits for the glossary (ROADMAP item 10).
Part VI ("The Real Codebase") is the home for appended chapters about what a
project has that an exercise does not — build systems, dependencies, testing,
concurrency, authoring an ABI boundary, reading tool output. Chapter 29
discharges the threading promises made in Ch 16/18; Chapter 30 is the
authoring side of Ch 16's Bestiary (which only teaches consuming those
shapes); Chapter 31 supplies the sanitizer reports Ch 24's Day 2 tells the
reader to study but never shows.
README.md carries the origin story and contribution invitation; the book
itself stays free of meta-commentary.

NOTE (platform): LeakSanitizer is NOT supported on macOS/arm64 — a leaking
program under ASan reports nothing there, so a clean run says nothing about
leaks. Leak coverage comes from CI/Linux. Stated in Chapter 31 and in
Chapter 25's Finding 10.

## Layout

- `book/` — the book, canonical, one file per chapter and appendix:
  `NN-<slug>.md` for chapters 01–31, `A-`…`F-<slug>.md` for the appendices,
  E absent until the glossary lands (digits sort before letters, so the
  listing is the reading order)
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
  (files, strings, timing, handles, lookups, paths, async), each with a
  `main()` asserting what its recipes claim; build_all.sh builds and runs
  all seven. Same sync
  discipline as testlab: the recipe functions are quoted verbatim in the
  appendix, so editing one means editing `book/F-rosetta-cookbook.md` in the
  same commit (the mains are scaffolding and appear in no listing)
- `solutions/` — reference solutions for all exercises; plus `Buffer.h`, the
  Chapter 15 class extracted out of `buffer.cpp` so the testlab suite can
  include it (Chapter 28's structural point, applied)
- `scripts/build_all.sh` — builds AND runs every solution; the repo invariant.
  Its last two sections may skip: one configures, builds and runs
  `exercises/buildlab/`'s CMakeLists, the other rebuilds
  `solutions/device_threaded_solution.cpp` under `-fsanitize=thread` (a second
  build, because TSan and ASan do not combine). Without cmake on PATH, or
  without a ThreadSanitizer that can compile *and start* a trivial program,
  each prints SKIPPED and stays green; `--require-cmake` and `--require-tsan`
  (CI passes both) refuse to skip
- `scripts/check.sh` — builds/runs a learner's own attempt under the canonical
  flags (optional 2nd arg links fakesdk/fakedevice vendor code); `SAN=thread`
  switches the sanitizer for the threadlab's second build
- `scripts/check_markup.sh` — enforces the alert and mermaid-fence shapes
  below over `book/` and the built single file; run by CI, and worth running
  locally after touching either. Structure only, never mermaid grammar
- `scripts/check_mermaid.sh` — the other half: hands every chapter with a
  diagram to mermaid-cli and fails if one does not draw. Needs `mmdc`
  (`npm install -g @mermaid-js/mermaid-cli`); without it a local run says
  SKIPPED rather than passing, and CI's `--required` refuses to skip
- `scripts/check_platform_claims.sh` — runs the sanitizer demonstrations and
  asserts what the chapters promise *per platform*: ASan's exit code (134 on
  macOS, 1 on Linux), TSan's (134 / 66), whether LeakSanitizer reports at all
  (no on macOS/arm64), and whether a frame carries a column number (Ch 31's
  atos-vs-llvm-symbolizer point). CI runs it on ubuntu AND macos with
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
  on a vendor default rather than on the standard), and a
  `book` job: build_book.sh, --check-nav, check_markup.sh,
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
   quoted verbatim in the book. Changing them requires updating Chapter 17/18
   in the same commit — and is almost never the right move.
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
    book's advice, and by far the common case (29 of the 40).
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
(Chapter 32+, Appendix E+) — no item requires renumbering. Delivered items
stay on the list marked DONE so item numbers never shift. Short version:

- Tier 1 (load-bearing): CLOSED. Build systems/CMake was item 1 and is now
  Chapter 26; dependency management was item 2 and is now Chapter 27;
  testing was item 3 and is now Chapter 28; concurrency was item 4 and is
  now Chapter 29
- Tier 2: byte-level protocol work (item 7) leads the list, then
  consolidated const-correctness (item 8) and scenario chapters — tickets,
  not task cards (item 11; item 7 and the COM lab are candidate tickets).
  Authoring an ABI boundary was item 6 and is now Chapter 30; the debugging
  chapter was item 5 and is now Chapter 31
- Tier 3: C++/C# interop (P/Invoke), a glossary (Appendix E). The Rosetta
  Cookbook was item 12 and is now Appendix F — Recipes 1–8, then 9–13
  (files, paths, async); it grows by PR like the Findings log (Recipe
  template in CONTRIBUTING.md)
- Carried over: COM-style refcounting exercise (Bestiary Shape 3 has no lab).
  The threaded-callback lab was the other one and is DONE — `exercises/threadlab/`
  plus `solutions/device_threaded_solution.cpp` under a TSan CI gate
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
