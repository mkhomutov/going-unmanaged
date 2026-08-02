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
- reading an uninitialized local or member — an *indeterminate* value, and the one entry on this list a C# developer has never met (its own section below)
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

> [!TIP]
> **Key principle:** "I treat warnings as errors, run sanitizers regularly, and reach for AddressSanitizer the moment anything smells like memory corruption."

The plug-in angle: when your code runs inside a host application — a CAD package, a DAW, an office suite — a memory bug in your plug-in doesn't crash your plug-in. It crashes the *host*, possibly minutes later in unrelated code, taking the user's unsaved work with it. That is why the discipline in this book (RAII, ownership, invalidation rules) is the job, not pedantry.

### The UB you have never met: uninitialized values

Every other item on that list has a C# equivalent you can at least imagine. This one does not exist in C# at all. Fields are zeroed by the runtime before your constructor body runs, and **definite assignment** makes reading an unassigned local a *compile error* — `int x; Console.WriteLine(x);` does not build, and never has. So a C# developer has not once debugged an uninitialized read, and arrives with no reflex for it whatsoever.

C++ gives you neither guarantee:

```cpp
void F() {
    int x;                 // no initializer: x holds whatever bytes were there
    if (x > 0) { ... }     // UB - and it will look like it works
}
```

`int x;` *default-initializes*, and for built-in types default-initialization does nothing at all. The value is **indeterminate**; reading it is undefined behavior of the quiet kind. Debug builds often show 0 on Linux and macOS, because pages fresh from the OS are zeroed; MSVC's Debug runtime checks (`/RTC1`, on by default in the Visual Studio Debug configuration) fill the slot with a pattern instead, so you read `-858993460` — `0xcccccccc`, the most recognizable Windows tell that nobody wrote to this variable. Both are accidents of the build rather than guarantees, and Release shows whatever that stack slot last held. That is exactly the works-on-my-machine signature this chapter opened with, and the reason it is worth its own section: the bug does not announce itself, and you have no instinct that says to look.

> [!WARNING]
> **Trap:** a variable that reads the same value every time in Debug — 0, or `0xcccccccc` under MSVC — is not initialized; it is one stack frame away from garbage.

And the sanitizer reflex from the previous section does not cover this one: Address and UB sanitizers do not report an uninitialized read. The tool that does is **MemorySanitizer** (`-fsanitize=memory`), which is Linux-and-clang-only and cannot be combined with ASan — so a clean `-fsanitize=address,undefined` run on a Mac says nothing whatsoever about this bug. Here the compiler's warnings are the first line of defense, not the runtime's.

The same rule reaches three places beyond the plain local. A **member left out of the initializer list** (Chapter 4) is default-initialized on exactly these terms — the constructor compiled, the member holds junk. **`new T[n]` without braces** does not zero either — `new int[n]{}` does, and one pair of braces is the whole fix (Finding 7 in Chapter 25 has the rest). And a **C API struct** you hand to a vendor function is the same hazard with a longer fuse, which is why Chapters 2 and 17 write `= {}` on every one of them.

The habit is three lines long. Initialize at the point of declaration, so there is no window in which the variable is readable and wrong. Put `= {}` on every API struct. And when the compiler hedges that a variable *might* be used uninitialized — GCC's `-Wmaybe-uninitialized`, clang's `-Wsometimes-uninitialized`, MSVC's C4701 *potentially uninitialized local variable* — treat it as a certainty rather than a maybe: the hedge is there because the compiler could not prove the path, not because it thinks you are probably fine.

---

---


<!-- nav:begin -->
[← Chapter 2 — Value Semantics](02-value-semantics.md) · [Contents](README.md) · [Chapter 4 — Classes, Inheritance, Interfaces →](04-classes-inheritance-interfaces.md)
<!-- nav:end -->
