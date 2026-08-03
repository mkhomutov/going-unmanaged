# cookbook/ — Appendix F's listings, compiled

This directory is not an exercise: there is no task card and nothing to
attempt cold. It holds the recipe listings of Appendix F (*The Rosetta
Cookbook*), one translation unit per domain —

| File | Recipes |
|---|---|
| `files.cpp` | 1 — read a whole file into a string |
| `strings.cpp` | 2–5 — split, join, build, format |
| `timing.cpp` | 6 — time a call |
| `handles.cpp` | 7 — wrap a C handle so it frees itself |
| `lookups.cpp` | 8 — look up a key without inserting it |

— each with a `main()` that asserts what its recipes claim.
`scripts/build_all.sh` builds and runs all five under the canonical flags on
every push, so a recipe that stops being true stops being green.

The sync rule is the testlab discipline: the recipe functions here are quoted
**verbatim** in `book/F-rosetta-cookbook.md`. Editing a recipe on either side
means editing both in the same commit. The `main()` functions are scaffolding
and appear in no listing — change those freely.
