# choosing/ — Appendix H's claims, asserted

This directory is not an exercise: there is no task card and nothing to
attempt cold. It holds the measurements behind Appendix H (*Choosing:
Signatures, Containers, and Storage*), so the appendix can state costs
instead of assurances —

| File | Procedures | What it asserts |
|---|---|---|
| `counted.h` | — | the instrument: a type that tallies its own copies and moves |
| `passing.cpp` | 2, 3 | what a sink costs against `const&` for each kind of caller; the missing `&` in a loop; what returning costs |
| `storing.cpp` | 1, 4 | that vector growth relocates elements and boxed elements stand still, and what each costs |

`counted.h` and the quoted functions in both TUs appear verbatim in
`book/H-choosing.md`: editing one means editing the appendix in the same
commit (the cookbook discipline, enforced by `scripts/check_verbatim.sh`).
The `main()`s are scaffolding and appear in no listing.

`scripts/build_all.sh` builds and runs both under the canonical flags on
every push. The point is that the appendix's numbers cannot rot: if a
future toolchain elides differently, the build fails rather than the page
quietly lying.

One assertion is deliberately loose, and the looseness is the lesson:
returning a *named* local asserts zero copies but only `moves <= 1`,
because NRVO is permitted rather than guaranteed (Chapter 6). Returning a
temporary asserts zero of both — that one is mandatory since C++17.
