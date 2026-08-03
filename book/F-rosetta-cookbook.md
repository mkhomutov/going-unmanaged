## Appendix F — The Rosetta Cookbook

Parts I–VI teach the language. This appendix serves a different moment:
mid-task, the C# name already in your head, fifteen seconds to spend. Find
the thing you are reaching for in the left column; the recipe gives the C++
spelling, says why it looks that way, and names the trap that costs an
afternoon. The *why* paragraphs cross-reference the chapter that owns each
concept rather than re-teaching it — this page is for looking up, not for
reading.

Every listing compiles, runs, and holds under the canonical flags: the
recipes live as code in `exercises/cookbook/`, and `scripts/build_all.sh`
asserts what each one claims on every push. Recipe numbers are stable —
recipes append and are never renumbered — so a note that says "Recipe 7"
stays right.

| Reaching for... | The recipe |
|---|---|
| `File.ReadAllText` | [Recipe 1 — Read a whole file into a string](#recipe-1--read-a-whole-file-into-a-string) |
| `string.Split` | [Recipe 2 — Split a string](#recipe-2--split-a-string) |
| `string.Join` | [Recipe 3 — Join strings](#recipe-3--join-strings) |
| `StringBuilder` | [Recipe 4 — Build a string in a loop](#recipe-4--build-a-string-in-a-loop) |
| `string.Format` / `$"..."` | [Recipe 5 — Format values into a string](#recipe-5--format-values-into-a-string) |
| `Stopwatch` | [Recipe 6 — Time a call](#recipe-6--time-a-call) |
| `using` / `IDisposable` | [Recipe 7 — Wrap a C handle so it frees itself](#recipe-7--wrap-a-c-handle-so-it-frees-itself) |
| `TryGetValue` | [Recipe 8 — Look up a key without inserting it](#recipe-8--look-up-a-key-without-inserting-it) |
| LINQ | the collections index predates this page: [the LINQ table of Chapter 11](11-stl-containers-and-algorithms.md#chapter-11--stl-containers-algorithms-and-iterator-invalidation) |

### Recipe 1 — Read a whole file into a string

**In C#:** `var text = File.ReadAllText(path);`

**The recipe:**

```cpp
std::string read_all_text(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open: " + path);
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();    // one streamed read; no line loop to get wrong
    return buffer.str();
}
```

**Why it looks like this.** There is no `File` static class: the stream
object *is* the open file, and
[Chapter 1](01-ownership-and-raii.md#chapter-1--ownership-and-raii) already
taught you what that buys — the `ifstream` closes itself on every path out of
the function, the throw included. `rdbuf()` hands the whole file to the
string stream in one operation, the closest thing iostreams have to a
one-liner. And `std::ios::binary` reads the bytes as they are; without it,
Windows translates `\r\n` on the way through and the "same" file compares
differently per platform. Needs `<fstream>`, `<sstream>`, `<stdexcept>`.

> [!WARNING]
> **Trap:** a stream that failed to open does not throw — every read on it quietly produces nothing, so without the `if (!in)` check a missing file becomes an empty string and no error. That check is the part `File.ReadAllText` did for you.

### Recipe 2 — Split a string

**In C#:** `var parts = text.Split(',');`

**The recipe:**

```cpp
std::vector<std::string> split(const std::string& text, char sep) {
    std::vector<std::string> parts;
    std::istringstream stream(text);
    std::string field;
    while (std::getline(stream, field, sep)) {
        parts.push_back(field);
    }
    return parts;
}
```

**Why it looks like this.** `std::string` ships no `Split`, and this loop is
the idiom the ecosystem converged on: `getline`'s third argument makes any
character the "line" ending, so the same function that reads lines from a
file reads fields from a string stream. It keeps interior empty fields
(`"a,,b"` gives three), which is what field-shaped data needs. The other
spelling you will meet, `stream >> word`, splits on runs of *any* whitespace
and drops empties — right for words, wrong for columns; choose the one you
mean. Needs `<sstream>`, `<vector>`.

> [!WARNING]
> **Trap:** `"a,b,"` splits into two fields here where C# gives three — `getline` never reports a field after the final separator, so a trailing delimiter is invisible; if the column count matters, validate it.

### Recipe 3 — Join strings

**In C#:** `var line = string.Join(", ", parts);`

**The recipe:**

```cpp
std::string join(const std::vector<std::string>& parts, const std::string& sep) {
    std::string result;
    for (const auto& part : parts) {
        if (!result.empty()) {
            result += sep;    // between elements only - never leading
        }
        result += part;
    }
    return result;
}
```

**Why it looks like this.** The guard clause is the whole trick: append the
separator only once something is already there, and the fencepost problem
never starts. Ten lines for what C# does in one feels like a step down —
until you notice they are the same ten lines every time. Write it once per
codebase; most codebases already have, so grep before adding yours.

> [!WARNING]
> **Trap:** `result += sep + part;` builds and destroys a temporary string every pass — the two `+=` lines append in place and say the same thing (Recipe 4 is the why).

### Recipe 4 — Build a string in a loop

**In C#:** `var sb = new StringBuilder(); sb.Append(...);`

**The recipe:**

```cpp
std::string build_report(const std::vector<int>& values) {
    std::string out;
    // one allocation up front - the StringBuilder(capacity) constructor
    out.reserve(values.size() * 12);
    for (int value : values) {
        out += "value=";
        out += std::to_string(value);
        out += '\n';
    }
    return out;
}
```

**Why it looks like this.** `std::string` *is* the string builder.
`StringBuilder` exists because C# strings are immutable, so `+=` there
re-creates the whole string every pass; here the string is your own mutable
buffer ([Chapter 2](02-value-semantics.md#chapter-2--value-semantics)'s value
semantics), `+=` appends in place with amortized growth, and `reserve` plays
the capacity constructor. This is the rare reflex to unlearn outright: the
class C# taught you to avoid in a loop is the right default in C++.

> [!WARNING]
> **Trap:** the C# tax comes back if you write `out = out + piece` — the assignment form re-creates the string every pass in any language; the appender is `+=` (or `.append()`).

### Recipe 5 — Format values into a string

**In C#:** `var s = $"{count} samples, ratio {ratio:F2}";`

**The recipe:**

```cpp
std::string describe(int count, double ratio) {
    std::ostringstream out;
    out << count << " samples, ratio "
        << std::fixed << std::setprecision(2) << ratio;
    return out.str();
}

std::string describe_c(int count, double ratio) {
    char buffer[64];
    std::snprintf(buffer, sizeof buffer, "%d samples, ratio %.2f", count, ratio);
    return buffer;
}
```

**Why it looks like this.** The honest answer: C++17 has no interpolation.
`std::format`, the true analogue, arrives in C++20 — and toolchains around
SDK work are exactly the ones that lag. Until yours has it, these are the two
dialects: the stream when you want the compiler checking types
(`std::fixed << std::setprecision(2)` is `:F2` spelled as stream state), and
`snprintf` when printf specifiers are already the local language — around C
SDKs they are, because that is how the C world logs, asserts, and documents
([Chapter 16](16-the-sdk-bestiary.md#chapter-16--the-sdk-bestiary)'s shapes
speak it natively). Needs `<sstream>` and `<iomanip>`, or `<cstdio>`.

> [!WARNING]
> **Trap:** `std::snprintf` never overruns, but it truncates silently — the return value is the length it *wanted* to write, and comparing that against the buffer size is the only way to notice the cut.

### Recipe 6 — Time a call

**In C#:** `var sw = Stopwatch.StartNew(); ... sw.ElapsedMilliseconds`

**The recipe:**

```cpp
void report_batch_time() {
    const auto start = std::chrono::steady_clock::now();
    run_the_batch();    // the code being timed
    const auto elapsed = std::chrono::steady_clock::now() - start;
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    std::cout << ms.count() << " ms\n";
}
```

**Why it looks like this.** A stopwatch is two time points and a subtraction;
`steady_clock` is the monotonic clock, which is what `Stopwatch` was
underneath all along. The subtraction gives a typed `duration` rather than a
bare number, and `duration_cast` makes the unit visible at the call site —
where `ElapsedMilliseconds` buried it in a property name, here you choose it,
and `<chrono>` will not let you mix units by accident. Needs `<chrono>`.

> [!WARNING]
> **Trap:** `system_clock` is the wall clock — NTP or the user can move it mid-measurement, backwards included. Intervals come from `steady_clock`; `system_clock` is for timestamps only.

### Recipe 7 — Wrap a C handle so it frees itself

**In C#:** `using var file = File.OpenWrite(path);`

**The recipe:**

```cpp
using FileHandle = std::unique_ptr<std::FILE, int (*)(std::FILE*)>;

FileHandle open_file(const char* path, const char* mode) {
    return FileHandle(std::fopen(path, mode), &std::fclose);
}
```

**Why it looks like this.** The most load-bearing three lines of the
transition: the deleter is part of the pointer's *type*, so destruction calls
`fclose` exactly once on every exit path — `using`, without needing a block.
`FILE*` here stands for every handle a C API ever hands you: substitute the
SDK's create/destroy pair and the recipe is unchanged
([Chapter 16](16-the-sdk-bestiary.md#chapter-16--the-sdk-bestiary)'s Bestiary
is a catalogue of exactly such pairs). `unique_ptr` never calls the deleter
on null, so a failed `fopen` needs no special-casing — test the handle, like
the C API taught you. When there is more to manage than one close — a
callback registration, a paired init/deinit with state — graduate to the
wrapper class of
[Chapter 18](18-exercise-the-device-sdk.md#chapter-18--exercise-the-device-sdk).
Needs `<memory>`, `<cstdio>`.

> [!WARNING]
> **Trap:** a plain `std::unique_ptr<std::FILE>` compiles happily and then calls `delete` on a pointer C code allocated — undefined behavior every time. The deleter must match the allocator, which is the whole reason it is part of the type.

### Recipe 8 — Look up a key without inserting it

**In C#:** `if (settings.TryGetValue("timeout", out var value))`

**The recipe:**

```cpp
void apply_timeout_setting(const std::map<std::string, int>& settings) {
    const auto it = settings.find("timeout");
    if (it != settings.end()) {
        apply_timeout(it->second);    // found - the iterator is the out-parameter
    }
}
```

**Why it looks like this.** `find` is `TryGetValue` with the iterator playing
the out-parameter: one lookup, no exception, no insertion. Its two siblings
do different jobs — `at()` is the throwing indexer, and `operator[]` is
*insert-or-return*, a writer's tool.
[Chapter 11](11-stl-containers-and-algorithms.md#chapter-11--stl-containers-algorithms-and-iterator-invalidation)
owns the container story; this is its most-used line, pulled out to where you
will look for it. Needs `<map>` — or `<unordered_map>`; the recipe is
identical.

> [!WARNING]
> **Trap:** reading a missing key with `settings["timeout"]` default-constructs a value and inserts it — the read mutates the map. That is also why `[]` does not compile on a `const` map: the compiler is telling you it writes.

<!-- nav:begin -->
[← Appendix D — Resources, Further Reading, and First-Week Tips](D-resources.md) · [Contents](README.md)
<!-- nav:end -->
