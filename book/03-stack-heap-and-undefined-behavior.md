## Chapter 3 — Stack, Heap, and Undefined Behavior

The physical model underneath everything else in this book. C# hides it behind the GC; C++ makes you its manager — and punishes ignorance with undefined behavior.

### Stack vs heap, explicitly

A question worth being able to answer instantly: "where does this variable live?"

```cpp
void F() {
    int x = 5;                        // STACK: freed automatically at }
    Widget w;                         // STACK: whole object, dtor at }
    Widget* p = new Widget();         // w on HEAP, p itself on stack
    auto u = std::make_unique<Widget>();  // heap object, stack owner
    std::vector<int> v(1000);         // v's bookkeeping on stack,
}                                     // the 1000 ints on HEAP
```

**Stack**: allocation is one pointer bump — near-free; freed in reverse order automatically; small (~1 MB per thread typically) — huge arrays as locals overflow it; the natural home of value semantics and RAII. **Heap**: for objects that outlive the current scope, sizes unknown at compile time, or big data; slower (allocator work, cache misses); in modern C++ you touch it almost exclusively through containers and smart pointers, never bare new.

Contrast to internalize: in C# every class instance is heap + GC, full stop. In C++ heap use is a deliberate choice — and good C++ minimizes it. "Why is this on the heap?" is a legitimate code review question.

### Undefined behavior (UB) as a concept

UB is not "an exception is thrown" and not "the program crashes". It means **the standard places no requirements whatsoever** on what happens — and crucially, **the compiler is allowed to assume UB never occurs** and optimize accordingly. Result: code that works in Debug, breaks in Release; works on your machine, corrupts data in production; appears to work for years.

The greatest hits, all met in this book:

- dereferencing null or dangling pointers/references (lambda capturing dead locals, string_view to a temporary, c_str() outliving its string)
- out-of-bounds access: v[i] past the end, iterator invalidation (Chapter 11)
- use-after-move beyond assign/destroy (Chapter 6)
- deleting through a base pointer without a virtual destructor (Chapter 5)
- double-free (the shallow-copy bug behind = delete)
- signed integer overflow (unsigned wraps; signed is UB!), data races on unsynchronized shared data

Why C++ tolerates this: checks cost cycles, and C++'s contract is "you don't pay for what you don't use". The language trusts you; tooling backs you up:

```bash
# AddressSanitizer - catches heap/stack corruption, use-after-free at runtime
clang++ -fsanitize=address,undefined -g main.cpp
# also: MSVC /fsanitize=address, valgrind, and static analysis (clang-tidy)
```

> **Key principle:** "I treat warnings as errors, run sanitizers regularly, and reach for AddressSanitizer the moment anything smells like memory corruption."

The plug-in angle: when your code runs inside a host application — a CAD package, a DAW, an office suite — a memory bug in your plug-in doesn't crash your plug-in. It crashes the *host*, possibly minutes later in unrelated code, taking the user's unsaved work with it. That is why the discipline in this book (RAII, ownership, invalidation rules) is the job, not pedantry.

---

---


<!-- nav:begin -->
[← Chapter 2 — Value Semantics](02-value-semantics.md) · [Contents](README.md) · [Chapter 4 — Classes, Inheritance, Interfaces →](04-classes-inheritance-interfaces.md)
<!-- nav:end -->
