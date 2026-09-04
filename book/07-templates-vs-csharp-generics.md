## Chapter 7 — Templates vs C# Generics

They look identical — `List<T>` vs `std::vector<T>` — but the machinery is completely different. **C# generics are compiled once to IL and specialized by the runtime — one shared body for every reference type, a fresh one for each value type; C++ templates are a code-generation machine** — the compiler stamps out a separate, fully compiled version for *each* T you use, at compile time, from source. This is called **instantiation**.

Note what that does and does not say. .NET generics are *reified*, not erased: the JIT really does emit a distinct specialization per value type, which is why `List<int>` boxes nothing and why `typeof(T)` works. The difference is *when* and *from what* — the JIT specializes from IL at run time, on demand, for the types actually used; C++ specializes from source at compile time, and every instantiation is in the binary before the program starts.

**For Java readers:** your baseline is the opposite one — erasure, one shared body, boxed primitives — so do not pattern-match the C# column onto your generics. The compensation: C++ templates are the per-type specialization Java never had, while Consequence 4's T-is-gone-at-runtime is the erasure you have always lived with. Half of this chapter is home turf.

```cpp
template <typename T>
T Max(T a, T b) { return (a > b) ? a : b; }

Max(3, 5);        // compiler GENERATES int Max(int, int)
Max(2.5, 1.0);    // compiler GENERATES double Max(double, double)
Max(str1, str2);  // generates a std::string version
```

Nothing is decided at runtime — no boxing, no type checks, zero overhead. That is why `std::sort` on a `vector<int>` beats C's qsort: the comparison inlines completely.

### Consequence 1 — duck typing (and C++20 concepts)

C# demands constraints up front (`where T : IComparable<T>`). Templates declare nothing — the compiler just tries to compile your code with T. If T has `operator>`, it works; if not, you get an error at the point of use, often a notoriously long one ("template error novels").

```cpp
// C++20 concepts = C#'s where clauses, 15 years late
template <typename T>
requires std::totally_ordered<T>
T Max(T a, T b) { return a > b ? a : b; }

void Sort(std::ranges::range auto& container);   // terse form
```

### Consequence 2 — templates live in headers

The compiler must see the full template source to stamp out a version for your T, so template code cannot hide in a .cpp file — implementation and all go in the header. Put it in a .cpp and consumers get **linker errors** (unresolved external). (The one sanctioned exception, which you will meet in vendor code: a library that knows every T it will ever serve can keep the body in a .cpp and add an **explicit instantiation** — `template class Foo<int>;` — for exactly those types. It is a build-time optimization for closed sets, not the default.)

**Try it (30 seconds).** Move a template's body into a .cpp behind a declaring header and predict the failure *stage* — compiler or linker — before building from another file. Telling those two apart at a glance is the Chapter 12 skill, arriving early — and that chapter's what-goes-where table is the row this rule belongs to.

### Consequence 3 — templates are more powerful than generics

```cpp
template <typename T, int N>       // value parameters! C# cannot do this
class FixedArray {
    T data[N];                     // size baked in at compile time
};
FixedArray<double, 3> vec3;        // this is how std::array works
```

Non-type parameters, specialization, compile-time metaprogramming. Modern C++ prefers constexpr functions and concepts over the old arcane template tricks — and the handful of templates a plug-in author actually writes, with the one trick worth owning, is [Chapter 41](41-templates-you-will-write.md#chapter-41--templates-you-will-write).

### Consequence 4 — no runtime type info via templates

`typeof(T)`, reflection, `GetType()` — none of that exists. T is gone after compilation. The little runtime typing C++ has is RTTI, working only on polymorphic types:

```cpp
Shape* p = GetShape();
Circle* c = dynamic_cast<Circle*>(p);   // like C# 'as' - nullptr if not Circle
if (c) c->radius = 5;
```

> [!TIP]
> **Key principle:** dynamic_cast is legal but culturally frowned upon — needing it often signals the virtual interface is designed wrong. "I'd prefer adding a virtual method over dynamic_cast chains."

### Trade-off summary

| | C# generics | C++ templates |
|---|---|---|
| When resolved | runtime (JIT) | compile time |
| Constraints | where, enforced upfront | none pre-C++20; concepts in C++20 |
| Performance | some overhead for ref types | zero — fully specialized code |
| Code location | anywhere | headers |
| Cost | — | slower builds, bigger binaries, ugly errors |
| Reflection on T | yes | no |

### In the wild: C-style SDKs

Established C++ SDKs frequently ship their own template container libraries paralleling the STL — Qt's `QVector`/`QMap`, Unreal's `TArray`/`TMap`, and many vendor equivalents born before the STL was trustworthy on all platforms. STL fluency translates directly: the concepts (and the invalidation rules) are the same, only the spelling differs. Expect to read the vendor's containers in API samples and convert at the boundary.

---


<!-- nav:begin -->
[← Chapter 6 — The Rule of Five and Move Semantics](06-the-rule-of-five-and-move-semantics.md) · [Contents](README.md) · [Chapter 8 — Error Handling: Exceptions and Error Codes →](08-error-handling.md)
<!-- nav:end -->
