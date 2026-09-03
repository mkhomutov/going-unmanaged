# Exercise: The Build-Model Lab

The eight breakages and why each error looks the way it does: **Chapter 23** of the book.
No reference solution — the artifact is your NOTES on what each error stage looks
like, because reading build errors is the skill. ~45 min.

Starting point (already green): `Greeter.h` / `Greeter.cpp` / `main.cpp` in this
directory. Build it, then break it the eight ways of Chapter 23, ONE at a time.
For each: predict the error stage (preprocessor / compile / link), provoke it,
paste the first error line into your notes with a one-line translation, restore,
move to the next.

Build: g++ -std=c++17 -Wall -Wextra -g Greeter.cpp main.cpp -o greeter
  (no sanitizers needed — the compiler and linker ARE the lab equipment;
   breakage 7 needs the two-step stale-object build described in the chapter)

`CMakeLists.txt` in this directory is not part of this lab. It belongs to
**Chapter 26**, which builds this same trio with CMake, and `build_all.sh`
keeps it green. Ignore it here — build by hand, because watching each stage
fail is the whole point. And if you are on Chapter 26's *Try it*, that file is
the answer: write your own, in a directory of its own, before you read it.
