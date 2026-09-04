## Appendix F — The Rosetta Cookbook

Parts I–VI teach the language. This appendix serves a different moment:
mid-task, the C# name already in your head — or the Java name, middle
column — and fifteen seconds to spend. Find the thing you are reaching for; the recipe gives the C++
spelling, says why it looks that way, and names the trap that costs an
afternoon. The *why* paragraphs cross-reference the chapter that owns each
concept rather than re-teaching it — this page is for looking up, not for
reading.

Every listing compiles, runs, and holds under the canonical flags — all but
one, marked where it appears, which is C++23 and builds behind a probe: the
recipes live as code in `exercises/cookbook/`, and `scripts/build_all.sh`
asserts what each one claims on every push. Recipe numbers are stable —
recipes append and are never renumbered — so a note that says "Recipe 7"
stays right.

| Reaching for... | ...or, in Java | The recipe |
|---|---|---|
| `File.ReadAllText` | `Files.readString` | [Recipe 1 — Read a whole file into a string](#recipe-1--read-a-whole-file-into-a-string) |
| `string.Split` | `String.split` | [Recipe 2 — Split a string](#recipe-2--split-a-string) |
| `string.Join` | `String.join` | [Recipe 3 — Join strings](#recipe-3--join-strings) |
| `StringBuilder` | `StringBuilder` (same name) | [Recipe 4 — Build a string in a loop](#recipe-4--build-a-string-in-a-loop) |
| `string.Format` / `$"..."` | `String.format` | [Recipe 5 — Format values into a string](#recipe-5--format-values-into-a-string) |
| `Stopwatch` | `System.nanoTime` | [Recipe 6 — Time a call](#recipe-6--time-a-call) |
| `using` / `IDisposable` | try-with-resources | [Recipe 7 — Wrap a C handle so it frees itself](#recipe-7--wrap-a-c-handle-so-it-frees-itself) |
| `TryGetValue` | `Map.get` / `getOrDefault` | [Recipe 8 — Look up a key without inserting it](#recipe-8--look-up-a-key-without-inserting-it) |
| `File.WriteAllText` | `Files.writeString` | [Recipe 9 — Write a string to a file](#recipe-9--write-a-string-to-a-file) |
| `Path.Combine` | `Path.of` / `resolve` | [Recipe 10 — Build a path from pieces](#recipe-10--build-a-path-from-pieces) |
| `File.Exists` / `Directory.Exists` | `Files.exists` / `isRegularFile` | [Recipe 11 — Check that a file or directory exists](#recipe-11--check-that-a-file-or-directory-exists) |
| `Directory.GetFiles` | `Files.list` | [Recipe 12 — List the files in a directory](#recipe-12--list-the-files-in-a-directory) |
| `Task.Run` / `await` | `ExecutorService` + `Future.get` | [Recipe 13 — Run work on another thread and wait for it](#recipe-13--run-work-on-another-thread-and-wait-for-it) |
| `event` / `EventHandler` | `addXxxListener` | [Recipe 14 — Expose an event](#recipe-14--expose-an-event) |
| `Console.WriteLine` / `Console.Error` | `System.out` / `System.err` | [Recipe 15 — Print a diagnostic you will actually see](#recipe-15--print-a-diagnostic-you-will-actually-see) |
| `System.Timers.Timer` / `Task.Delay` | `ScheduledExecutorService` | [Recipe 16 — Run something every interval](#recipe-16--run-something-every-interval) |
| `Encoding.UTF8.GetString` / `GetBytes` | `getBytes(UTF_8)` / `new String(bytes, UTF_8)` | [Recipe 17 — Convert between UTF-8 and UTF-16](#recipe-17--convert-between-utf-8-and-utf-16) |
| `list.IndexOf` / `Contains` / `str.Contains` | `indexOf` / `contains` | [Recipe 18 — Find an element, an index, or a substring](#recipe-18--find-an-element-an-index-or-a-substring) |
| `int?` / `??` / `?.` | `Optional<T>` / `orElse` / `map` | [Recipe 19 — Carry a value that may be absent](#recipe-19--carry-a-value-that-may-be-absent) |
| pattern-matching `switch` on a type | sealed interfaces + `switch` | [Recipe 20 — Switch on the kind of a message](#recipe-20--switch-on-the-kind-of-a-message) |
| `class ParseException : Exception` | `class ParseException extends Exception` | [Recipe 21 — Throw and catch your own exception type](#recipe-21--throw-and-catch-your-own-exception-type) |
| `int.TryParse` with a reason / a `Result<T>` from a library | `Optional` / `Either` from a library | [Recipe 22 — Return a value or an error](#recipe-22--return-a-value-or-an-error) |
| `string.IsNullOrEmpty` / `s ?? ""` | `s == null \|\| s.isEmpty()` | [Recipe 23 — Test for an empty string, and for no string at all](#recipe-23--test-for-an-empty-string-and-for-no-string-at-all) |
| `[Conditional("DEBUG")]` / `#if DEBUG` | `assert` (with `-ea`) | [Recipe 24 — Compile a diagnostic out of Release](#recipe-24--compile-a-diagnostic-out-of-release) |
| `JsonSerializer.Serialize` | Jackson `writeValueAsString` | [Recipe 25 — Serialize a record to JSON](#recipe-25--serialize-a-record-to-json) |
| `JsonSerializer.Deserialize<T>` | Jackson `readValue` | [Recipe 26 — Read a JSON config with defaults](#recipe-26--read-a-json-config-with-defaults) |
| `new List<T>(capacity)` / `new T[n]` | `new ArrayList<>(n)` / `new T[n]` | [Recipe 27 — Pre-size a collection](#recipe-27--pre-size-a-collection) |
| LINQ | Streams | the collections index predates this page: [the LINQ table of Chapter 11](11-stl-containers-and-algorithms.md#chapter-11--stl-containers-algorithms-and-iterator-invalidation) |

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
and `<chrono>` will not let you mix units by accident. Needs `<chrono>`, `<iostream>`.

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
kind — maps to .NET 7's late-arriving `Path.Exists`; before that it had no
C# name. Every `std::filesystem` function ships as
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
on gcc/clang usually a fresh OS thread, no pool unless you build one; MSVC
runs it on the Windows thread pool, recycling threads — so never rely on
fresh-thread guarantees like `thread_local` starting clean —
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

### Recipe 14 — Expose an event

**In C#:** `public event EventHandler<int> SampleReady;` … `SampleReady?.Invoke(this, s);`

**The recipe:**

```cpp
class SampleSource {
public:
    using Handler = std::function<void(int)>;

    int subscribe(Handler handler) {
        handlers_.emplace_back(next_id_, std::move(handler));
        return next_id_++;    // the token is how -= works without delegate identity
    }

    void unsubscribe(int id) {
        handlers_.erase(std::remove_if(handlers_.begin(), handlers_.end(),
                            [id](const auto& entry) { return entry.first == id; }),
                        handlers_.end());
    }

    void raise(int sample) {    // the ?.Invoke: an empty list is a zero-pass loop
        for (const auto& entry : handlers_) {
            entry.second(sample);
        }
    }

private:
    std::vector<std::pair<int, Handler>> handlers_;
    int next_id_ = 0;
};
```

**Why it looks like this.** `event` is language sugar over a delegate field;
here the field is explicit — a vector of callables — and `std::function` is
the delegate ([Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency)).
The token replaces `-=`: C# unsubscribes by delegate identity, but
`std::function` cannot be compared for equality, so subscribers hold the id
that `subscribe` returned. The `?.Invoke` null-check disappears — an empty
vector loops zero times — and "only the declaring class may raise" is an
access decision you make (put `raise` in `private`), not a language rule.
Two C# habits to check at the door: a handler that unsubscribes *during*
`raise` mutates the vector mid-loop — Chapter 21's invalidation, arriving
through an event — and the whole consuming side of this pattern is
[Chapter 22](22-exercise-lambda-lifetimes.md#chapter-22--exercise-lambda-lifetimes)'s
subject. Needs `<functional>`, `<vector>`, `<algorithm>`, `<utility>`.

> [!WARNING]
> **Trap:** the C# leak runs the other way here — C#'s classic event bug is the publisher keeping dead subscribers *alive*; nothing here keeps anything alive, so a subscriber that dies without `unsubscribe` leaves a dangling capture, and the next `raise` is a use-after-free delivered by your own class.

### Recipe 15 — Print a diagnostic you will actually see

**In C#:** `Console.WriteLine(...)` / `Console.Error.WriteLine(...)`

**The recipe:**

```cpp
void report_progress(int done, int total) {
    std::cout << "processed " << done << " of " << total << '\n';    // buffered: fast
}

void report_failure(const std::string& what) {
    std::cerr << "error: " << what << '\n';    // unbuffered: survives a crash
}
```

**Why it looks like this.** The mapping is direct — `cout` is `Console.Out`,
`cerr` is `Console.Error` — but the split that matters is buffering.
`cout` is line-buffered at a terminal on POSIX (a Windows console flushes
per call — even sooner) and *fully* buffered into a file or CI
log everywhere, and a process that dies takes the buffer with it —
[Chapter 28](28-testing.md#chapter-28--testing) watched four `[ ok ]` lines
vanish exactly this way. `cerr` is unbuffered: slower per line, on screen
before the next statement runs, which is precisely what you want from the
message that explains the crash. When the codebase needs real logging —
levels, sinks, rotation — the ecosystem default is **spdlog**; in plug-in
work, first check whether the host SDK hands you a log *callback*, because
writing into the host's log is worth more than owning your own. And for
Chapter 29's bugs, prints are the wrong tool entirely — they change the
timing (Chapter 31's point); reach for the sanitizer instead. Needs
`<iostream>`.

> [!WARNING]
> **Trap:** `std::endl` is a flush, not a newline — in a hot loop it turns buffered output into a syscall per line; but drop flushing entirely and Chapter 28's fate awaits: the crash eats the buffer and the log ends four lines early. `'\n'` by default, flush on purpose.

### Recipe 16 — Run something every interval

**In C#:** `var t = new System.Timers.Timer(250); t.Elapsed += OnTick; t.Start();`

**The recipe:**

```cpp
class RepeatingTimer {
public:
    RepeatingTimer(std::chrono::milliseconds interval, std::function<void()> tick)
        : worker_([this, interval, tick = std::move(tick)] {
              while (!stop_) {
                  // Task.Delay, spelled honestly: a thread you own, blocked.
                  std::this_thread::sleep_for(interval);
                  if (!stop_) {
                      tick();
                  }
              }
          }) {}

    ~RepeatingTimer() {
        stop_ = true;
        worker_.join();    // Chapter 29's obligation - and this join IS the Stop()
    }

private:
    std::atomic<bool> stop_{false};    // declared before worker_: initialized first
    std::thread worker_;
};
```

**Why it looks like this.** The standard library has no timer, and the
honest answer has two halves. In plug-in work, *the host's tick or idle
callback is the timer* — starting your own thread inside someone else's
event loop is a transplant error, so read the SDK's threading documentation
before writing this class. When you do own the process, a timer is exactly
this: a worker thread, a sleep loop (`sleep_for` is `Task.Delay`, blocking a
real thread — [Chapter 29](29-concurrency.md#chapter-29--concurrency)'s
model), and an atomic stop flag the destructor sets before the join that
Chapter 29 obliges. The member order is Finding 2 of Chapter 25 applied:
`stop_` is declared before `worker_` so the thread never reads an
uninitialized flag. And the captured `this` is why the type must not move —
Chapter 18's re-register-on-move lesson; the user-declared destructor
conveniently suppresses the moves. Teardown waits out at most one interval;
a `condition_variable` turns that into an immediate wake when it matters.
Needs `<atomic>`, `<chrono>`, `<functional>`, `<thread>`.

> [!WARNING]
> **Trap:** a timer whose tick touches an object must not outlive it — and C# let you forget `Stop()` because the GC kept the target alive; here the join in the destructor *is* the Stop, and skipping it (a detached thread) is a tick delivered into freed memory.

### Recipe 17 — Convert between UTF-8 and UTF-16

**In C#:** `Encoding.UTF8.GetBytes(s)` / `Encoding.UTF8.GetString(bytes)` — or nothing at all, because `string` *was* UTF-16 and the runtime converted at every boundary without telling you.

**The recipe:**

```cpp
// UTF-8 -> UTF-16. Invalid input becomes U+FFFD, the convention browsers
// follow; no exceptions, no locale, no deprecated machinery.
std::u16string utf8_to_utf16(std::string_view utf8) {
    std::u16string out;
    for (std::size_t i = 0; i < utf8.size(); ) {
        const auto b0 = static_cast<unsigned char>(utf8[i]);
        std::size_t n = b0 < 0x80          ? 1
                      : (b0 & 0xE0) == 0xC0 ? 2
                      : (b0 & 0xF0) == 0xE0 ? 3
                      : (b0 & 0xF8) == 0xF0 ? 4 : 0;
        char32_t cp = n == 1 ? b0
                    : n     ? b0 & (0x7Fu >> n)   // the lead byte's payload
                            : 0xFFFDu;            // stray or invalid lead
        std::size_t taken = 1;
        for (std::size_t k = 1; n && k < n && i + k < utf8.size(); ++k) {
            const auto bk = static_cast<unsigned char>(utf8[i + k]);
            if ((bk & 0xC0) != 0x80) { n = 0; break; }  // sequence cut short
            cp = (cp << 6) | (bk & 0x3Fu);
            ++taken;
        }
        if (n == 0 || taken != n || cp > 0x10FFFFu ||
            (cp >= 0xD800u && cp <= 0xDFFFu) ||           // surrogates
            (n == 2 && cp < 0x80u) || (n == 3 && cp < 0x800u) ||
            (n == 4 && cp < 0x10000u))                    // overlong forms
            cp = 0xFFFDu;
        i += taken;
        if (cp < 0x10000u) {
            out.push_back(static_cast<char16_t>(cp));
        } else {                                  // astral plane: a pair
            cp -= 0x10000u;
            out.push_back(static_cast<char16_t>(0xD800u + (cp >> 10)));
            out.push_back(static_cast<char16_t>(0xDC00u + (cp & 0x3FFu)));
        }
    }
    return out;
}

// UTF-16 -> UTF-8. Lone surrogates become U+FFFD; everything else is
// mechanical: split the code point across 1-4 bytes, high bits first.
std::string utf16_to_utf8(std::u16string_view utf16) {
    std::string out;
    for (std::size_t i = 0; i < utf16.size(); ++i) {
        char32_t cp = utf16[i];
        if (cp >= 0xD800u && cp <= 0xDBFFu && i + 1 < utf16.size() &&
            utf16[i + 1] >= 0xDC00u && utf16[i + 1] <= 0xDFFFu) {
            cp = 0x10000u + ((cp - 0xD800u) << 10) + (utf16[i + 1] - 0xDC00u);
            ++i;                                  // consumed the pair
        } else if (cp >= 0xD800u && cp <= 0xDFFFu) {
            cp = 0xFFFDu;                         // lone surrogate
        }
        if (cp < 0x80u) {
            out.push_back(static_cast<char>(cp));
        } else if (cp < 0x800u) {
            out.push_back(static_cast<char>(0xC0u | (cp >> 6)));
            out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
        } else if (cp < 0x10000u) {
            out.push_back(static_cast<char>(0xE0u | (cp >> 12)));
            out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
            out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
        } else {
            out.push_back(static_cast<char>(0xF0u | (cp >> 18)));
            out.push_back(static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu)));
            out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
            out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
        }
    }
    return out;
}
```

**Why it looks like this.** The honest part first: the standard library has
no good answer — `<codecvt>` was deprecated in C++17 with no replacement —
so real codebases convert with the platform (`MultiByteToWideChar` /
`WideCharToMultiByte` on Windows, where vendor "wide" strings and 16-bit
`wchar_t` live), with ICU or their framework, or with the vendor SDK's own
helpers. [Chapter 9](09-casts-conversions-and-strings.md#chapter-9--casts-conversions-and-strings)'s
rule is only that the conversion is *named*, wherever it lives. What
hand-rolling the mechanism once buys you is the demystification: seventy
lines cover every code point Unicode will ever assign, both directions are
bit-work at documented offsets — [Chapter 34](34-parse-this-capture.md#chapter-34--parse-this-capture)'s
wire discipline applied to text — and damaged input becomes `U+FFFD` (the
browser convention) instead of an exception, which is the policy question
every converter must answer and most APIs bury. `char16_t` is the portable
spelling of "16-bit unit"; on Windows it and `wchar_t` are the same bits.
Needs `<string>`, `<string_view>`.

> [!WARNING]
> **Trap:** none of the three `size()`s counts characters — "Grüße" is five characters, seven UTF-8 bytes and five UTF-16 units, while one 𝄞 is one, four and two. A length check that "worked for years" on ASCII is an encoding bug with a long fuse.

### Recipe 18 — Find an element, an index, or a substring

**In C#:** `list.IndexOf(x)`, `list.Contains(x)`, `text.Contains("word")`

**The recipe:**

```cpp
template <class Seq, class T>
std::optional<std::size_t> index_of(const Seq& values, const T& wanted) {
    const auto it = std::find(values.begin(), values.end(), wanted);
    if (it == values.end()) {
        return std::nullopt;             // an algorithm says "not found" as end()
    }
    return static_cast<std::size_t>(std::distance(values.begin(), it));
}

bool contains_word(std::string_view text, std::string_view word) {
    return text.find(word) != std::string_view::npos;    // a string says it as npos
}
```

**Why it looks like this.** "Not found" has three spellings in C++: an
algorithm says `end()`, a string says `npos` — the largest `size_t` there
is — and a lookup you write yourself says `optional` or `nullptr`, which is
why `index_of` hands back the index as Recipe 19's `optional` rather than as
C#'s `-1`, and only after the check. `std::find` is `IndexOf` without the
index, and the index is a `std::distance` away; C++20 gives the associative
containers a member `contains`, C++23 gives strings one, and a `vector`
never gets it.
[Chapter 11](11-stl-containers-and-algorithms.md#chapter-11--stl-containers-algorithms-and-iterator-invalidation)
owns the algorithm story. Needs `<algorithm>`, `<iterator>`, `<optional>`,
`<string_view>`.

> [!WARNING]
> **Trap:** `if (text.find(word))` compiles and tests the *position* — a match at offset 0 reads as false and `npos` as true; and `std::find_if` over a `std::map` compiles too, walking every node when the member `m.find(key)` was the lookup you meant.

### Recipe 19 — Carry a value that may be absent

**In C#:** `int? port = int.TryParse(text, out var p) ? p : null;` `port ?? 8080;` `text?.Length`

**The recipe:**

```cpp
std::optional<int> parse_port(std::string_view text) {
    int value = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || end != text.data() + text.size() || value < 0 || value > 65535) {
        return std::nullopt;              // not a port: absence, not an error (Chapter 8)
    }
    return value;                         // TryParse's out-parameter, as the return
}

int port_or_default(std::optional<int> port) {
    return port.value_or(8080);           // the ?? operator
}

std::optional<std::size_t> digits_in(const std::optional<std::string>& text) {
    if (!text) {
        return std::nullopt;              // ?. by hand: C++17 has no null-propagating call
    }
    return text->size();                  // -> is only legal once you have checked
}
```

**Why it looks like this.** `std::optional<T>` is `T?` with the value kept
behind `*` and `->` rather than in front of them, so the caller has to
look before touching it, and `std::from_chars` is `int.TryParse` — no
exception, no locale, an error code and an end pointer you check. The
three shapes are the three C# operators: `std::nullopt` is `null`,
`value_or` is `??`, and `?.` has no C++17 spelling — the `if (!text)` is
that operator written out.
[Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency) owns
the type, and
[Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes)
decides when absence is the right answer at all. Needs `<optional>`,
`<charconv>`, `<string>`, `<string_view>`.

> [!WARNING]
> **Trap:** `*port` or `port->` on an empty optional is undefined behavior, not a null-reference exception — it reads garbage, the program carries on, and the sanitizers say nothing; `port.value()` is the spelling that throws.

### Recipe 20 — Switch on the kind of a message

**In C#:** `switch (e) { case Temperature t: ...; case Fault f: ...; case Heartbeat: ...; }`

**The recipe:**

```cpp
struct Temperature { int centi; };        // centi-degrees, as the wire carries them
struct Fault       { int code; };
struct Heartbeat   {};
using Event = std::variant<Temperature, Fault, Heartbeat>;

template <class... Fs> struct overloaded : Fs... { using Fs::operator()...; };
template <class... Fs> overloaded(Fs...) -> overloaded<Fs...>;

std::string describe(const Event& e) {
    return std::visit(overloaded{
        [](const Temperature& t) { return "temperature " + std::to_string(t.centi) + " centi-degrees"; },
        [](const Fault& f)       { return "fault " + std::to_string(f.code); },
        [](Heartbeat)            { return std::string("heartbeat"); },
    }, e);
}
```

**Why it looks like this.** C# pattern-matches on the runtime type of an
object; C++17 has no runtime type for three unrelated structs, so the
closed set is spelled as a `std::variant` and the `switch` as
`std::visit` — a call that hands the live alternative to whichever lambda
takes it. The two `overloaded` lines are the idiom that turns those lambdas
into one callable with one `operator()` each; the standard library does not
ship it, and every codebase on C++17 has a copy.
[Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency) owns
the type, and says when a variant beats the class hierarchy you would have
written in C#. Needs `<variant>`, `<string>`.

> [!WARNING]
> **Trap:** leave one alternative out of the visitor and the build fails — which is the feature; the same omission in a `switch` on a `kind` field compiles and falls through, and that is how a vendor's new event type crashes a plug-in a year after it shipped.

### Recipe 21 — Throw and catch your own exception type

**In C#:** `class ParseException : Exception { public int Line { get; } }` … `catch (ParseException e)`

**The recipe:**

```cpp
class ParseError : public std::runtime_error {
public:
    ParseError(int line, const std::string& what)
        : std::runtime_error("line " + std::to_string(line) + ": " + what),
          line_(line) {}
    int line() const noexcept { return line_; }    // the payload what() cannot carry
private:
    int line_;
};

int parse_channel_count(std::string_view text, int line) {
    int value = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || end != text.data() + text.size() || value <= 0) {
        throw ParseError(line, "channel count is not a number: '" + std::string(text) + "'");
    }
    return value;
}

int channels_or_default(std::string_view text, int line) {
    try {
        return parse_channel_count(text, line);
    } catch (const ParseError& e) {          // the derived type FIRST
        log_line(e.line(), e.what());
        return 2;
    } catch (const std::exception& e) {      // then the base: order is the rule
        log_line(line, e.what());
        return 2;
    }
}
```

**Why it looks like this.** Derive from `std::runtime_error` (or
`std::logic_error` for a caller bug) so every `catch (const std::exception&)`
in the program — the plug-in entry point of
[Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes)
among them — already handles it, and build the message once in the
constructor, because `what()` returns a `const char*` that cannot be
assembled later. Anything `what()` cannot carry is a member with an
accessor. Throw by value, catch by `const&` (Chapter 8): a catch by value
slices the payload off. Needs `<stdexcept>`, `<string>`, `<string_view>`,
`<charconv>`.

> [!WARNING]
> **Trap:** catch clauses are tried in order, so a `catch (const std::exception&)` written above the `catch (const ParseError&)` makes the second handler dead code — both compilers warn by default (clang names it `-Wexceptions`), so a codebase that silences warnings ships it.

### Recipe 22 — Return a value or an error

**In C#:** `if (!int.TryParse(text, out var n)) …` when the caller needs the *reason* — or a `Result<T, TError>` from a library

**The recipe:**

```cpp
template <class T, class E>
class Result {
public:
    static Result ok(T value)   { return Result(std::in_place_index<0>, std::move(value)); }
    static Result fail(E error) { return Result(std::in_place_index<1>, std::move(error)); }

    bool has_value() const noexcept { return state_.index() == 0; }
    explicit operator bool() const noexcept { return has_value(); }

    const T& value() const { return std::get<0>(state_); }   // throws bad_variant_access on a failure
    const E& error() const { return std::get<1>(state_); }   // ...and on a success

private:
    template <std::size_t I, class X>                        // built in place: one move, not two
    Result(std::in_place_index_t<I> door, X&& x) : state_(door, std::forward<X>(x)) {}
    std::variant<T, E> state_;                               // index 0 is the value, 1 the error
};

struct ConfigError { int line; std::string what; };
struct Config      { int channels; };

// The translation at the module's edge: the parser throws, this function
// returns. Nothing above it ever sees a ParseError.
Result<Config, ConfigError> load_config(std::string_view text) {
    try {
        return Result<Config, ConfigError>::ok(Config{parse_channel_count(text, 1)});
    } catch (const ParseError& e) {          // the throw stops here: failure becomes a value
        return Result<Config, ConfigError>::fail(ConfigError{e.line(), e.what()});
    }
}
```

**Why it looks like this.** Three spellings of one idea, chosen by what the
caller needs to know: `std::optional<T>` (Recipe 19) when absence needs no
explanation, this `Result<T, E>` when it does and the toolchain is C++17,
and `std::expected<T, E>` when it is C++23 — the same function with
`std::unexpected(...)` on the failure side, plus `and_then` and `transform`
for chaining, which
[Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes)'s
translation-layer section shows and `exercises/cookbook/expected.cpp`
builds as the cookbook's one C++23 listing. The `Result` above is
[Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency)'s
`std::variant` behind two named doors, and the `try` inside `load_config`
is that section's edge: the parser throws, the function returns. Needs
`<variant>`, `<utility>`, `<cstddef>`, `<string>`, `<string_view>`.

> [!WARNING]
> **Trap:** `value()` on the error side throws — `bad_variant_access` here, `bad_expected_access<E>` in C++23 — so a caller that skips the check has not written error-code style, it has written an exception with a worse name; test with `if (r)` first, and `value()` is for the one frame allowed to throw.

### Recipe 23 — Test for an empty string, and for no string at all

**In C#:** `if (string.IsNullOrEmpty(name))` … `name ?? "unnamed"`

**The recipe:**

```cpp
std::string name_or_default(const char* from_c_api) {
    if (from_c_api == nullptr) {            // the one null there is: a C API's "no name"
        return "unnamed";
    }
    return from_c_api;                      // safe now - std::string(nullptr) is UB
}
```

**Why it looks like this.** `IsNullOrEmpty` exists because a C# `string`
can be null *and* empty and callers rarely care which; C++ separates the
two by type, and
[Chapter 9](09-casts-conversions-and-strings.md#chapter-9--casts-conversions-and-strings)
owns the facts — a `std::string` cannot be null, so `empty()` is the whole
test, "no string at all" is Recipe 19's `optional<std::string>`, and the
one null in the picture is the `const char*` a C API returns, which is what
this recipe guards. The copy into a `std::string` is deliberate: returning
a view would inherit the C buffer's lifetime, Chapter 10's dangling view.
Needs `<string>`.

> [!WARNING]
> **Trap:** `std::string name = Thing_GetName(h);` with a null return is undefined behavior that reads like an assignment — libc++ dies inside the constructor and libstdc++ throws `std::logic_error` — and `scripts/check_platform_claims.sh` asserts both, because neither is a report you would expect from that line.

### Recipe 24 — Compile a diagnostic out of Release

**In C#:** `[Conditional("DEBUG")] static void CheckInvariant(...)` — or `#if DEBUG … #endif`

**The recipe:**

```cpp
void check_channel_count([[maybe_unused]] int channels) {          // used only in Debug
    assert(channels > 0 && "a session has at least one channel");   // gone under NDEBUG
#ifndef NDEBUG
    std::cerr << "[debug] channels=" << channels << '\n';           // and so is this block
#endif
}
```

**Why it looks like this.** `assert` is [Appendix E](E-glossary.md#appendix-e--glossary)'s
`assert / NDEBUG` entry wearing `[Conditional("DEBUG")]`'s job, with two
differences worth the paragraph. The `&& "message"` is the idiom for a
message, since the whole expression is what the failure prints, and an
`#ifndef NDEBUG` block is `#if DEBUG` for anything larger than one
expression — the sign inverted, because the define means *release*. And
unlike `[Conditional]`, which removes the *call*, the call and its argument
evaluation survive here; only the body empties, which is why the parameter
is `[[maybe_unused]]` (in Release nothing reads it, and `-Wextra` would say
so) and why a macro — `#ifdef NDEBUG` / `#define CHECK_CHANNELS(x) ((void)0)`
— is the spelling that also spares the argument. Needs `<cassert>`,
`<iostream>`.

> [!WARNING]
> **Trap:** the expression inside `assert` vanishes with it — `assert(bump() == 1)` runs `bump()` in Debug and never in Release, and `exercises/cookbook/logging.cpp` is built both ways to prove it.

### Recipe 25 — Serialize a record to JSON

**In C#:** `var text = JsonSerializer.Serialize(readings, new JsonSerializerOptions { WriteIndented = true });`

**The recipe:**

```cpp
struct Reading {
    int sensor;
    double value;
    std::string unit;
};

// Two free functions the library finds by argument-dependent lookup - no
// attribute, no reflection: this IS the [JsonPropertyName] table, by hand.
void to_json(json& j, const Reading& r) {
    j = json{{"sensor", r.sensor}, {"value", r.value}, {"unit", r.unit}};
}

void from_json(const json& j, Reading& r) {
    j.at("sensor").get_to(r.sensor);
    j.at("value").get_to(r.value);
    j.at("unit").get_to(r.unit);
}

std::string serialize(const std::vector<Reading>& readings) {
    return json(readings).dump(2);            // 2 = indent; dump() alone is one line
}
```

**Why it looks like this.** The standard library has no JSON
([Chapter 27](27-dependency-management.md#chapter-27--dependency-management)),
so this is the cookbook's one dependency — nlohmann/json, vendored under
`exercises/third_party/` exactly as that chapter's first strategy says,
version recorded beside it. There is no reflection to walk your fields, so
the mapping is two free functions the library finds by argument-dependent
lookup ([Appendix E](E-glossary.md#appendix-e--glossary)'s ADL entry) —
write them once per type and every `std::vector<Reading>`,
`std::map<std::string, Reading>` and nested struct converts for free; forget
one and the error is [Chapter 41](41-templates-you-will-write.md#chapter-41--templates-you-will-write)'s
overload-resolution novel, naming neither `from_json` nor `Reading`. Keys
come out sorted, because the document is a map — unlike `JsonSerializer`,
which writes properties in declaration order, so never diff the two outputs
as text. Needs `<nlohmann/json.hpp>` (`-isystem exercises/third_party` on the
compile line, which `scripts/check.sh` adds), `<string>`, `<vector>`, and
`using json = nlohmann::json;`.

> [!WARNING]
> **Trap:** `j["missing"]` on a non-const document *inserts* a null for the key (Recipe 8's trap), so a read that meant to check has changed what you serialize next. `const` is not the fix: the write no longer compiles, but a *read* of a missing key on a `const json` is an assertion failure, undefined behavior under `NDEBUG`. `at()` is the read, on both.

### Recipe 26 — Read a JSON config with defaults

**In C#:** `var cfg = JsonSerializer.Deserialize<Config>(text)!;` — `public int Timeout { get; set; } = 30;` for the field that may be absent, `required` for the one that must not be

**The recipe:**

```cpp
struct Config {
    int timeout = 30;
    std::string name;
};

Config load_config(std::string_view text) {
    const json j = json::parse(text);         // junk throws json::parse_error - the event pole
    Config c;
    c.timeout = j.value("timeout", c.timeout);      // TryGetValue with a default: absent is fine
    c.name = j.at("name").get<std::string>();        // at(): required - missing throws out_of_range
    return c;
}
```

**Why it looks like this.** Three outcomes, three spellings, and they are
[Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes)'s
decision applied to a file. Text that is not JSON at all is the event pole
and `parse` throws; a field that may be absent is not an error, and
`value(key, default)` is `TryGetValue` with the default in the call; a field
that must be there is `at()`, which throws `out_of_range` naming the key.
The default covers *absence* only: a key present with the wrong type,
`null` included, throws `type_error` — and so does `value()` on a document
that parsed but is not an object, because a bare `5` is valid JSON. The
document owns its strings — `get<std::string>()` copies out, which is the
point. Needs `<nlohmann/json.hpp>` (`-isystem exercises/third_party`),
`<string>`, `<string_view>`, and `using json = nlohmann::json;`.

> [!WARNING]
> **Trap:** a reference into the document — `const auto& s = j.at("name").get_ref<const std::string&>();` — is [Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency)'s dangling view the moment `j` goes out of scope, a heap-use-after-free under ASan; copy the value out, or keep the document alive as long as anything points into it.

### Recipe 27 — Pre-size a collection

**In C#:** `var samples = new List<int>(capacity);` — or `new double[n]`, which is a different thing

**The recipe:**

```cpp
std::vector<int> read_samples(std::size_t expected) {
    std::vector<int> samples;
    samples.reserve(expected);            // List<T>(capacity): room for expected, size still 0
    for (std::size_t i = 0; i < expected; ++i) {
        samples.push_back(next_sample());  // size grows; no reallocation until the room runs out
    }
    return samples;
}

std::vector<double> zeroed(std::size_t n) {
    return std::vector<double>(n);        // new double[n]: n elements, every one 0.0
}
```

**Why it looks like this.** A vector carries two numbers, and C# only ever
showed you one. `size()` is `Count`, the elements that exist; `capacity()`
is the room allocated for them, which `List<T>` keeps as `Capacity` and
almost nobody reads. `reserve` sets the second and leaves the first alone:
no element is constructed, the vector is exactly as empty as before, and
the growth it would have paid for later —
[Chapter 11](11-stl-containers-and-algorithms.md#chapter-11--stl-containers-algorithms-and-iterator-invalidation)'s
reallocation, every element moved to a bigger block — is paid once, up
front, at the moment you know the count. `vector(n)` and `resize(n)` set
*both*: they construct `n` value-initialised elements, zero for a number,
which is `new T[n]`'s contract and not `List<T>(n)`'s. Reserve once, before
the loop; a reserve inside it reserves nothing new, and the amortised
doubling was already doing that job. On
[Chapter 36](36-the-host-stutters.md#chapter-36--the-host-stutters)'s
deadline path, a reserve at setup is how a `push_back` in the callback
stops being an allocation. And what `reserve` does not do is *pin*:
[Chapter 33](33-here-is-the-report.md#chapter-33--here-is-the-report)'s
pitfall stands, and the first growth past the reserve moves everything
again. Needs `<vector>`.

> [!WARNING]
> **Trap:** `reserve(n)` then `v[i] = x` compiles and writes into room that holds no element — undefined behavior AddressSanitizer reports as `container-overflow`, Chapter 21's report shape — and `resize(n)` followed by `n` calls to `push_back` gives you `2n` elements, the first `n` of them zero.

<!-- nav:begin -->
[← Appendix E — Glossary](E-glossary.md) · [Contents](README.md) · [Appendix G — The Bridge Catalogue →](G-the-bridge-catalogue.md)
<!-- nav:end -->
