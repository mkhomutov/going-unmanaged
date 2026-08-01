## Chapter 7 — Templates vs C# Generics

They look identical — `List<T>` vs `std::vector<T>` — but the machinery is completely different. **C# generics are one compiled thing that works for any T at runtime; C++ templates are a code-generation machine** — the compiler stamps out a separate, fully compiled version for *each* T you use, at compile time. This is called **instantiation**.

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
// C++20 concepts = C#'s where clauses, 20 years late
template <typename T>
requires std::totally_ordered<T>
T Max(T a, T b) { return a > b ? a : b; }

void Sort(std::ranges::range auto& container);   // terse form
```

### Consequence 2 — templates live in headers

The compiler must see the full template source to stamp out a version for your T, so template code cannot hide in a .cpp file — implementation and all go in the header. Put it in a .cpp and consumers get **linker errors** (unresolved external).

### Consequence 3 — templates are more powerful than generics

```cpp
template <typename T, int N>       // value parameters! C# cannot do this
class FixedArray {
    T data[N];                     // size baked in at compile time
};
FixedArray<double, 3> vec3;        // this is how std::array works
```

Non-type parameters, specialization, compile-time metaprogramming. Modern C++ prefers constexpr functions and concepts over the old arcane template tricks.

### Consequence 4 — no runtime type info via templates

`typeof(T)`, reflection, `GetType()` — none of that exists. T is gone after compilation. The little runtime typing C++ has is RTTI, working only on polymorphic types:

```cpp
Shape* p = GetShape();
Circle* c = dynamic_cast<Circle*>(p);   // like C# 'as' - nullptr if not Circle
if (c) c->radius = 5;
```

> [!IMPORTANT]
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

---


<!-- nav:begin -->
[← Chapter 6 — The Rule of Five and Move Semantics](06-the-rule-of-five-and-move-semantics.md) · [Contents](README.md) · [Chapter 8 — Error Handling: Exceptions and Error Codes →](08-error-handling.md)
<!-- nav:end -->
