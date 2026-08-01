## Chapter 21 — Exercise: Iterator Invalidation

*Trains: Chapter 11 (the trap section), Chapter 3 (UB). Time: ~45 min. Format: three broken loops to fix — write predictions, run broken versions under ASan, then fix.*

### The three tasks

1. Remove all odd numbers from a `vector<int>` while iterating.
2. Append a copy of every element to the same vector while iterating it.
3. Hold a reference to `v[0]`, `push_back` once, then use the reference.

Write each the naive way first, predict the failure mode, run under ASan, and only then fix.

### Reference solutions and the reasoning

<details>
<summary><strong>Show the solutions — do the tasks cold first</strong></summary>

```cpp
// Iterator invalidation lab - the FIXED patterns (broken ones live in comments).
#include <iostream>
#include <numeric>
#include <vector>

int main() {
    std::vector<int> v(10);
    std::iota(v.begin(), v.end(), 0);              // 0..9

    // Task 1: remove odd numbers while iterating.
    // BROKEN: for (auto it=v.begin(); it!=v.end(); ++it) if (*it%2) v.erase(it);
    for (auto it = v.begin(); it != v.end(); ) {
        if (*it % 2) it = v.erase(it);             // erase returns next valid
        else         ++it;
    }
    // or simply: std::erase_if(v, [](int x){ return x % 2; });

    // Task 2: append while iterating - by INDEX against a captured size,
    // because push_back may reallocate and kill every iterator/reference.
    // BROKEN: for (int x : v) v.push_back(x);     // UB on reallocation
    const size_t n = v.size();
    v.reserve(v.size() * 2);                       // belt AND suspenders
    for (size_t i = 0; i < n; ++i) v.push_back(v[i]);

    // Task 3: the reference that dies. BROKEN version:
    //   int& first = v[0]; v.push_back(99); std::cout << first;  // maybe UB
    // FIXED: re-acquire after any potentially-reallocating call, or reserve.

    for (int x : v) std::cout << x << ' ';
    std::cout << "\n";
    return 0;
}
```

**Task 1** — `v.erase(it)` invalidates `it`; the loop's `++it` then increments a dead iterator. ASan typically reports `heap-use-after-free` *if* the erase triggered internal movement, but may also stay silent while producing wrong results — the Finding 10 lesson again: predict values, don't rely on the crash. The fix is the erase-returns-next idiom, or `std::erase_if` which encapsulates it.

**Task 2** — the subtle one, because it *sometimes works*. `push_back` invalidates everything only when capacity is exceeded; a run that happens to stay within capacity behaves, then production data grows and it detonates. The fix has two independent parts: iterate by **index** against a **pre-captured size** (indices survive reallocation; the captured size stops the loop from chasing its own tail), and `reserve` up front (which makes reallocation impossible *and* documents intent). Either alone suffices; both together read as deliberate.

**Task 3** — references into a vector are exactly as fragile as iterators. `int& first = v[0];` is a pointer in disguise; after a reallocating `push_back` it dangles. ASan reports `heap-use-after-free` with the free stack inside vector's internals — your first encounter with a report whose "freed by" stack contains no code of yours. Reading it correctly ("the *container* freed the old block during growth") is the skill; the fix is re-acquiring after mutation, or reserving.

**The rule to leave with:** after any potentially-reallocating operation, treat every iterator, pointer, and reference into that vector as dead. The invalidation table in Chapter 11 says which containers are gentler — and why `map`'s stable iterators are sometimes worth its slower lookups.

</details>

---


<!-- nav:begin -->
[← Chapter 20 — Exercise: Slicing and Polymorphism](20-exercise-slicing-and-polymorphism.md) · [Contents](README.md) · [Chapter 22 — Exercise: Lambda Lifetimes →](22-exercise-lambda-lifetimes.md)
<!-- nav:end -->
