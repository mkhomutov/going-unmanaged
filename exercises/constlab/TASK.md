# Exercise: The Const Lab

const as one subject rather than five fragments: **Appendix I** of the book.
~45 min.

## What is here

`counter.h` is a small const-correct class — a const half, a non-const half,
a `mutable` cache and an overload pair. `main.cpp` is two judges in one file.
Write your own first; these are the answer.

## The task

1. **Split a class's interface in two.** Take something you have written —
   the Chapter 15 Buffer is ideal — and mark every member function that does
   not change observable state `const`. Compile, and read the errors as a
   list of findings rather than a list of chores.
2. **Sort each error into three piles**, per Appendix I's procedure: it
   genuinely mutates (unmark it), it touches a cache or a lock (`mutable`),
   or it calls something that should itself be const (recurse — this is the
   transitive part, and where most of the errors come from).
3. **Add the overload pair** where a caller needs read access to internals,
   and make the const one return a reference the caller cannot write through.
4. **Prove `mutable` is honest.** Add a cached derived value, then assert
   that two calls *through a `const&`* compute it once — and read the
   recompute count through the non-const path, because a probe the const
   half can reach is exactly the trap this appendix warns about.

## The judge, and why it is unusual

Every other lab in this repository asserts that a program *runs* correctly.
This one cannot: **a const violation never reaches a binary.** So `main.cpp`
carries five of them behind `-D` guards, and `scripts/build_all.sh` compiles
the file once per guard and requires each to be *refused*:

```bash
../../scripts/check.sh main.cpp                            # must build and run
c++ -std=c++17 -DCONSTLAB_VIOLATION_1 -c main.cpp          # must NOT build
```

The five are worth predicting before you run them — say what the compiler
will object to in each case, then check:

1. Calling a non-const member function through a `const&`.
2. Assigning a non-`mutable` member inside a const member function.
3. Writing through the reference the const overload handed back.
4. Binding a `const` object to a `T&`.
5. Writing through a `const char*`.

Two things keep that check from being vacuous, and both are worth stealing
if you build something similar. The clean build must **succeed**, so a typo
that breaks the file for an unrelated reason fails there rather than passing
here. And the diagnostic must actually *name* const or read-only —
and the grep must see the *message* only, everything up to `error:` cut away
first, because this directory is called `constlab` and any path left in the
string matches "const" by itself.
