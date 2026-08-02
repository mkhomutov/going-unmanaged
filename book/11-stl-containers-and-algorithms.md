## Chapter 11 — STL Containers, Algorithms, and Iterator Invalidation

### The container map

| C# | C++ | Notes |
|---|---|---|
| `List<T>` | `std::vector<T>` | your default, 95% of the time |
| `Dictionary<K,V>` | `std::unordered_map<K,V>` | hash table, O(1) |
| `SortedDictionary<K,V>` | `std::map<K,V>` | tree, O(log n), sorted iteration |
| `HashSet<T>` | `std::unordered_set<T>` | |
| `Queue<T>` / `Stack<T>` | `std::queue` / `std::stack` | |
| `LinkedList<T>` | `std::list<T>` | almost never the right choice |
| `T[]` | `std::vector<T>`, or `std::array<T, N>` when N is fixed | a C# `T[]` sizes itself at runtime, so vector is the general match; `array` needs N at compile time and stores its elements inline — no heap block of its own |

> [!WARNING]
> **Gotcha:** plain `std::map` is the TREE (sorted, O(log n)); the Dictionary equivalent is `unordered_map`. "I'd use unordered_map for lookups unless I need sorted order."

### Why vector dominates — cache locality

A vector is one contiguous memory block. Even where a list is theoretically better, vector usually wins because CPUs prefetch contiguous memory. `push_back` is amortized O(1): when capacity runs out, the vector **reallocates** (typically doubling) and moves everything to a new block — remember that reallocation, it matters below.

### Key operations

```cpp
std::vector<Widget> v;
v.push_back(w);
v.emplace_back("name", 5);   // construct in place - prefer it
v[3];                        // no bounds check (fast, UB if out of range)
v.at(3);                     // bounds-checked, throws - like C# indexer

std::unordered_map<std::string, Widget> m;
m["wall"] = w;               // insert or overwrite
// TRAP: reading with [] INSERTS a default value if key missing!
auto it = m.find("wall");    // the safe lookup
if (it != m.end()) Use(it->second);   // ->first key, ->second value
if (m.contains("wall")) ...  // C++20, like ContainsKey
```

### Iterators

An iterator is a generalized pointer: `begin()` points at the first element, `end()` points **one past the last** (a sentinel — never dereference it). The half-open range [begin, end) is the universal STL currency.

```cpp
auto it = std::find(v.begin(), v.end(), target);
if (it != v.end()) {   // "not found" == end, the idiom
    Use(*it);
}
```

### Algorithms — LINQ's rough equivalent

| LINQ | STL (`<algorithm>` / `<numeric>`) |
|---|---|
| FirstOrDefault(pred) | `std::find_if(begin, end, pred)` — returns iterator |
| Count(pred) | `std::count_if(begin, end, pred)` |
| Any / All | `std::any_of / all_of / none_of` |
| OrderBy | `std::sort(begin, end, cmp)` — but IN PLACE, and *unstable*: `std::stable_sort` is the real match |
| Select | `std::transform(begin, end, std::back_inserter(out), func)` |
| Aggregate / Sum | `std::accumulate(begin, end, 0)` |
| Max | `std::max_element` — returns ITERATOR to max |

Mindset shifts: STL algorithms mutate in place (LINQ returns new lazy sequences), and "Where" is awkward pre-C++20 — the classic filter is the famous **erase-remove idiom**:

```cpp
// remove all small widgets - the pre-C++20 incantation:
v.erase(std::remove_if(v.begin(), v.end(),
        [](const Widget& w){ return w.size < 10; }),
        v.end());
// remove_if only SHIFTS survivors forward and returns the new logical
// end; erase then chops the garbage tail. Two steps, always paired.

std::erase_if(v, [](const Widget& w){ return w.size < 10; });  // C++20
```

### THE trap: iterator invalidation

The C# equivalent — modifying a collection during foreach — throws immediately. C++ gives you **undefined behavior**: maybe a crash, maybe silent corruption, maybe it works on your machine and dies in production.

```cpp
// BUG - the classic:
for (auto it = v.begin(); it != v.end(); ++it) {
    if (it->size < 10)
        v.erase(it);       // 'it' is now INVALID; ++it next loop is UB
}

// FIX - erase returns the next valid iterator:
for (auto it = v.begin(); it != v.end(); /* nothing */) {
    if (it->size < 10)
        it = v.erase(it);  // step forward via the return value
    else
        ++it;
}
// or better: std::erase_if(v, pred);
```

Worse — **push_back can invalidate everything too**: if the vector grows, the whole block moves, and every iterator, pointer, and reference into it dangles. Appending while iterating is UB even though you deleted nothing.

Invalidation rules to memorize: **vector** — insert/erase invalidates iterators at/after the point, and ALL of them if reallocation happens. **unordered_map** — insertion can invalidate iterators (rehash) but references survive; erase kills only the erased. **map/list** — iterators stable except the erased element.

### In the wild: C-style SDKs

Vendor container libraries (Qt, Unreal, and countless in-house ones) mirror the STL: same concepts, same invalidation logic, different spelling. C APIs additionally return dynamic arrays via pointer+count pairs or opaque handles (Chapter 17's payload pattern), which you often immediately wrap or copy into proper containers so the rest of your code lives in STL-land.

---

---


<!-- nav:begin -->
[← Chapter 10 — Modern C++ Fluency](10-modern-cpp-fluency.md) · [Contents](README.md) · [Chapter 12 — The Compilation Model →](12-the-compilation-model.md)
<!-- nav:end -->
