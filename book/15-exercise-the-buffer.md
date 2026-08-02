## Chapter 15 — Exercise: The Buffer

The Tracer (Chapter 14) had `std::string` silently doing the dangerous work. The Buffer replaces it with a raw `int*` — the same five functions, but now every ordering mistake is a heap corruption instead of a style nit. This chapter contains the exercise, the reference solution, and the four findings a real first attempt produced (logged as Findings 6–9 in Chapter 25).

### The exercise

Write from memory: a class owning a heap array of ints (`size_`, `data_`). Requirements: destructor frees; copy constructor deep-copies; copy assignment (aim for copy-and-swap); move constructor steals **and nulls the source**; move assignment frees own data, steals, nulls, self-move-safe; `noexcept` where it is *true*; `explicit` where it belongs; a zero-initialized buffer; and an element accessor.

Then: a `main` exercising all five paths with predictions written as comments before running — including assignment over an *existing* buffer, a vector with reallocation, and `b = std::move(b)`.

Then the sabotage runs under AddressSanitizer (see "Experiments" below).

### Reference solution

<details>
<summary><strong>Show the solution — do the exercise cold first</strong></summary>

One file, as your attempt will be. In this repository the same code is now two: the class in `solutions/Buffer.h`, the demo `main()` in `solutions/buffer.cpp`. Nothing about the class changed — but a class sharing a translation unit with an entry point cannot be linked into a test binary, so [Chapter 28](28-testing.md#chapter-28--testing) has to split it before it can test it. Read this listing as one file; that split is a later chapter's problem.

```cpp
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <utility>
#include <vector>

class Buffer {
public:
    explicit Buffer(size_t size)
        : size_(size), data_(new int[size]{})   // {} => zero-initialized (Finding 7)
    {}

    ~Buffer() { delete[] data_; }               // delete[] on nullptr is a safe no-op

    // ---- copy: deep, exception-safe ---------------------------------------
    Buffer(const Buffer& other)
        : size_(other.size_), data_(new int[other.size_])
    {
        std::copy(other.data_, other.data_ + size_, data_);
    }

    Buffer& operator=(const Buffer& other) {    // copy-and-swap (Finding 6)
        Buffer tmp(other);   // ALL throwing work happens here; *this untouched
        Swap(tmp);           // three noexcept pointer exchanges
        return *this;
    }                        // tmp's destructor frees our OLD block

    // ---- move: steal and null out -----------------------------------------
    Buffer(Buffer&& other) noexcept
        : size_(std::exchange(other.size_, 0)),
          data_(std::exchange(other.data_, nullptr))
    {}

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {                   // self-move guard
            delete[] data_;                     // safe HERE: nothing below throws
            size_ = std::exchange(other.size_, 0);
            data_ = std::exchange(other.data_, nullptr);
        }
        return *this;
    }

    // ---- access -------------------------------------------------------------
    size_t Size() const noexcept { return size_; }

    int&       At(size_t i)       { assert(i < size_); return data_[i]; }  // Finding 8
    const int& At(size_t i) const { assert(i < size_); return data_[i]; }

    void Swap(Buffer& other) noexcept {
        std::swap(size_, other.size_);
        std::swap(data_, other.data_);
    }

private:
    size_t size_ = 0;
    int*   data_ = nullptr;
};

int main() {
    Buffer a(5);
    a.At(2) = 42;                    // writable now (Finding 8)

    Buffer b = a;                    // copy ctor: deep copy
    Buffer c(3);
    c = a;                           // copy assignment via copy-and-swap
    std::cout << "c[2]=" << c.At(2) << " (expect 42)\n";

    const Buffer& view = a;
    std::cout << "view[2]=" << view.At(2) << " (const overload)\n";

    Buffer d = std::move(a);         // move ctor: a is now empty husk
    std::cout << "a.Size()=" << a.Size() << " (expect 0)\n";

    c = std::move(d);                // move assignment
    c = std::move(c);                // self-move: must be harmless
    std::cout << "c[2]=" << c.At(2) << " after self-move (expect 42)\n";

    std::vector<Buffer> v;
    v.push_back(Buffer(2));
    v.push_back(Buffer(4));          // reallocation: moves (noexcept honest)
    std::cout << "vector ok, sizes " << v[0].Size() << "," << v[1].Size() << "\n";
    return 0;
}
```

Notes on the choices:

- **`new int[size]{}`** — the braces zero-initialize. Without them the contents are indeterminate and reading them is UB (Finding 7).
- **Copy assignment is copy-and-swap**: the copy constructor does all throwing work into `tmp` while `*this` is untouched; `Swap` is three noexcept pointer exchanges; `tmp`'s destructor frees the old block on the way out. Strong exception guarantee and self-assignment safety fall out for free (Finding 6).
- **`std::exchange(other.data_, nullptr)`** — "take the old value, leave this one" in a single expression; the idiomatic steal-and-null. Equivalent to two lines, harder to forget the second.
- **Move assignment deletes first and that is fine here** — nothing after the `delete[]` can throw. The same shape inside *copy* assignment is a bug (Finding 6): the difference is what follows the delete.
- **The const-overload pair for `At`** — `int& At(size_t)` plus `const int& At(size_t) const`. Same name; the compiler picks by the constness of the object. This is exactly how `vector::operator[]` works (Finding 8). The `assert` documents the bounds contract; throwing `std::out_of_range` would be the `.at()`-style alternative.
- **Destructor is one line** — `delete[]` on `nullptr` is a safe no-op, and nulling members in a destructor is dead work: the object ceases to exist in the next instant (Finding 9).
- The deliberate `c = std::move(c)` in `main` draws a compiler warning (`-Wself-move`) — good: the compiler flags suspicious code, and the class survives it anyway, which is the requirement.

</details>

### The unhappy path, step by step

The instructive bug from a real first attempt — release-before-acquire copy assignment:

```cpp
Buffer& operator=(const Buffer& other) {
    if (this != &other) {
        delete[] data_;              // old block GONE. data_ dangles.
        size_ = other.size_;
        data_ = new int[size_];      // <- suppose THIS throws bad_alloc
        std::copy(...);
    }
    return *this;
}
```

If the allocation throws, the exception propagates out mid-function and the object is a zombie: `data_` still points at freed memory. During stack unwinding its destructor runs — `delete[] data_` — **freeing the same block twice**. The assignment didn't just fail to copy; it corrupted the heap on the way out. The principle: **do all the throwing work before touching your own state.** Copy-and-swap makes the correct ordering structural rather than a discipline to remember.

### Experiments (sabotage runs under ASan)

Take a working copy, break one thing at a time, build with `-fsanitize=address -g`, run, and read each report until it makes sense:

1. **Remove the null-out in the move constructor** → double-free; ASan shows both freeing stacks.
2. **Make copy assignment shallow** (`data_ = other.data_;`) → double-free plus a leak of the orphaned old block.
3. **Remove the self-move guard**, run `b = std::move(b)` → reason first about whether your implementation gives use-after-free or silent data loss, then verify.
4. **Simulate the unhappy path**: revert to release-before-acquire and insert `throw std::bad_alloc{};` after the `delete[]` — watch Finding 6 detonate.

### Why you would never ship this class

`std::vector<int>` already is this class, written by experts, tested for decades — holding one as the member gives all five operations for free. (`std::unique_ptr<int[]>` is the other candidate, but it is move-only: as a member it hands you the destructor and the two moves and *deletes* your copies, so the deep-copy semantics this chapter just built would still be yours to write.) Rule of Zero beats Rule of Five (Chapter 6). Hand-rolling the five is for the rare type that *is* the resource wrapper — and knowing how is precisely what makes the shortcut safe to take everywhere else.

---


<!-- nav:begin -->
[← Chapter 14 — Exercise: The Lifetime Tracer](14-exercise-the-lifetime-tracer.md) · [Contents](README.md) · [Chapter 16 — The SDK Bestiary →](16-the-sdk-bestiary.md)
<!-- nav:end -->
