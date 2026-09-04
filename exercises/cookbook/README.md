# cookbook/ — Appendix F's listings, compiled

This directory is not an exercise: there is no task card and nothing to
attempt cold. It holds the recipe listings of Appendix F (*The Rosetta
Cookbook*), one translation unit per domain —

| File | Recipes |
|---|---|
| `files.cpp` | 1, 9 — read and write a whole file |
| `strings.cpp` | 2–5, 23 — split, join, build, format; the empty string that is not null |
| `timing.cpp` | 6, 16 — time a call; a repeating timer |
| `handles.cpp` | 7 — wrap a C handle so it frees itself |
| `lookups.cpp` | 8, 18 — look up a key without inserting it; find an element, an index, or a substring |
| `paths.cpp` | 10–12 — combine, the exists pair, listing |
| `async.cpp` | 13 — run work on another thread and wait for it |
| `events.cpp` | 14 — expose an event |
| `logging.cpp` | 15, 24 — print a diagnostic you will actually see; compile one out of Release (built twice, once with `-DNDEBUG`) |
| `alternatives.cpp` | 19–20 — a value that may be absent; a value that is one of several kinds |
| `errors.cpp` | 21–22 — an exception type of your own; a value or an error, on C++17 |
| `expected.cpp` | Chapter 8's chaining listing — `std::expected`, the one C++23 TU, built as its own probe |
| `json.cpp` | 25–26 — serialize a record, read a config; the one TU with a dependency, `exercises/third_party/nlohmann/`, included with `-isystem` |
| `containers.cpp` | 27 — pre-size a collection: `reserve` against `vector(n)` and `resize` |

— each with a `main()` that asserts what its recipes claim.
`scripts/build_all.sh` builds and runs all of them on every push, so a recipe
that stops being true stops being green — all but one under the canonical
flags, which pin C++17. `expected.cpp` is C++23, the one file here cut by
standard rather than by domain, and it is its own probe: a toolchain that
cannot build it prints SKIPPED, and CI passes `--require-expected` so it can
never skip there.

The sync rule is the testlab discipline: the recipe functions here are quoted
**verbatim** in `book/F-rosetta-cookbook.md` — and three units are quoted
whole by Chapter 8 as well (`Result` and `load_config` from `errors.cpp`,
`channels_doubled` from `expected.cpp`), which each file's banner names.
Editing a recipe on either side means editing every page that quotes it in
the same commit. The `main()` functions are scaffolding
and appear in no listing — change those freely.
