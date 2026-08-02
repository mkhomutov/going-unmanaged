## Chapter 6 — The Rule of Five and Move Semantics

The heart of resource-owning classes — and the explanation of what `std::move` actually does.

### Setup: the special member functions

The compiler auto-generates up to five functions for every class. The generated versions copy/move each member field:

```cpp
class Buffer {
public:
    ~Buffer();                          // 1. destructor
    Buffer(const Buffer&);              // 2. copy constructor
    Buffer& operator=(const Buffer&);   // 3. copy assignment
    Buffer(Buffer&&);                   // 4. move constructor   (C++11)
    Buffer& operator=(Buffer&&);        // 5. move assignment    (C++11)
};
```

### The Rule of Zero (the modern ideal)

**Write none of the five.** Compose your class out of members that manage themselves (std::string, std::vector, unique_ptr) and the compiler-generated versions are automatically correct:

```cpp
class Document {
    std::string title_;
    std::vector<Page> pages_;
    std::unique_ptr<Renderer> renderer_;
    // NOTHING to write. Copy, move, destruction - all correct for free.
    // (copy is deleted because of unique_ptr - which is correct too)
};
```

### The Rule of Five

**If you must write any one of the five** (usually because you hold a raw resource), **you almost certainly need to write — or explicitly delete — all five.** Destructor without copy operations = the double-free bug. Copy without move = silent performance loss everywhere.

### What "move" actually means

Copying a resource-owning object duplicates the resource — expensive. Moving means **stealing**: the new object takes the guts (the pointer), the old object is left empty but valid.

```cpp
std::vector<int> a = MakeMillion();
std::vector<int> b = a;             // COPY: allocate + copy 1M ints
std::vector<int> c = std::move(a);  // MOVE: c takes a's pointer.
                                    // ~3 pointer assignments. a now empty.
```

> [!NOTE]
> **The big reveal:** std::move moves nothing. It is just a cast — it marks a value as "you may steal from this", making the compiler select the move overload instead of the copy one. The stealing happens inside the move constructor. After moving from a variable, don't use it except to assign or destroy it.

### Rvalue references — the && syntax

`Buffer&&` means "reference to something I'm allowed to steal from": a temporary, or something explicitly marked with `std::move`.

```cpp
void Take(const Buffer& b);   // called for normal variables (copy)
void Take(Buffer&& b);        // called for temporaries / std::move'd things

Buffer x;
Take(x);              // first overload
Take(std::move(x));   // second - you granted permission to steal
Take(MakeBuffer());   // second - temporaries are fair game automatically
```

That last line is the one to keep: a function's result is a temporary, so it selects the stealing overload on its own — you never write `std::move` around a call, and nothing is copied into `Take`.

Returning containers by value is cheap for a *separate* reason, and the two are worth holding apart because they are routinely conflated. Since C++17, a function that returns a temporary outright — `return Buffer(n);` — gets **mandatory** elision: the object is built directly in the caller's storage, and no copy or move constructor is called or even required to exist. Return a *named* local instead and you are back to the optional flavour, NRVO, with an implicit move as the fallback (Chapter 14 watches both happen).

### The canonical exercise: Rule of Five for a raw buffer

Learn this shape cold:

```cpp
class Buffer {
    size_t size_ = 0;
    int*   data_ = nullptr;

public:
    explicit Buffer(size_t size)
        : size_(size), data_(new int[size]{}) {}

    // 1. Destructor
    ~Buffer() { delete[] data_; }

    // 2. Copy constructor - deep copy
    Buffer(const Buffer& other)
        : size_(other.size_), data_(new int[other.size_])
    {
        std::copy(other.data_, other.data_ + size_, data_);
    }

    // 3. Copy assignment - copy-and-swap idiom
    Buffer& operator=(const Buffer& other) {
        Buffer tmp(other);   // deep copy (may throw - fine, we're untouched)
        swap(tmp);           // steal tmp's guts
        return *this;
    }                        // tmp's destructor frees OUR old data

    // 4. Move constructor - steal and null out
    Buffer(Buffer&& other) noexcept
        : size_(other.size_), data_(other.data_)
    {
        other.size_ = 0;
        other.data_ = nullptr;  // CRITICAL: or its destructor frees OUR data
    }

    // 5. Move assignment
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            delete[] data_;           // free what we hold
            size_ = other.size_;
            data_ = other.data_;      // steal
            other.size_ = 0;
            other.data_ = nullptr;    // leave source empty-but-valid
        }
        return *this;
    }

private:
    void swap(Buffer& other) noexcept {
        std::swap(size_, other.size_);
        std::swap(data_, other.data_);
    }
};
```

### Details that separate working code from correct code

- **Nulling out the source in moves.** Forget it and the moved-from object's destructor deletes the data you just stole — double-free. The #1 bug in first attempts.
- **noexcept on move operations.** Not decoration: std::vector checks it. When reallocating, vector only moves your elements if the move can't throw — otherwise it falls back to copying for exception-safety. Omit noexcept and your type silently copies inside vectors.
- **Self-assignment check** (`if (this != &other)`) in move assignment — `a = std::move(a)` shouldn't destroy the data.
- **Copy-and-swap** for copy assignment: copy into a temp, then swap. If allocation throws, your object is untouched (the strong exception guarantee), and self-assignment is handled for free.

> [!TIP]
> **The stance to hold:** "In real code I'd never write this class — I'd hold `std::vector<int>` or `unique_ptr<int[]>` and get all five for free. Rule of Zero beats Rule of Five." Hand-rolling the five is a last resort; knowing how is what makes the shortcut safe.

### Where moves matter in daily code

```cpp
std::vector<Buffer> buffers;
buffers.push_back(std::move(myBuffer));  // move into container, no copy

widget.SetName(std::move(longString));   // sink params take by value + move

std::unique_ptr<Shape> s = std::make_unique<Circle>();
shapes.push_back(std::move(s));          // unique_ptr can ONLY move - this
                                         // is how ownership transfer is spelled
```

### In the wild: C-style SDKs

Large C++ SDKs often ship their own unique_ptr analog (an "Owner" or "ScopedRef" type) with the same move-only behavior. Any RAII guard you write around SDK handles is exactly the "class holding a raw resource" case — either delete copy/move entirely (simplest, as in Chapter 1's guard), or implement moves properly when guards must be stored in containers or returned from factories (as Chapter 18's DeviceSession does — with a subtle twist worth meeting there).

---

---


<!-- nav:begin -->
[← Chapter 5 — Virtual Dispatch and the Virtual Destructor](05-virtual-dispatch-and-the-virtual-destructor.md) · [Contents](README.md) · [Chapter 7 — Templates vs C# Generics →](07-templates-vs-csharp-generics.md)
<!-- nav:end -->
