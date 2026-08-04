# Exercise: Lambda Lifetimes

Full statement and the fixed patterns: **Chapter 22** of the book.
Do it COLD first. ~45 min. The C# contrast is the whole point: C# closures keep
captured objects alive via the GC; C++ captures do exactly what you wrote,
including dangling.

The three tasks:

1. `MakeCounter()`: return a lambda that returns 1, 2, 3... on successive calls.
   The naive `[&count]` version compiles and dangles — demonstrate, then fix.
2. A `Button` struct storing a `std::function<void()>` callback that prints a
   label — where the label is created in a NARROWER scope than the click.
   Make it correct by ownership design, not by luck.
3. Capture-by-move: a lambda that OWNS a `unique_ptr` (something copies can't do).

Build: g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -g your.cpp -o task
  (or: ../../scripts/check.sh your.cpp — from this directory)
  (Windows: ..\..\scripts\check.ps1 your.cpp — from a Developer PowerShell)
