# Exercise: Slicing and Polymorphism

Full statement, the broken version, and the fixed design: **Chapter 20** of the book.
Do it COLD first; write predictions BEFORE compiling. ~45 min.

Write: a small `Shape` hierarchy (`Circle`, `Rect`) with a virtual `Area()`.
Deliberately do it WRONG first: put shapes into a `std::vector<Shape>` and total
the areas — predict what happens, then observe (make the base abstract and watch
the design error become a compile error). Then fix the design so polymorphism
actually works, and prove with output that the right `Area()` runs per element.
Finally: remove `virtual` from the base destructor and demonstrate under ASan
(heap-allocated derived object owning memory) what breaks.

Build: g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -g your.cpp -o task
  (or: ../../scripts/check.sh your.cpp — from this directory)
