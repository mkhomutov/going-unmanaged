## Appendix F — The Rosetta Cookbook

Parts I–VI teach the language. This appendix serves a different moment:
mid-task, the C# name already in your head — or the Java name, middle
column — and fifteen seconds to spend. Find the thing you are reaching for; the recipe gives the C++
spelling, says why it looks that way, and names the trap that costs an
afternoon. The *why* paragraphs cross-reference the chapter that owns each
concept rather than re-teaching it — this page is for looking up, not for
reading.

Every listing compiles, runs, and holds under the canonical flags: the
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

<!-- nav:begin -->
[← Appendix E — Glossary](E-glossary.md) · [Contents](README.md)
<!-- nav:end -->
