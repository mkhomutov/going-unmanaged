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

Navigation and usability; no chapter or Finding numbers moved.

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
