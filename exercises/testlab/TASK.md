# Exercise: The Test Lab (write the harness, then test the Buffer)

Full statement and the demonstration that closes it: **Chapter 28** of the book,
*Try it*. Do steps 1–3 COLD — the framework from the description, not from the
listing; the reference files here are the comparison afterwards. ~60 min.

Write: `tiny_test.h`, doing the three jobs any test framework does — a registry
that survives static initialization, a `CHECK` macro capturing the expression
text and the call site, a runner returning a meaningful exit code. Then extract
the Chapter 15 Buffer into `Buffer.h` so a demo and a test binary can both
include it (a .cpp with `main()` cannot link into a binary that has its own —
that structural fact is the chapter's point). Then the suite: deep copy, copy
assignment across sizes, self-assignment, the moved-from husk, move assignment
over an existing buffer, self-move, and vector reallocation.

Prove: break one expected value, confirm red and exit code 1, put it back. Then
the run the chapter is built around — break ONLY the null-out in the move
constructor, watch every assertion still pass, and rebuild under
`-fsanitize=address,undefined` to get the double-free report. That last one is
book-only on purpose: it exists to fail, so it is not in `build_all.sh`.

Build: ../../scripts/check.sh your_test.cpp
  (Windows: ..\..\scripts\check.ps1 your_test.cpp — from a Developer PowerShell)
  (from a directory of your own, with YOUR `Buffer.h` next to your test.
   Extracting it is step 2 — build against your own extraction, not this
   repository's, or you have skipped the part the chapter is about. check.sh
   has no include-path option and needs none when the header sits next to
   the test.)

Afterwards, to run the reference suite against the already-extracted header in
`solutions/` — the same thing `build_all.sh` does:
  g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -g \
      buffer_test.cpp -I ../../solutions -o test

The two files here — `tiny_test.h` and `buffer_test.cpp` — are the chapter's
listings verbatim, and `scripts/build_all.sh` builds and runs the suite under
the canonical flags on every push. Editing them means editing Chapter 28 in the
same commit.
