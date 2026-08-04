# Exercise: The Buffer (the Rule of Five, for real)

Full statement, reference solution, and the unhappy path: **Chapter 15** of the book.
Do it COLD first — from memory, no lookups; solution afterwards.

Write: a class owning a heap `int` array (`size_`, `data_`). Destructor frees;
copy ctor deep-copies; copy assignment (aim for copy-and-swap); move ctor steals
AND NULLS the source; move assignment frees own data, steals, nulls, self-move-safe;
`noexcept` where it is TRUE; `explicit` where it belongs; zero-initialized storage;
an element accessor.

Prove: a `main` exercising all five paths with predictions written as comments
before running — including assignment over an EXISTING buffer, a vector with
reallocation, and `b = std::move(b)`. Then the sabotage runs under ASan
(Chapter 15, "Experiments").

Build: g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -g your.cpp -o task
  (or: ../../scripts/check.sh your.cpp — from this directory)
  (Windows: ..\..\scripts\check.ps1 your.cpp — from a Developer PowerShell)
