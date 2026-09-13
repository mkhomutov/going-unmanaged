# The Callers Must Not Notice — ticket card (Chapter 45)

This lab is a **ticket, not a task**, and the only one in the book whose
starting point *works*. `before/` is a class that has shipped since 2009;
`main.cpp` is a caller as one of three teams wrote it; the ticket is to
modernise the class without changing a byte of the caller or a line of its
behaviour. Chapter 45 states everything in full and walks the diagnosis
behind a spoiler fold — work the ticket cold first, one seam at a time.

> **Modernise `Catalog` for 4.0 — without touching the callers.** The
> native layer's `Catalog` is 2009 code: raw pointers, a hand-rolled
> array, a manual `Clear`. The 4.0 snapshot feature takes a copy of a
> `Catalog`, and the feature branch dies at exit under the sanitizers —
> *heap-use-after-free in `Catalog::Clear`*. The review board's rule for
> 4.0 is *no owning raw pointers in the native layer*. Three teams' files
> include `catalog.h`; none of them may change, and none of their
> behaviour may.

**The files beside this card:** `before/catalog.h` and `before/catalog.cpp`
are the 2009 class, working and green under the flags — the starting
point, and the one you modernise in a copy of your own. `main.cpp` is the
caller: it never changes. `snapshot.cpp` is the 4.0 feature, with the judge
for the retrofit's promises built in. **`after/` is the FIXED reference**,
kept green by `build_all.sh` on every push — the same `main.cpp` built
against `before/` and against `after/`, their outputs compared byte for
byte, then `snapshot.cpp` against `after/`. Do not start from it.

## Work the ticket

1. **Read the header for the promises it makes**, in its comments as much
   as its declarations: what `Find` says about the address it returns,
   what `Parse` says about malformed input, what `Clear` is for, what an
   empty `Catalog` is. Write them down. That list is the contract you are
   about to keep, and the part of it the compiler cannot check.
2. **Reproduce.** Build the caller against `before/` — green, six lines of
   output; keep that output. Build `snapshot.cpp` against `before/`: the
   report from the ticket. Read it against Chapter 6: which special member
   is on the access stack, who wrote it, and what did the *freed by*
   stack's `Grow` do to the *other* catalog?
3. **Seam 1 — declare what the compiler was writing for you.** Two lines
   in the header: the copy operations, `= delete`. Rebuild the caller (it
   compiles — that is the build proving the claim). Rebuild the snapshot
   branch (it does not, at the copy line, naming the deleted function).
4. **Seam 2 — move the ownership inside.** Before you choose the container,
   predict what `std::vector<Entry>` would do to the caller's `alpha`; then
   try it and run the byte-identical caller under the flags. Then choose
   again, and keep the promise.
5. **Seam 3 — let the destructor go.** `= default`, with `Clear` still
   public because callers call it. Say why this seam comes after seam 2.
6. **Seam 4 — earn the copy.** Deep copy, move, copy-and-swap. The snapshot
   branch compiles and its judge passes. At every seam, the caller against
   your current state, its output compared byte for byte with the 2009
   output, under the full flags:

   ```bash
   INC=before scripts/check.sh before/catalog.cpp main.cpp > out.2009
   INC=yours scripts/check.sh yours/catalog.cpp main.cpp > out.now && cmp out.2009 out.now
   ```

   (`INC` is where `main.cpp`'s `#include "catalog.h"` is resolved, since
   the caller sits beside two directories that each hold one; keep each
   state's `catalog.h` beside its `catalog.cpp`.)
7. **Stretch: the boundary.** Suppose `catalog.h` shipped in an SDK and the
   three teams' binaries could not be rebuilt. Which of the four seams are
   still allowed? Take the answer to Chapter 30 and check it against
   `sizeof(Catalog)` before and after.
