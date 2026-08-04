# Exercise: The Lifetime Tracer

Full statement, the complete instrument, and annotated output: **Chapter 14** of the book.
This one you build and KEEP — it is a diagnostic instrument, not a puzzle.

Write: a `Tracer` class printing from the five special member functions it defines
(dtor, copy ctor/assign, move ctor/assign) plus the ordinary constructor that names
it, stamped with an instance ID and its own address.
Push several into a `std::vector` WITHOUT `reserve` and predict every output line
before running.

Then the three experiments (Chapter 14): delete `noexcept` from the move ctor;
add `v.reserve(4);`; self-assignment `x = x`. Predict each before running.

Build: g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -g your.cpp -o task
  (or: ../../scripts/check.sh your.cpp — from this directory)
