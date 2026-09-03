## Appendix I — Const-Correctness

`const` is the keyword most likely to look like something you already have and turn out to be a different idea entirely.

C#'s `const` is compile-time literal substitution — the value is baked into every call site, which is why changing one in a library and not rebuilding the callers is a known trap. `readonly` is closer: a field that cannot be reassigned after the constructor finishes. But neither of them says anything about what a *caller* may do with an object you handed them. That is what C++'s `const` is for, and it is the reason const shows up in every C++ code review the reader will ever sit in.

The mechanics — the three parameter forms, the trailing `const` on a method, `const char*` versus `char* const` and the read-it-backwards rule — are [Appendix A.5](A-fundamentals-refresher.md#appendix-a--fundamentals-refresher). This page is the model underneath them, the parts A.5 does not cover, and the procedure for adding const to a class that does not have it yet.

### The model: const describes a path, not an object

This is the sentence to carry away, and everything else on the page follows from it.

```cpp
Counter c;                    // c itself is not const
const Counter& view = c;      // but THIS path to it is

c.Add(5);                     // fine — a non-const path
view.Add(5);                  // compile error — a const one
```

The same object, reachable two ways at once, mutable through one and not the other. `const` did not describe `c`; it described the reference. That is why the discipline is called const-*correctness*: it is a property of your interfaces rather than of your data, and it is enforced entirely at compile time — there is no runtime check, no exception, and nothing to test for. A const violation is a bug that never reaches a binary, which is why `exercises/constlab/` judges this page by asserting that five of them **fail to compile**.

> [!NOTE]
> **Surprise for C# devs:** `readonly` protects a field from its own class's later assignments. `const` protects nothing at all — it protects a *route*, and the same object can be simultaneously writable to its owner and read-only to everyone it was lent to. C# can say that about a *struct* — `in` parameters, `readonly` members, `ref readonly` — and about a class it cannot say it at all, which is why `IReadOnlyList<T>` exists as a whole separate interface rather than as a keyword.

### The interface splits in two

A const member function is the promise that decides which half of your class a `const&` can reach. Mark them and the split appears for free:

```cpp
class Counter {
public:
    std::size_t Size() const;      // the const half — reachable through const&
    double Average() const;
    const int& At(std::size_t) const;

    void Add(int);                 // the other half — needs a non-const path
    int& At(std::size_t);
};
```

There is no separate `ICounterView` to write and keep in sync. The read interface is the const-marked subset of the one class, and a caller selects it by the type of reference they hold.

### Bitwise const, logical const, and `mutable`

The compiler enforces **bitwise** const: inside a const member function, no member may be assigned. What you usually mean is **logical** const: nothing a caller can *observe* changes. Caches, memo tables and mutexes sit in the gap between the two.

```cpp
double Average() const {
    if (!cached_) {                // recompute once, remember the answer
        average_ = Recompute();
        cached_ = true;
    }
    return average_;
}

mutable double average_ = 0.0;     // "not part of the value"
mutable bool   cached_  = false;
```

`mutable` is the escape hatch and it is narrow on purpose. It says this member is not part of the object's value, so writing it inside a const member function is legal. Caches, memoization, a `std::mutex` you must lock to *read* safely — all correct uses. `exercises/constlab/` asserts this one: two calls to `Average()` on a const reference, one computation, and nothing observable changed.

> [!WARNING]
> **Trap:** `mutable` on something a caller can observe makes `const` stop meaning anything on that class — and it will not warn you, because you asked for it.

### The overload pair

Same name, same parameters, different constness — and it is the constness of the *object* that selects, not the arguments:

```cpp
const int& At(std::size_t i) const { return data_[i]; }   // read-only path
int&       At(std::size_t i)       { return data_[i]; }   // writable path
```

The const overload returns `const int&` for the reason the whole page rests on: handing back a writable reference from a const member function would give the caller a non-const path to an object they reached through a const one. The promise would be intact in the signature and broken in fact. [Finding 8 of Chapter 25](25-findings-from-practice.md#chapter-25--findings-from-practice-a-living-log) is this pair found the hard way, on the Chapter 15 Buffer — including the half the compiler will not enforce for you.

Two small relatives of the pair. A container's `cbegin()` and `cend()` hand out `const_iterator`s whatever the constness of the container, so a loop that only reads can say so. And `std::as_const(x)` casts an lvalue to `const&` in place — the honest way to call the const overload on a non-const object, in a test say, without declaring a `const&` alias first.

### It is transitive, and that is the entire cost

Marking one function `const` forces everything it calls on `this` to be const too, and that requirement propagates as far as it needs to. Which produces the characteristic experience of retrofitting const into a codebase that never had it: **you add one keyword and get twenty errors.**

Those twenty are not the cost of const. A few are places the promise was already being broken with nothing checking; most are places it was never written down, and step 3 below is what tells them apart. But they arrive all at once, in code you may not own, and that is why const-correctness is nearly free if it is there from the first commit and genuinely expensive to add in year three — the same shape as [Chapter 27](27-dependency-management.md#chapter-27--dependency-management)'s "one version per binary, decided on purpose", where the discipline costs nothing until you try to adopt it late.

### Making an existing class const-correct

A procedure, because doing this by instinct is how `const_cast` gets into a codebase.

1. **Mark every member function that does not change observable state `const`.** All of them, in one pass. Do not compile yet.
2. **Compile, and read the errors as a list of findings.** Each one is a place where an observer was mutating something.
3. **Sort each error into one of three piles.** It genuinely mutates → unmark it, it was never an observer. It touches a cache, a memo or a lock → the member is `mutable`. It calls another member that should itself be const → recurse; this is the transitive part, and it is where most of the twenty come from.
4. **Add the const overload only where a caller needs read access to internals** — an `At`, a `begin`, a `Data`. Not everywhere.
5. **Reach for `const_cast` at no point in steps 1–4.** If it seems necessary, the answer is in pile one or pile two and has been misfiled.

### Where const does nothing

Three places the reflex misfires, and knowing them stops you writing `const` where it misleads or costs:

- **Top-level `const` on a by-value parameter.** `void Save(const int x)` is not part of the signature and callers cannot see it. It is a note to yourself about the body — harmless, and it does not mean what the reader of the header will assume it means.
- **`const` on a returned value.** `const std::string Make()` is free where the result *initializes* something — `std::string s = Make();` is elided either way, because elision compares the cv-unqualified type. It costs a copy wherever the result is *bound* instead: `s = Make()` and `v.push_back(Make())` both fall to the copy overload, because a const rvalue will not bind to `T&&`. A pessimization where it lands, and a common one in older code.
- **`const` on a local.** Useful to you; almost never the thing that lets the optimizer do something it could not otherwise prove.

### `const_cast`, and the one time it is not a lie

Casting away const and then writing through the result is undefined behavior **whenever the object underneath is genuinely const** — and where that object lives decides how you find out. A namespace-scope `const` usually sits in read-only memory, so the write faults and the process dies on the spot; a const local usually takes the write silently and then disagrees with itself, because the compiler already folded the value you promised would not change. Nothing is watching for either: the fault is the memory system's rather than const's, and the cast is legal and the write looks ordinary.

There are two honest uses. The first is the one [Chapter 9](09-casts-conversions-and-strings.md#chapter-9--casts-conversions-and-strings)'s cast table names: calling a C API that does not modify its argument but forgot to say so in its signature — the object is not const, only your handle to it was. The second is implementing the non-const overload in terms of the const one, so a non-trivial accessor is written once:

```cpp
const int& At(std::size_t i) const { /* bounds checks, logging */ return data_[i]; }

int& At(std::size_t i) {
    // Safe in this exact direction: the object is known non-const here,
    // because we are in a non-const member function.
    return const_cast<int&>(static_cast<const Counter&>(*this).At(i));
}
```

The direction is what makes it legal. Adding const to reach the other overload is always fine; casting it away is only fine when you can prove the object was never const to begin with, and in a non-const member function you can.

### `const` is not `constexpr`

They are conflated constantly and they answer different questions. `const` is "I will not write through this path". `constexpr` is "this can be computed during compilation". A `constexpr` variable is also const; a const variable is usually not constexpr. If you find yourself wanting the C# `const` you remember — a compile-time constant baked into the caller — the C++ spelling is `constexpr`, and the baked-into-the-caller trap comes with it exactly as it does in C#.

### Choosing the parameter shape is not on this page

`const&` is one branch of a four-way decision, and re-deriving it here would put the same reasoning in two places. [Appendix H](H-choosing.md#appendix-h--choosing-signatures-containers-and-storage) is the procedure — whether the function keeps a copy, whether the type is a polymorphic base, and what the sink actually costs when the caller passes an lvalue.

### Pitfalls

- **Adding `const_cast` to make step 2 of the procedure quiet.** It converts a compile error you can see into undefined behavior you cannot, which is the worst trade in this book.
- **`mutable` on a member a caller can observe.** The class still compiles and `const` now guarantees nothing about it.
- **Returning a non-const reference from a const member function.** It compiles whenever the member is a pointer of *any* kind — raw, `unique_ptr`, `shared_ptr` — or a reference: const reaches the handle and stops there. Swap in a `std::vector` and the same line becomes a compile error, which is one more argument for the container. It is also why [Chapter 30](30-authoring-an-abi-boundary.md#chapter-30--authoring-an-abi-boundary)'s PIMPL — `int Widget::Score() const { return impl_->score; }` — is a promise you keep by hand rather than one the compiler keeps for you.
- **Assuming `const` means thread-safe.** It means no writes through this path. Two threads reading through const references are fine; a `mutable` cache behind one of them is a data race, which is [Chapter 29](29-concurrency.md#chapter-29--concurrency)'s subject and the reason a `mutable std::mutex` is such a common sight.
- **Marking a whole class's methods const to silence a caller.** The caller's type was the thing to fix.

> [!TIP]
> **Key principle:** "const describes a path, not an object — so I mark the member functions that do not change observable state, and the read interface appears for free."

> [!TIP]
> **Key principle:** "A const violation is a compile error, never a runtime one — so const-correctness costs nothing at the start of a class and twenty errors in year three, and I pay it at the start."

> [!TIP]
> **Key principle:** "`mutable` is for members that are not part of the value — a cache, a memo, a mutex — and never for anything a caller can observe."

---

<!-- nav:begin -->
[← Appendix H — Choosing: Signatures, Containers, and Storage](H-choosing.md) · [Contents](README.md)
<!-- nav:end -->
