# Changelog

Notable changes to the handbook, newest first.

**How versions work here:** the book's chapter and Finding numbers are the
public contract — people cite them, so they version like an API.
**MAJOR** = renumbering or restructuring (existing numbers change meaning);
**MINOR** = appended content (new chapters, Findings, exercises);
**PATCH** = corrections that move no numbers. Details in
[CONTRIBUTING.md](CONTRIBUTING.md). Numbering freezes at v1.0 — until then,
numbers may still move.

## [0.2.0] — Unreleased

Navigation and usability, one correction, the first six appended chapters,
the split of the book into per-chapter files, and GitHub-native rendering;
no existing chapter or Finding number changed meaning. That last point is
what makes this MINOR rather than MAJOR: the version contract is about what
a *number* refers to, and nothing renumbered — only the file the text lives
in changed.

- **Changed: the book renders GitHub-native** (PATCH-level — formatting and
  six illustrations, no text rewritten and no number moved). The forty
  top-level callout blockquotes gained a `> [!TYPE]` line so GitHub draws
  them as alerts: 7 WARNING (Trap, Gotcha), 31 IMPORTANT (Key principle,
  Rule to recite, the stance to hold, Habit, and Chapter 30's one rule),
  2 NOTE (the big reveal, the surprise for C# devs). The bold labels stay —
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
  verified against the UBSan behaviour the chapter documents and is correct
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
