# choosing/ — Appendix H's claims, checked

This directory is not an exercise: there is no task card and nothing to
attempt cold. It holds the measurements behind Appendix H (*Choosing:
Signatures, Containers, and Storage*), so the appendix can state costs
instead of assurances —

| File | Procedures | What it checks |
|---|---|---|
| `counted.h` | — | the instrument: a type that tallies its own copies and moves, plus the `CHECK` judge |
| `passing.cpp` | 2, 3 | what a sink costs against `const&` for each kind of caller — in copies, moves **and allocations** — the missing `&` in a loop, and what returning costs |
| `storing.cpp` | 1, 4 | that vector growth relocates elements while boxed ones stand still, what each costs, and which containers spare references |

**The sync rule.** Some units here are quoted in `book/H-choosing.md`, and
each file's banner names exactly which. `scripts/check_verbatim.sh` holds
the pairing in **both** directions: every `cpp` fence on the page must be
byte-identical to something in this directory, and every unit the banners
name must appear on the page *whole*. Editing one side means editing the
other in the same commit — the cookbook discipline. Anything not named in a
banner, `main()` included, is scaffolding and appears in no listing.

**The judge is `CHECK`, not `assert`.** `assert` compiles to nothing under
`-DNDEBUG`, which a CMake `Release` build defines (Chapter 26) — an
assert-judged harness would print its success line and exit 0 having
verified nothing. `CHECK` counts failures and sets the exit code, the same
choice `perflab` and `bridgelab` made. `counted.h` also carries a
`static_assert` on the noexcept move, which survives even that.

`scripts/build_all.sh` builds and runs both TUs under the canonical flags
on every push, and `passing.cpp` a **second** time under
`-fno-elide-constructors`. That second build is the point of the return
figures: with NRVO on, a returned temporary and a returned named local both
measure zero moves, so the distinction the appendix draws between them —
one guaranteed, one merely permitted — is never exercised. Switching NRVO
off leaves mandatory C++17 elision alone and makes the two differ.

One check is deliberately loose, and the looseness is the lesson: returning
a *named* local is checked at zero copies but only `moves <= 1`, because
NRVO is permitted rather than guaranteed (Chapter 6). Returning a temporary
is checked at zero of both — that one is mandatory since C++17.
