# Exercise: Iterator Invalidation

Full statement and the reasoning: **Chapter 21** of the book.
Format: write each loop the NAIVE way first, predict the failure mode, run it
under ASan, and only then fix. ~45 min.

The three tasks:

1. Remove all odd numbers from a `vector<int>` while iterating.
2. Append a copy of every element to the same vector while iterating it.
3. Hold a reference to `v[0]`, `push_back` once, then use the reference.

Build: g++ -std=c++20 -Wall -Wextra -fsanitize=address,undefined -g your.cpp -o task
  (C++20 for std::erase_if, the modern fix for task 1;
   or: STD=c++20 ../../scripts/check.sh your.cpp — from this directory)
   (Windows: $env:STD='c++20'; ..\..\scripts\check.ps1 your.cpp)
