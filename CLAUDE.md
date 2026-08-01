# CLAUDE.md — project guide for AI-assisted work on Going Unmanaged

## What this project is

"Going Unmanaged — A Hands-On C++ Handbook for C# Developers." A curated,
exercise-driven handbook built by the maintainer (17y C# developer returning
to C++ for SDK work) together with an AI assistant. The canonical content is
the per-chapter files under `book/` — one file per chapter and appendix
(6 parts, 31 chapters, appendices A–D), indexed by `book/README.md`. The
single-file `going-unmanaged.md` is no longer checked in: it is a build
artifact produced by `scripts/build_book.sh`.
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
  `NN-<slug>.md` for chapters 01–31, `A-`…`D-<slug>.md` for the appendices
  (digits sort before letters, so the listing is the reading order)
- `book/README.md` — front matter and the Contents; GitHub renders it when
  someone opens `book/`, so it is the reader's entry point
- `exercises/` — one directory per exercise, each with a TASK.md task card;
  `exercises/README.md` is the index (exercise ↔ chapter ↔ solution)
- `exercises/fakesdk/`, `exercises/fakedevice/` — also carry vendor-style code
  users must NOT edit (contracts quoted verbatim in chapters 17/18)
- `exercises/buildlab/` — Greeter.h/.cpp + main.cpp, the Chapter 23 starting
  point; built by build_all.sh so the scaffold stays green
- `solutions/` — reference solutions for all exercises
- `scripts/build_all.sh` — builds AND runs every solution; the repo invariant
- `scripts/check.sh` — builds/runs a learner's own attempt under the canonical
  flags (optional 2nd arg links fakesdk/fakedevice vendor code)
- `scripts/build_book.sh` — concatenates `book/` back into the single-file
  book at `build/going-unmanaged.md` (gitignored); `--write-nav` regenerates
  the nav footers, `--check-nav` fails if one is stale
- `.github/workflows/ci.yml` — runs build_all.sh on every push/PR, plus a
  `book` job: build_book.sh, --check-nav, and a lychee link check
  (`--offline --include-fragments`: relative links and anchors are blocking,
  external URLs are not checked). The check covers the built single file too,
  on purpose — that is what catches a cross-file link the build does not
  rewrite into an in-page anchor
- `.github/workflows/release.yml` — on a `v*` tag, builds the single file and
  attaches it to the GitHub release

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

- Voice: honest, practical, first-person-curator; C# comparisons are the
  pedagogical spine ("in C# this would..."). British-neutral English.
- Every exercise chapter follows: *trains / vendor code (if any) / the task /
  reference solution / pitfalls / stretch goals*.
- Findings (Chapter 25) follow strictly: **Found in / The theory /
  broken-vs-fixed code / Habit**. Community findings via PR keep this shape.
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
  - **Render it before committing; parsing is not enough.** `mermaid.parse()`
    proves the grammar is legal and tells you nothing about the picture. CI
    cannot check this. Extract the block from the committed file, render it
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
  consolidated const-correctness (item 8). Authoring an ABI boundary was
  item 6 and is now Chapter 30; the debugging chapter was item 5 and is now
  Chapter 31
- Tier 3: C++/C# interop (P/Invoke), a glossary (Appendix E)
- Carried over: COM-style refcounting exercise (Bestiary Shape 3 has no lab);
  threaded-callback lab (FakeDevice stretch goal promoted to an exercise)
- Structural item: splitting the book per-chapter under `book/` with a build
  script that concatenates — DONE, see the ROADMAP entry

## Versioning

Semver-ish with chapter/Finding numbering as the public contract:
renumbering = MAJOR, appended content = MINOR, corrections = PATCH.
Releases are annotated git tags + a CHANGELOG.md entry. Numbering freezes
at v1.0. Full policy in CONTRIBUTING.md.

## Working with the maintainer

The maintainer also uses the exercises for personal training. When asked for
help with an exercise ATTEMPT (as opposed to repo maintenance), default to
REVIEW mode: critique their code against the chapter's pitfalls; don't write
the solution for them unless they explicitly ask.
