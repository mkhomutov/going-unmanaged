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

**Task 1** — `v.erase(it)` invalidates `it`; the loop's `++it` then increments a dead iterator. Note what `erase` does *not* do: it frees nothing. It shifts the tail down and shrinks the size, leaving the allocation and its capacity exactly where they were — so there is no `heap-use-after-free` available here, because there has been no free. What ASan typically reports instead is `container-overflow`: a read past `end()` but still inside `begin() + capacity()`, which it can see only because the standard library annotates a vector's unused capacity under ASan — libc++ by default, libstdc++ only with `-D_GLIBCXX_SANITIZE_VECTOR`. The report's shape gives it away — an allocation stack and no "freed by" stack at all. Switch the annotations off (`ASAN_OPTIONS=detect_container_overflow=0`) and the same run reports something else or nothing at all, still producing wrong results: the Finding 10 lesson again, predict values, don't rely on the crash. Hold that against Task 3, where a *reallocating* `push_back` really does free the old block and `heap-use-after-free` is exactly the right report. The fix is the erase-returns-next idiom, or `std::erase_if` which encapsulates it.

**Task 2** — the subtle one, because it *sometimes works*. `push_back` invalidates *everything* only when capacity is exceeded — but it always invalidates the past-the-end iterator, and a range-`for` grabs `end()` once before its first iteration. So a run that stays within capacity merely *behaves*: it is still undefined, and once production data grows it detonates outright. Of the two halves of the fix, only one is actually a fix. Iterating by **index** against a **pre-captured size** is (indices survive reallocation; the captured size stops the loop chasing its own tail). `reserve` up front makes reallocation impossible and documents intent, but it cannot rescue the broken range-`for` — that cached `end()` is dead either way. The index loop alone suffices; the `reserve` on top reads as deliberate.

**Task 3** — references into a vector are exactly as fragile as iterators. `int& first = v[0];` is a pointer in disguise; after a reallocating `push_back` it dangles. ASan reports `heap-use-after-free` with the free stack inside vector's internals — your first encounter with a report whose "freed by" stack contains no code of yours. Reading it correctly ("the *container* freed the old block during growth") is the skill; the fix is re-acquiring after mutation, or reserving.

**The rule to leave with:** after any potentially-reallocating operation, treat every iterator, pointer, and reference into that vector as dead. Chapter 11's invalidation rules say which containers are gentler, and [Appendix H](H-choosing.md#appendix-h--choosing-signatures-containers-and-storage) turns them into a column you can choose from — including why `map`'s stable iterators are sometimes worth its slower lookups.

</details>

---


<!-- nav:begin -->
[← Chapter 20 — Exercise: Slicing and Polymorphism](20-exercise-slicing-and-polymorphism.md) · [Contents](README.md) · [Chapter 22 — Exercise: Lambda Lifetimes →](22-exercise-lambda-lifetimes.md)
<!-- nav:end -->
