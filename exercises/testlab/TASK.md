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
assignment across sizes, self-assignment, the moved-from husk, vector
reallocation.

Prove: break one expected value, confirm red and exit code 1, put it back. Then
the run the chapter is built around — break ONLY the null-out in the move
constructor, watch every assertion still pass, and rebuild under
`-fsanitize=address,undefined` to get the double-free report. That last one is
book-only on purpose: it exists to fail, so it is not in `build_all.sh`.

Build: g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -g \
         your_test.cpp -I ../../solutions -o test
  (`-I` because the extracted `Buffer.h` lives in `solutions/` — this repo's own
   copy of step 2, already done. `scripts/check.sh` has no include-path option,
   so use the line above for a test that includes it; `check.sh your_test.cpp`
   works if your `Buffer.h` sits next to your test.)

The two files here — `tiny_test.h` and `buffer_test.cpp` — are the chapter's
listings verbatim, and `scripts/build_all.sh` builds and runs the suite under
the canonical flags on every push. Editing them means editing Chapter 28 in the
same commit.
