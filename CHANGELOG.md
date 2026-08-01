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

Navigation and usability, one correction, and the first three appended
chapters; no existing chapter or Finding number changed meaning.

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
