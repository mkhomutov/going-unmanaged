# Exercise: The Build-Model Lab

The seven breakages and why each error looks the way it does: **Chapter 23** of the book.
No reference solution — the artifact is your NOTES on what each error stage looks
like, because reading build errors is the skill. ~45 min.

Starting point (already green): `Greeter.h` / `Greeter.cpp` / `main.cpp` in this
directory. Build it, then break it the seven ways of Chapter 23, ONE at a time.
For each: predict the error stage (preprocessor / compile / link), provoke it,
paste the first error line into your notes with a one-line translation, restore,
move to the next.

Build: g++ -std=c++17 -Wall -Wextra -g Greeter.cpp main.cpp -o greeter
  (no sanitizers needed — the compiler and linker ARE the lab equipment;
   breakage 7 needs the two-step stale-object build described in the chapter)
