## Chapter 25 — Findings from Practice: a Living Log

*A living log: weak spots discovered during hands-on exercises, each with the theory behind it, the broken and fixed code side by side, and the habit to build. New findings get appended as practice continues — this chapter is meant to grow.*

### Finding 1 — Copy-shaped moves: a move that doesn't steal is a copy with a misleading name

**Found in:** the Tracer exercise — move operations written as `name = "moved from " + t.name;`.

**The theory.** To understand moving, look at what a `std::string` physically is: roughly three fields — a pointer to heap-allocated characters, a size, and a capacity. "Moving a string" means something concrete about those fields:

```cpp
name = std::move(t.name);
// name's pointer  = t.name's pointer    <- steal the heap block
// t.name's pointer = nullptr (empty)    <- null out the source
```

Three pointer-sized assignments. No allocation, no character copying. That O(1) theft — versus O(n) duplication — is the *entire reason move semantics exist*.

One simplification to hold lightly: every mainstream implementation also keeps *short* strings inside the string object itself — the **small-string optimization**, capacity 15 on libstdc++ and MSVC, 22 on libc++ — and moving one of those copies its bytes rather than stealing a pointer. Still O(1), still no allocation. Pointer theft is the long-string picture, and long strings are the ones that cost you anything.

**The broken version, dissected:**

```cpp
Tracer(Tracer&& t) noexcept {
    name = "moved from " + t.name;   // looks like a move, is a copy
}
```

Step by step, `"moved from " + t.name`: (1) **builds a brand-new buffer** — on the heap as soon as the result outgrows the small-string buffer, which the Tracer's own short names do not, (2) copies the literal into it, (3) **copies every character of t.name** into it, (4) assigns the temporary to `name`. Meanwhile `t.name` is untouched — still owning its original characters, fully intact.

Compare with the definition of copying: allocate, duplicate characters, leave the source whole. **Identical.** This is a copy constructor that prints the word "move".

**Why it matters beyond pedantry.** Scale the member up: imagine the class held a vector of a million points. This pattern duplicates a million points on every vector reallocation and every `std::move` into a container — while *claiming* to be the cheap option. And the lie compounds through `noexcept` (see Finding 3): step (1) allocates the moment the result outgrows that small-string buffer, and allocation **can throw** — so the noexcept promise is false for any string big enough to matter.

**The fix — steal in the initializer list, print after:**

```cpp
Tracer(Tracer&& t) noexcept : name(std::move(t.name)) {
    std::cout << name << " move constructor\n";   // name, not t.name -
    t.name = "(husk)";                            // we already stole it!
}

Tracer& operator=(Tracer&& t) noexcept {
    std::cout << t.name << " move assignment\n";
    name = std::move(t.name);    // steal (string drops our old block itself)
    t.name = "(husk)";
    return *this;
}
```

Two details visible only in tracing code: in the move *constructor*, by the time the body runs, `t.name` is already moved-from — in practice empty on every mainstream implementation, though the standard promises only valid-but-unspecified (Finding 5) — so print your own `name`, which now holds the stolen value. And `t.name = "(husk)"` is tracing sugar only — it re-fills the source so destructor output shows which objects were gutted; real code leaves moved-from strings empty (and strictly, that assignment is not `noexcept` — a longer literal would allocate — so it slightly compromises noexcept purity: fine in a learning tracer, not in production).

**Habit:** a move operation's body should contain `std::move(member)` for every resource-holding member — and nothing that allocates. If you can't write it that way, question whether the operation is really a move.

### Finding 2 — Member initializer list vs assignment in the body — construction happens before the brace

**Found in:** the same Tracer — all four copy/move operations assigned `name` inside the body.

**The theory.** Members are **constructed before the constructor body runs** (Chapter 4). So this:

```cpp
Tracer(const Tracer& t) {          // <- name ALREADY exists here, empty
    name = "copy of " + t.name;    // step 2: assign over it
}
```

is a two-step dance: default-construct `name` as an empty string, then assign. The initializer-list form constructs it right the first time:

```cpp
Tracer(const Tracer& t) : name("copy of " + t.name) {   // one step
    std::cout << ...;
}
```

**Why build the reflex on easy cases:** for a string the waste is trivial — but for a `const` member or a reference member, the body-assignment version *does not compile at all* (they can only be initialized, never assigned). If the initializer list is already your default, those cases cost you nothing; if body-assignment is your default, they cost you a confused half-hour.

**Habit:** the constructor body is for logic (logging, validation, registration). Member *values* belong in the initializer list — and remember they initialize in declaration order, not list order.

### Finding 3 — noexcept is a promise, not a decoration

**Found in:** adding `noexcept` to a move that internally allocates.

**The theory.** `noexcept` tells callers — and especially `std::vector` — "this operation cannot throw." Vector uses it to choose its reallocation strategy: if your element's move constructor is noexcept, it *moves* elements to the new block; if not, it *copies* them, so that a mid-transfer exception can't leave the container half-destroyed. That is the behavior difference observed live in the Tracer output: `moved from v1 copy constructor` on reallocation before the keyword, a move after it.

**The trap discovered here is the reverse direction:** claiming noexcept on an operation that can throw. String concatenation allocates; allocation can throw `std::bad_alloc`. If an exception ever escapes a noexcept function, the program calls **`std::terminate`** — and whether the stack is unwound first, partly or at all, is left to the implementation, so you cannot count on your destructors running. So a false noexcept converts a recoverable out-of-memory into an instant process death, in the rare moment you least want it.

**The rule that makes both directions safe:** noexcept belongs on operations that genuinely just shuffle pointers — which real moves do (Finding 1). Write the move correctly and the promise is automatically true. The two findings are one finding: *steal, don't build — then noexcept is honest and vector moves your elements.*

**Habit:** `noexcept` on every move constructor and move assignment — and a body that justifies it.

### Finding 4 — Assignment must deal with what you already hold

**Found in:** Tracer's assignment operators — bodies identical to the constructors', which *happened* to be safe.

**The theory.** Construction and assignment differ in one crucial way: at assignment time, **the target already owns something**. `name = ...` was safe in Tracer only because `std::string::operator=` internally releases the old buffer before taking the new value — the string did the dangerous step invisibly.

With a raw resource, nothing is invisible:

```cpp
// Buffer holds: int* data_
Buffer& operator=(const Buffer& other) {
    data_ = new int[other.size_];        // BUG: the block data_ USED to
    ...                                  // point at is now leaked forever
}
```

Correct assignment must release-then-acquire — or better, sidestep the ordering problem entirely with **copy-and-swap** (Chapter 6): copy into a temporary, swap guts with it, let the temporary's destructor free your old resource. That also delivers the strong exception guarantee and self-assignment safety for free.

**Habit:** whenever writing `operator=`, ask first: "what happens to what I'm currently holding?" If the answer is "a member type handles it," fine — but know *which* member and *how*, because the day the member is a raw pointer, nobody handles it but you.

### Finding 5 — The moved-from state: valid but unspecified

**Found in:** the question "what state is `a` in?" after `Tracer c = std::move(a);`.

**The theory.** A moved-from object is **not destroyed and not invalid** — it lives until its scope ends and its destructor runs normally (the Tracer output showed every moved-from object still getting a destructor line). The standard's phrase for its state is *valid but unspecified*: it is a real object satisfying its class invariants, but you must not assume anything about its contents.

What is allowed on a moved-from object: destroy it, assign a new value to it, call operations with no preconditions (`clear()`, `empty()`). What is not: read its value expecting anything in particular.

```cpp
std::string s = "hello";
std::string t = std::move(s);
s.size();          // legal, but the value is unspecified - don't rely on it
s = "reborn";      // perfectly fine - assignment gives it a value again
```

One corner worth knowing: **self-move** (`a = std::move(a)`) must not corrupt the object — which is why the Buffer's move assignment carries `if (this != &other)`. Standard types survive self-move (left valid-but-unspecified); your types should too.

**Habit:** after `std::move(x)`, mentally mark `x` as a husk: assign or destroy, nothing else.

### Finding 6 — Release-before-acquire assignment: exception safety is an ordering problem

**Found in:** the Buffer exercise — copy assignment that deleted the old block, then allocated the new one.

**The theory.** Assignment differs from construction in one way: the target already owns something (Finding 4). The naive order — free mine, then acquire the new — has a hidden failure mode:

```cpp
delete[] data_;              // old block gone; data_ dangles
data_ = new int[size_];      // if THIS throws, the function exits here
```

The object is left holding a dangling pointer. Its destructor later runs during stack unwinding and frees the same block a second time — **an exception turned into heap corruption**. The bug is invisible in every test where allocation succeeds, which is all of them until production.

**The principle:** do all the work that can throw *before* modifying your own state. Two shapes deliver it:

```cpp
// allocate-first (minimal change):
int* fresh = new int[other.size_];                    // may throw - still intact
std::copy(other.data_, other.data_ + other.size_, fresh);
delete[] data_;                                       // point of no return
data_ = fresh;  size_ = other.size_;

// copy-and-swap (structural - can't get the order wrong):
Buffer tmp(other);   // throwing work in the copy ctor; *this untouched
Swap(tmp);           // noexcept pointer exchanges
                     // tmp's dtor frees the old block
```

**The contrast that seals the lesson:** *move* assignment legitimately deletes first — because nothing after its `delete[]` can throw. Same first line, opposite verdict. Exception safety is not about which operations you call; it is about **what state you are in if any given line throws**.

**Habit:** in any assignment operator, find the first line that modifies `*this` and ask: "can anything after this line throw?" If yes, reorder.

### Finding 7 — `new T[n]` does not zero: indeterminate values are UB to read

**Found in:** the Buffer constructor — `data_(new int[size])`.

**The theory.** `new int[size]` default-initializes the elements, and for built-in types default-initialization does *nothing*: the memory holds whatever bytes were there. Reading an element before writing it is undefined behavior of the quiet kind — on Linux and macOS it often prints 0 from pages the OS handed over zeroed, and garbage once that memory has been reused; MSVC's debug heap fills fresh allocations with `0xcd` instead, so you read `-842150451` there (Chapter 3's fill-pattern story, one allocator along). The Chapter 3 signature: works on my machine.

```cpp
data_(new int[size])     // indeterminate contents
data_(new int[size]{})   // value-initialized: all zeros. One pair of braces.
```

C# contrast worth noting: `new int[5]` in C# is always zeroed — the runtime guarantees it. C++ makes zeroing opt-in because it costs a memset and C++'s contract is "don't pay for what you don't use."

**Habit:** every `new T[n]` gets `{}` unless a measured reason says otherwise — and in real code, prefer `std::vector` which value-initializes anyway.

### Finding 8 — Accessors: return by reference, and provide the const-overload pair

**Found in:** the Buffer — `int At(size_t) const` returning a copy, making the buffer write-only through its own API.

**The theory.** Returning by value hands out a copy; `buf.At(2) = 7` does not compile at all — assignment needs a modifiable lvalue, and a returned `int` is a prvalue. Containers hand out **references** to their elements — and because a single `const` reference-returning accessor would hand out a mutable reference from a const object, the idiom is the pair.

Do not expect the compiler to stop you writing that single accessor, either. `const` on a member function is **shallow**: through a raw `int* data_` it makes the pointer `int* const`, not `const int*`, so `int& At(size_t i) const { return data_[i]; }` compiles happily and lets a caller mutate a const `Buffer`. (Swap in a `std::vector` member and it *does* become a compile error — which is one more argument for the container.) The pair below is a design rule you enforce, not one the language enforces for you.

```cpp
int&       At(size_t i)       { assert(i < size_); return data_[i]; }
const int& At(size_t i) const { assert(i < size_); return data_[i]; }
```

Same name; overload resolution picks by the constness of the object: mutable buffer gets the writable reference, `const Buffer&` gets the read-only one. This is precisely how `std::vector::operator[]` is declared — the trailing-const material of Appendix A.5 made practical.

Also decide the bounds contract explicitly: `assert` (documented precondition, free in Release) or throw `std::out_of_range` (the `.at()` convention). Unchecked-and-undocumented is the only wrong answer.

**Habit:** when writing any container-like accessor, write both overloads in one motion — needing the second is the rule, not the exception.

### Finding 9 — Destructors: no null checks, no dead stores

**Found in:** the Buffer destructor — `if (data_) delete[] data_; data_ = nullptr; size_ = 0;`.

**The theory.** Three small misunderstandings in one function. `delete`/`delete[]` on a null pointer is a guaranteed safe no-op — the check is redundant. And assigning to members in a destructor is dead work: the object ceases to exist the instant the destructor returns; no code can legally observe those stores. (Compilers routinely eliminate them, confirming their meaninglessness.)

```cpp
~Buffer() { delete[] data_; }    // complete and correct
```

Beyond style, the busywork signals a mental model worth correcting: nulling members "for safety" in a destructor implies the object might be touched afterward — and if anything *does* touch it afterward, that is use-after-destruction UB which no amount of member-nulling makes safe. The safety comes from ownership discipline (Chapter 1), not from defensive stores.

**Habit:** a resource-owning destructor is one line per resource. If it is longer, ask what the extra lines think they are protecting against.

### Finding 10 — A clean sanitizer run is not a correctness proof

**Found in:** the Buffer sabotage experiments — removing the self-move guard produced "no issues," which was itself the bug.

**The theory.** Walk the guardless move assignment through `c = std::move(c)`. First the shape this book's Buffer actually uses:

```cpp
delete[] data_;                              // frees c's block
size_ = std::exchange(other.size_, 0);       // other IS c: exchange writes 0
                                             // and RETURNS 5 — which the outer
                                             // assignment then writes back
data_ = std::exchange(other.data_, nullptr); // same trick: data_ ends up
                                             // holding the pointer just freed
```

`x = std::exchange(x, v)` is an elaborate way of leaving `x` alone: `exchange` stores `v` and hands back the old value, and the outer assignment puts that old value straight back. So `c` keeps `size_ == 5` and the block it has already freed. This one is *loud* — the destructor deletes that block a second time and AddressSanitizer reports `attempting double-free`, and touching `c.At(2)` before then gets you a `heap-use-after-free`. Loud is the good case.

The dangerous version is the one next door — the two-statement steal, which is what most people write before they meet `std::exchange`:

```cpp
delete[] data_;                              // frees c's block
data_ = other.data_;  other.data_ = nullptr; // other IS c: null out the very
size_ = other.size_;  other.size_ = 0;       // member we just assigned from
```

Now `c` really does end up an empty, *valid-looking* buffer — `size_` 0, `data_` null. The destructor later does `delete[] nullptr`, a safe no-op. No invalid access ever occurred, so AddressSanitizer has nothing to say — and the data is silently gone. Same bug, same missing guard; one shape aborts with a three-stack report and the other exits 0.

**The principle:** sanitizers catch *memory crimes* — invalid reads and writes, double-frees, leaks. They cannot catch *memory-clean but logically wrong* behavior. The two verification tools are complementary and neither substitutes for the other:

- **ASan answers:** "did this program touch memory it shouldn't?"
- **Predictions and assertions answer:** "did this program produce the values it should?"

```cpp
c = std::move(c);
assert(c.Size() == 5 && c.At(2) == 42);   // fires on the SILENT variant,
                                          // which ASan runs straight past
```

A related mechanical lesson from the same session: **ASan halts at the first error by default**, and leak detection runs at normal program exit — so a double-free report can mask a leak that would have been reported later. To see subsequent errors, either remove the first crime from the run, or use report-and-continue — the full recipe is compile with `-fsanitize-recover=address` *and* run with `ASAN_OPTIONS=halt_on_error=0`. (Allocator-level crimes like a double-free continue under the runtime flag alone; instrumented reads and writes — a use-after-free among them — stay fatal without the build flag, and a reader who tries the flag by itself will conclude it is broken.) And note the report-shape asymmetry: a double-free carries three stacks (access, prior free, allocation); a leak carries exactly one — the allocation — because an orphaned block's birth is the only trace it ever leaves.

One platform caveat belongs here more than anywhere else in the book, because it is this same Finding wearing different clothes: **LeakSanitizer is not available on macOS/arm64.** Ask for it and it tells you so — `detect_leaks is not supported on this platform` — which means that on Apple silicon a leaking program under ASan reports nothing whatsoever, and the run comes back clean for a reason that has nothing to do with your code. Everything above about leaks assumes a platform that detects them; Linux does, and that is what this repository's CI runs. On a Mac, read "no leak report" as *no information*, and get leak coverage from CI or a Linux container before believing a program does not leak.

**Habit:** every experiment and every test states its expected *values*, not just "doesn't crash." The question "what should this print?" outranks "did ASan complain?" — and a run that surprises you by being clean deserves as much scrutiny as one that fails.

### Finding 11 — The Tracer as a permanent diagnostic tool

The exercise that surfaced Findings 1–5 is worth keeping as a reusable instrument: a class that prints from all five of its special member functions, plus the constructor that names it, makes the invisible visible. Drop a Tracer into any container, function signature, or algorithm and the output tells you exactly what the compiler chose to do. Findings it demonstrated on first run:

| Observation in output | Concept confirmed |
|---|---|
| `Tracer d = MakeTracer()` printed ONE constructor, no copy, no move | RVO / copy elision — returning by value is free |
| `ByRef(b)` printed *nothing* | `const T&` binds directly: no construction — why it's the default parameter idiom |
| `ByValue(std::move(b))` printed a move, not a copy | std::move selects the move overload; the parameter steals |
| Second `push_back` printed activity for v1 too | reallocation transfers existing elements to the new block |
| That transfer was a COPY until the move ctor was noexcept | vector's exception-safety rule (Finding 3) |
| Every constructor line paired with exactly one destructor line | RAII bookkeeping balances — no leaks, no doubles |
| Destructors ran in reverse construction order per scope | stack unwinding order |

**Habit:** when container or call behavior is mysterious at work, reach for the Tracer before reaching for theory. Ten lines of printing beat an hour of guessing — and it requires no tools a locked-down work machine lacks.

---

---


<!-- nav:begin -->
[← Chapter 24 — Practice Plan](24-practice-plan.md) · [Contents](README.md) · [Chapter 26 — Build Systems and CMake →](26-build-systems-and-cmake.md)
<!-- nav:end -->
