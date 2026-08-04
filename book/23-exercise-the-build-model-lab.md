## Chapter 23 — Exercise: The Build-Model Lab

*Trains: Chapter 12, hands-on. Time: ~45 min. No reference solution file — the artifact is your notes on what each error looks like, because reading build errors is the skill.*

### The setup

Split a trivial `Greeter` class across `Greeter.h` / `Greeter.cpp` with a `main.cpp` consumer. Get it building. Then break it seven ways, one at a time, and for each: predict the error *stage* (preprocessor / compile / link), provoke it, and paste the first error line into your notes with a one-line translation.

### The seven breakages

1. **Delete the `#include "Greeter.h"` from main.cpp** → compile error, `undeclared identifier` — or, for a type name, clang's `unknown type name 'Greeter'`. The translation unit never saw the declaration.
2. **Delete Greeter.cpp from the build command** (compile main.cpp alone) → **linker** error: `undefined reference` (GNU ld) / `Undefined symbols for architecture ...` (Apple ld) / `LNK2019 unresolved external` (MSVC). Everything compiled; the body is missing at link time. Learn to tell this apart from #1 at a glance — it is the single most practical build skill.
3. **Declare a method in the header, never define it anywhere,** and call it → same linker error as #2. Same symptom, different cause; the error text is identical, which is exactly why the *cause list* for unresolved externals belongs in your notes: missing .cpp in build, missing library, declared-never-defined, template body in a .cpp (Chapter 7).
4. **Remove the include guard** (`#pragma once`) and include the header twice via a second header → compile error, `redefinition of 'class Greeter'`. #include is paste; the guard is what makes double-paste harmless.
5. **Define a free function in the header** (outside the class, no `inline`), include it from two .cpp files → **linker** error: `multiple definition` (GNU ld) / `duplicate symbol` (Apple ld) / `LNK2005` (MSVC). The One Definition Rule enforced. Fix three ways and note the difference: `inline`, move the body to a .cpp, or make it a class member defined in-class (implicitly inline).
6. **Create a circular include** (A.h includes B.h includes A.h, guards present) and use B's type in A → include order decides what you see, which is the lesson: enter through A.h and the guards happen to short-circuit in an order that compiles clean; enter through B.h (make main.cpp include B.h first) and the cycle breaks with `unknown type name 'B'` — an undeclared name, nothing "incomplete" yet. Fix with a forward declaration in one of the headers — and note which usages permit forward declaration (pointers, references) and which demand the full definition (members by value, inheritance): keep a by-value member with only the forward declaration and *now* you meet `field has incomplete type`, the error of the half-fixed state.
7. **Change a class definition in the header, rebuild only main.cpp** (simulating a stale object file: compile Greeter.cpp, *then* edit the header, then compile only main.cpp and link both) → it links and misbehaves or crashes: an **ODR violation across translation units**, undetectable by the linker. This is why build systems track header dependencies and why "clean build fixes it" is a real phenomenon with a real cause — the moment you understand this breakage, incremental-build weirdness stops being mysterious.

### Why this lab earns its place

Half of all confusing C++ errors are build-model errors (Chapter 12). At work, against a vendor SDK with heavy headers and multi-project solutions, error-stage triage is the first move of every debugging session: *which tool complained — preprocessor, compiler, or linker — and therefore which file do I open?* After this lab, that triage takes five seconds.

---


<!-- nav:begin -->
[← Chapter 22 — Exercise: Lambda Lifetimes](22-exercise-lambda-lifetimes.md) · [Contents](README.md) · [Chapter 24 — Practice Plan →](24-practice-plan.md)
<!-- nav:end -->
