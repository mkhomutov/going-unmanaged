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
| `File.WriteAllText` | [Recipe 9 — Write a string to a file](#recipe-9--write-a-string-to-a-file) |
| `Path.Combine` | [Recipe 10 — Build a path from pieces](#recipe-10--build-a-path-from-pieces) |
| `File.Exists` / `Directory.Exists` | [Recipe 11 — Check that a file or directory exists](#recipe-11--check-that-a-file-or-directory-exists) |
| `Directory.GetFiles` | [Recipe 12 — List the files in a directory](#recipe-12--list-the-files-in-a-directory) |
| `Task.Run` / `await` | [Recipe 13 — Run work on another thread and wait for it](#recipe-13--run-work-on-another-thread-and-wait-for-it) |
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

### Recipe 9 — Write a string to a file

**In C#:** `File.WriteAllText(path, text);`

**The recipe:**

```cpp
void write_all_text(const std::string& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("cannot create: " + path);
    }
    out << text;
    if (!out.flush()) {
        throw std::runtime_error("write failed: " + path);
    }
}
```

**Why it looks like this.** The mirror of
[Recipe 1](#recipe-1--read-a-whole-file-into-a-string), with one asymmetry
that matters: on the way out, errors arrive *late*. The operating system
buffers writes, so a full disk or a yanked drive often surfaces only when
the buffer flushes — and the destructor's close, which also flushes, cannot
report it, because destructors do not throw. The explicit `flush()` before
scope end is therefore the one place the failure can become an exception
instead of silence. `std::ios::binary` for the same reason as Recipe 1:
bytes as written, no platform newline translation. Needs `<fstream>`,
`<stdexcept>`.

> [!WARNING]
> **Trap:** skip the flush-and-check and a full disk is silent data loss — the write "succeeds", the destructor swallows the error, and the file is short. C# threw; here the check is yours.

### Recipe 10 — Build a path from pieces

**In C#:** `var full = Path.Combine(dir, "logs", "app.txt");`

**The recipe:**

```cpp
std::filesystem::path log_path(const std::filesystem::path& dir) {
    return dir / "logs" / "app.txt";    // '/' inserts the platform's separator
}
```

**Why it looks like this.** `std::filesystem::path` (C++17) overloads
division, so the code reads like the path it builds, and the separator is
the platform's problem again — the thing you lost leaving `Path.Combine`
behind. It is a real type, not a string convention: `.filename()`,
`.extension()` and `.parent_path()` replace the `Path.Get*` family. And one
C# rule ports exactly: an absolute right-hand side replaces everything to
its left, just as it does in `Path.Combine` — that reflex survives the move.
Needs `<filesystem>`.

> [!WARNING]
> **Trap:** `p += "logs"` compiles and glues — `+=` is string concatenation with no separator, so one character separates `dir/logs` from `dirlogs`; the separator-aware append is `/=` (or `/`).

### Recipe 11 — Check that a file or directory exists

**In C#:** `if (File.Exists(path))` / `if (Directory.Exists(path))`

**The recipe:**

```cpp
namespace fs = std::filesystem;

bool config_present(const fs::path& p) {
    return fs::is_regular_file(p);    // File.Exists: it exists AND is a file
}

bool logs_dir_present(const fs::path& p) {
    return fs::is_directory(p);       // Directory.Exists: exists AND is a directory
}
```

**Why it looks like this.** The split is the same split C# makes:
`is_regular_file` is `File.Exists` (it exists *and* is a file),
`is_directory` is `Directory.Exists`, and the bare `fs::exists` — either
kind — is the one with no C# name. Every `std::filesystem` function ships as
[Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes)'s
pair — a throwing overload and an `error_code` overload — so the error
dialect is your choice per call site; the alias line is the convention
everyone writes. Needs `<filesystem>`.

> [!WARNING]
> **Trap:** check-then-open is a race — the file can vanish between the two, so gate nothing on this that the open will not re-verify itself; Recipe 1's `if (!in)` is the check that counts, this one is for reporting.

### Recipe 12 — List the files in a directory

**In C#:** `foreach (var f in Directory.GetFiles(dir))`

**The recipe:**

```cpp
std::vector<std::filesystem::path> list_files(const std::filesystem::path& dir) {
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path());
        }
    }
    return files;
}
```

**Why it looks like this.** The iterator *is* the enumeration: range-`for`
over a `directory_iterator` visits each entry once, the entry answers
`is_regular_file()` from what the traversal already knows, and
`recursive_directory_iterator` is `SearchOption.AllDirectories`. There is no
pattern argument — filter on `.extension()` yourself, which costs a line and
spares you a glob dialect. Needs `<filesystem>`, `<vector>`.

> [!WARNING]
> **Trap:** the order is unspecified — the same loop lists alphabetically on your machine and arbitrarily on the CI box, and C# never promised an order either, it just tended to deliver one; `std::sort` the result if order matters.

### Recipe 13 — Run work on another thread and wait for it

**In C#:** `var task = Task.Run(CountDefects); ... var n = await task;`

**The recipe:**

```cpp
int overlap_work() {
    std::future<int> task = std::async(std::launch::async, count_defects);
    const int other = do_other_work();    // runs while count_defects runs
    return other + task.get();            // the await: blocks until the result arrives
}
```

**Why it looks like this.** `std::async` is `Task.Run` without the runtime:
usually a fresh OS thread, no pool unless you build one —
[Chapter 29](29-concurrency.md#chapter-29--concurrency)'s model, in one
line. `.get()` is `await` spelled as a block: this thread stops until the
result arrives; nothing suspends, nothing resumes elsewhere. The
`std::launch::async` policy is not decoration — the default *may defer* the
work to run lazily inside `.get()`, on this thread, which is the opposite of
what `Task.Run` means. One behavior ports exactly: a throw inside the work
is captured and rethrown at `.get()`, the same unwrapping `await` did for
you. Needs `<future>`.

> [!WARNING]
> **Trap:** the future returned by `std::async` blocks in its destructor until the work finishes — dropping it to fire-and-forget turns "run this in the background" into "stop here until it is done", silently serializing the program.

<!-- nav:begin -->
[← Appendix D — Resources, Further Reading, and First-Week Tips](D-resources.md) · [Contents](README.md)
<!-- nav:end -->
