## Chapter 19 — Exercise: The Word Counter

*Trains: Chapter 10 (auto, lambdas, structured bindings), Chapter 11 (containers, algorithms, the map/vector dance). Time: ~60 min. Rule: cppreference only — this is the docs-navigation drill.*

### The task

Read a text file; count word frequencies case-insensitively, stripping punctuation from word edges; print the top 10 words that occur at least twice, sorted by count descending, ties broken alphabetically. That one sentence forces: file I/O, `unordered_map` counting, the map-to-vector transfer, a two-key sort lambda, filtering, and bounded output.

### Reference solution

<details>
<summary><strong>Show the solution — do the exercise cold first</strong></summary>

```cpp
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "usage: words <file>\n"; return 1; }

    std::ifstream in(argv[1]);                    // RAII: closes itself
    if (!in) { std::cerr << "cannot open " << argv[1] << "\n"; return 1; }

    std::unordered_map<std::string, int> freq;    // Dictionary<string,int>
    std::string word;
    while (in >> word) {
        // normalize: lowercase, strip non-alpha edges
        std::transform(word.begin(), word.end(), word.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        while (!word.empty() && !std::isalpha(static_cast<unsigned char>(word.front())))
            word.erase(word.begin());
        while (!word.empty() && !std::isalpha(static_cast<unsigned char>(word.back())))
            word.pop_back();
        if (!word.empty())
            ++freq[word];              // operator[] inserting IS wanted here
    }

    // map -> vector of pairs, because maps can't be sorted by value
    std::vector<std::pair<std::string, int>> ranked(freq.begin(), freq.end());
    std::sort(ranked.begin(), ranked.end(),
              [](const auto& a, const auto& b) {
                  if (a.second != b.second) return a.second > b.second;
                  return a.first < b.first;        // tie-break: alphabetical
              });

    std::erase_if(ranked, [](const auto& p) { return p.second < 2; });  // C++20
    // pre-C++20: ranked.erase(std::remove_if(...), ranked.end());

    const size_t top = std::min<size_t>(10, ranked.size());
    for (size_t i = 0; i < top; ++i)
        std::cout << ranked[i].first << ": " << ranked[i].second << "\n";
    return 0;
}
```

</details>

### What each part is really teaching

**`++freq[word]` — the one place the map's insert-on-read trap is a feature.** Chapter 11 warns that `operator[]` silently inserts a default value on a missing key. Counting is the idiom where that behavior is exactly what you want: first sight of a word inserts 0, then increments to 1. Knowing when a trap is a tool is the difference between rule-following and fluency.

**The map-to-vector dance.** You cannot sort a map by value — its ordering *is* its identity (`std::map` by key; `unordered_map` by nothing). The universal pattern: copy into a `vector<pair<K,V>>`, sort that. The range constructor `ranked(freq.begin(), freq.end())` does the transfer in one line.

**The two-key comparator.** `if (a.second != b.second) return a.second > b.second; return a.first < b.first;` — descending by count, ascending by word. A comparator must be a *strict weak ordering*; the classic bug is `return a.second >= b.second` (note `>=`), which violates it and produces undefined behavior inside `std::sort` — sometimes a crash, sometimes silently wrong order. If a sort ever crashes deep inside the standard library, audit the comparator first.

**`unsigned char` in the `tolower`/`isalpha` calls.** The `<cctype>` functions take an `int` that must be representable as `unsigned char`; passing a plain `char` that happens to be negative (any non-ASCII byte on signed-char platforms) is undefined behavior. The cast is not pedantry — it is the difference between working and UB the moment the input contains a name like "Müller". (The honest limitation: this solution treats bytes, not Unicode; real text pipelines need proper Unicode handling — Chapter 9's encoding discussion.)

**`std::erase_if` vs the erase-remove idiom.** The C++20 one-liner and its two-step ancestor from Chapter 11; the solution shows the modern form and names the classic in a comment because legacy codebases are full of it.

### Stretch goals

Rewrite the pipeline with C++20 ranges (`views::filter` + `views::take`); time the difference between `map` and `unordered_map` on a large file (then explain it via Chapter 11's cache-locality argument); make the minimum count and top-N command-line arguments with proper validation.

---


<!-- nav:begin -->
[← Chapter 18 — Exercise: The Device SDK](18-exercise-the-device-sdk.md) · [Contents](README.md) · [Chapter 20 — Exercise: Slicing and Polymorphism →](20-exercise-slicing-and-polymorphism.md)
<!-- nav:end -->
