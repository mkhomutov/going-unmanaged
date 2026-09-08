## Appendix F — The Rosetta Cookbook

Parts I–VI teach the language. This appendix serves a different moment:
mid-task, the C# name already in your head — or the Java name, middle
column — and fifteen seconds to spend. Find the thing you are reaching for; the recipe gives the C++
spelling, says why it looks that way, and names the trap that costs an
afternoon. The *why* paragraphs cross-reference the chapter that owns each
concept rather than re-teaching it — this page is for looking up, not for
reading.

Every listing compiles, runs, and holds under the canonical flags — all but
one, which is C++23; that one, the two that need libcrypto, the one that
needs libcurl and the one that needs sqlite3 build behind probes, and say so
where they appear. The recipes live as code in
`exercises/cookbook/`, and `scripts/build_all.sh` asserts what each one
claims on every push. Recipe numbers are stable —
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
| `new List<T>(capacity)` / `new T[n]` | `new ArrayList<>(n)` / `new int[n]` | [Recipe 27 — Pre-size a collection](#recipe-27--pre-size-a-collection) |
| `Stopwatch` in a `finally` / timing a delegate | `try`/`finally` + `System.nanoTime` | [Recipe 28 — Time a block on every exit, and a call for its result](#recipe-28--time-a-block-on-every-exit-and-a-call-for-its-result) |
| `DateTime.UtcNow.ToString("o")` | `Instant.now()` / `ISO_INSTANT` | [Recipe 29 — Stamp a log line with the time](#recipe-29--stamp-a-log-line-with-the-time) |
| `TimeSpan.FromSeconds(2)` handed to a native call | `Duration.ofSeconds(2)` / `toMillis()` | [Recipe 30 — Pass a timeout to a C API](#recipe-30--pass-a-timeout-to-a-c-api) |
| `IConfiguration` read at startup / `IsEnabledAsync` | `System.getenv` at startup | [Recipe 31 — Read a feature flag once](#recipe-31--read-a-feature-flag-once) |
| `[Flags] enum` + `HasFlag` | `EnumSet.of` / `contains` | [Recipe 32 — Combine flags as an enum class](#recipe-32--combine-flags-as-an-enum-class) |
| a field of class type + `IDisposable` | a field + `AutoCloseable` | [Recipe 33 — Hold an owned object as a field](#recipe-33--hold-an-owned-object-as-a-field) |
| `new BigThing()` — always on the heap | `new BigThing()` — always on the heap | [Recipe 34 — An object too big for the stack](#recipe-34--an-object-too-big-for-the-stack) |
| `EnumerateObject` / `TryGetProperty` / `EnumerateArray` | Jackson `JsonNode` — `fields()` / `has` / `elements()` | [Recipe 35 — Walk a JSON document you do not own](#recipe-35--walk-a-json-document-you-do-not-own) |
| `SHA256.HashData` / `Convert.ToHexString` | `MessageDigest.getInstance("SHA-256")` | [Recipe 36 — Hash bytes](#recipe-36--hash-bytes) |
| `AesGcm.Encrypt` / `Decrypt` | `Cipher.getInstance("AES/GCM/NoPadding")` | [Recipe 37 — Seal bytes for a reader in C#](#recipe-37--seal-bytes-for-a-reader-in-c) |
| `File.Replace` / write-then-move by hand | `Files.move(..., ATOMIC_MOVE)` | [Recipe 38 — Save a file without losing the old one](#recipe-38--save-a-file-without-losing-the-old-one) |
| `Directory.CreateDirectory` / `File.Copy` / `File.Move` / `Directory.Delete(recursive)` | `Files.createDirectories` / `copy` / `move` / `walkFileTree` | [Recipe 39 — Create, copy, move and delete, and a whole tree](#recipe-39--create-copy-move-and-delete-and-a-whole-tree) |
| `FileSystemWatcher` | `WatchService` | [Recipe 40 — Notice a file changed](#recipe-40--notice-a-file-changed) |
| `HttpClient.GetStringAsync` | `HttpClient.send` | [Recipe 41 — Call an HTTP endpoint](#recipe-41--call-an-http-endpoint) |
| `SqliteConnection` / `SqliteCommand.ExecuteReader` | `DriverManager.getConnection` / `PreparedStatement` | [Recipe 42 — Open a local database and run a query](#recipe-42--open-a-local-database-and-run-a-query) |
| `MemoryMappedFile.CreateOrOpen` / `CreateViewAccessor` | `FileChannel.map` (files only) | [Recipe 43 — Share a buffer with another process](#recipe-43--share-a-buffer-with-another-process) |
| `Regex.IsMatch` / `Match(...).Groups[1]` / `Regex.Replace` | `Pattern.compile` / `Matcher.group(1)` / `replaceAll` | [Recipe 44 — Match a pattern](#recipe-44--match-a-pattern) |
| `Trim` / `Equals(OrdinalIgnoreCase)` / `StartsWith` / `EndsWith` | `strip` / `equalsIgnoreCase` / `startsWith` / `endsWith` | [Recipe 45 — Trim, compare ignoring case, prefix and suffix](#recipe-45--trim-compare-ignoring-case-prefix-and-suffix) |
| `PostAsJsonAsync` / `ReadFromJsonAsync<T>` | `BodyPublishers.ofString(mapper.writeValueAsString(r))` / Jackson `readValue` | [Recipe 46 — Post a JSON body and read a JSON reply](#recipe-46--post-a-json-body-and-read-a-json-reply) |
| `Rfc2898DeriveBytes.Pbkdf2` / `HKDF.DeriveKey` | `SecretKeyFactory.getInstance("PBKDF2WithHmacSHA256")` / `KDF.getInstance("HKDF-SHA256")` (JDK 25) | [Recipe 47 — Derive a key](#recipe-47--derive-a-key) |
| `HMACSHA256.HashData` / `CryptographicOperations.FixedTimeEquals` | `Mac.getInstance("HmacSHA256")` / `MessageDigest.isEqual` | [Recipe 48 — Sign and verify bytes](#recipe-48--sign-and-verify-bytes) |
| `MemoryMappedFile.CreateFromFile` / `File.ReadAllBytes` on a large file | `FileChannel.map(READ_ONLY)` | [Recipe 49 — Read a large file without copying it](#recipe-49--read-a-large-file-without-copying-it) |
| LINQ | Streams | the collections index predates this page: [the LINQ table of Chapter 11](11-stl-containers-and-algorithms.md#chapter-11--stl-containers-algorithms-and-iterator-invalidation) |

**The clocks, by name.** The five things `System` gave you for time, and
where each lands — the map the timing recipes teach one row at a time:

| In C# | In C++ | Which recipe |
|---|---|---|
| `Stopwatch` | two `steady_clock::time_point`s and a `duration` | 6, 28 |
| `TimeSpan` | `std::chrono::duration` — the unit is in the type, `count()` strips it | 30 |
| `DateTime.UtcNow` | `system_clock::time_point`, the one clock with a calendar | 29 |
| `DateTime.Now` / `DateTimeOffset` / time zones | C++20's `zoned_time`, where the standard library ships it; before it, `localtime_r`/`localtime_s` and an offset you read yourself | none — 29's trap only warns |
| `Task.Delay` / `Thread.Sleep` | `std::this_thread::sleep_for(duration)` | 16 |

### Recipe 1 — Read a whole file into a string

**In C#:** `var text = File.ReadAllText(path);`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/files.cpp:recipe-1"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/files.rs:recipe-1"
    ```

**Why it looks like this.** There is no `File` static class: the stream
object *is* the open file, and
[Chapter 1](01-ownership-and-raii.md#chapter-1--ownership-and-raii) already
taught you what that buys — the `ifstream` closes itself on every path out of
the function, the throw included. `rdbuf()` hands the whole file to the
string stream in one operation, the closest thing iostreams have to a
one-liner. And `std::ios::binary` reads the bytes as they are; without it,
Windows translates `\r\n` on the way through and the "same" file compares
differently per platform. The parameter is a `std::filesystem::path`
rather than a `std::string`, because on Windows a path is not made of
`char` (Recipe 10) and the stream constructors have taken a `path` since
C++17 — a string argument still converts. Needs `<filesystem>`,
`<fstream>`, `<sstream>`, `<stdexcept>`. **In Rust** the same call is `std::fs::read_to_string`, and the trap below cannot happen: a missing file is an `Err`, not an empty string, and the `?` at the call site is the check you would otherwise forget.

> [!WARNING]
> **Trap:** a stream that failed to open does not throw — every read on it quietly produces nothing, so without the `if (!in)` check a missing file becomes an empty string and no error. That check is the part `File.ReadAllText` did for you.

### Recipe 2 — Split a string

**In C#:** `var parts = text.Split(',');`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/strings.cpp:recipe-2"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/strings.rs:recipe-2"
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

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/strings.cpp:recipe-3"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/strings.rs:recipe-3"
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

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/strings.cpp:recipe-4"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/strings.rs:recipe-4"
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

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/strings.cpp:recipe-5"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/strings.rs:recipe-5"
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

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/timing.cpp:recipe-6"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/timing.rs:recipe-6"
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

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/handles.cpp:recipe-7"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/handles.rs:recipe-7"
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
Needs `<memory>`, `<cstdio>`. **In Rust** the wrapper is a struct with `Drop` — the destructor by another name — around the raw handle, and the C functions are declared with `extern "C"`; the `unsafe` blocks mark exactly the two lines that trust C, which is the boundary Chapter 39 draws by hand.

> [!WARNING]
> **Trap:** a plain `std::unique_ptr<std::FILE>` compiles happily and then calls `delete` on a pointer C code allocated — undefined behavior every time. The deleter must match the allocator, which is the whole reason it is part of the type.

### Recipe 8 — Look up a key without inserting it

**In C#:** `if (settings.TryGetValue("timeout", out var value))`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/lookups.cpp:recipe-8"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/lookups.rs:recipe-8"
    ```

**Why it looks like this.** `find` is `TryGetValue` with the iterator playing
the out-parameter: one lookup, no exception, no insertion. Its two siblings
do different jobs — `at()` is the throwing indexer, and `operator[]` is
*insert-or-return*, a writer's tool.
[Chapter 11](11-stl-containers-and-algorithms.md#chapter-11--stl-containers-algorithms-and-iterator-invalidation)
owns the container story; this is its most-used line, pulled out to where you
will look for it. Needs `<map>` — or `<unordered_map>`; the recipe is
identical. **In Rust** `HashMap::get` and `BTreeMap::get` never insert, and the inserting lookup has its own name, `entry` — the distinction C++ hides behind `[]`.

> [!WARNING]
> **Trap:** reading a missing key with `settings["timeout"]` default-constructs a value and inserts it — the read mutates the map. That is also why `[]` does not compile on a `const` map: the compiler is telling you it writes.

### Recipe 9 — Write a string to a file

**In C#:** `File.WriteAllText(path, text);`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/files.cpp:recipe-9"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/files.rs:recipe-9"
    ```

**Why it looks like this.** The mirror of
[Recipe 1](#recipe-1--read-a-whole-file-into-a-string), with one asymmetry
that matters: on the way out, errors arrive *late*. The operating system
buffers writes, so a full disk or a yanked drive often surfaces only when
the buffer flushes — and the destructor's close, which also flushes, cannot
report it, because destructors do not throw. The explicit `flush()` before
scope end is therefore the one place the failure can become an exception
instead of silence. `std::ios::binary` for the same reason as Recipe 1:
bytes as written, no platform newline translation — and the parameter is a
`path` for Recipe 1's reason. Needs `<filesystem>`, `<fstream>`,
`<stdexcept>`.

> [!WARNING]
> **Trap:** skip the flush-and-check and a full disk is silent data loss — the write "succeeds", the destructor swallows the error, and the file is short. C# threw; here the check is yours.

### Recipe 10 — Build a path from pieces

**In C#:** `var full = Path.Combine(dir, "logs", "app.txt");`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/paths.cpp:recipe-10"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/paths.rs:recipe-10"
    ```

**Why it looks like this.** `std::filesystem::path` (C++17) overloads
division, so the code reads like the path it builds, and the separator is
the platform's problem again — the thing you lost leaving `Path.Combine`
behind. It is a real type, not a string convention: `.filename()`,
`.extension()` and `.parent_path()` replace the `Path.Get*` family. And one
C# rule ports exactly: an absolute right-hand side replaces everything to
its left, just as it does in `Path.Combine` — that reflex survives the move.
One thing `Path.Combine` never made you ask is what a path is *made of*:
`path::value_type` is `wchar_t` on Windows and `char` everywhere else, and a
`std::string` handed to the constructor is read in the platform's native
narrow encoding — on Windows the process's code page, which is UTF-8 only
if the process opted in — so a UTF-8
name from a JSON file or Recipe 17 arrives on disk as
[Chapter 9](09-casts-conversions-and-strings.md#chapter-9--casts-conversions-and-strings)'s
mojibake. `std::filesystem::u8path(s)` says the string is UTF-8 (C++17;
C++20 deprecates it for `path(u8"...")` with `char8_t`), and `p.u8string()`
is the way back — a `std::string` in C++17, a `std::u8string` in C++20;
the `buildlab-msvc` job asserts the round trip, because Windows is the one
platform where the two constructors differ. Needs `<filesystem>`.

> [!WARNING]
> **Trap:** `p += "logs"` compiles and glues — `+=` is string concatenation with no separator, so one character separates `dir/logs` from `dirlogs`; the separator-aware append is `/=` (or `/`).

### Recipe 11 — Check that a file or directory exists

**In C#:** `if (File.Exists(path))` / `if (Directory.Exists(path))`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/paths.cpp:recipe-11"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/paths.rs:recipe-11"
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

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/paths.cpp:recipe-12"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/paths.rs:recipe-12"
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

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/async.cpp:recipe-13"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/async_work.rs:recipe-13"
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
you. Needs `<future>`. **In Rust** the future is a `JoinHandle`, and `join()` returns a `Result` whose `Err` carries the panic — the exception-surfaces-at-`get()` behaviour, made visible in the type.

> [!WARNING]
> **Trap:** the future returned by `std::async` blocks in its destructor until the work finishes — dropping it to fire-and-forget turns "run this in the background" into "stop here until it is done", silently serializing the program.

### Recipe 14 — Expose an event

**In C#:** `public event EventHandler<int> SampleReady;` … `SampleReady?.Invoke(this, s);`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/events.cpp:recipe-14"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/events.rs:recipe-14"
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
subject. Needs `<functional>`, `<vector>`, `<algorithm>`, `<utility>`. **In Rust** the handler list is `Vec<(u32, Box<dyn FnMut(i32)>)>`, `retain` is the unsubscribe, and the borrow checker asks the question C# never did: a handler that captures the source itself cannot be written without `Rc<RefCell<…>>`, which is the cycle made visible.

> [!WARNING]
> **Trap:** the C# leak runs the other way here — C#'s classic event bug is the publisher keeping dead subscribers *alive*; nothing here keeps anything alive, so a subscriber that dies without `unsubscribe` leaves a dangling capture, and the next `raise` is a use-after-free delivered by your own class.

### Recipe 15 — Print a diagnostic you will actually see

**In C#:** `Console.WriteLine(...)` / `Console.Error.WriteLine(...)`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/logging.cpp:recipe-15"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/logging.rs:recipe-15"
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

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/timing.cpp:recipe-16"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/timing.rs:recipe-16"
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
Needs `<atomic>`, `<chrono>`, `<functional>`, `<thread>`. **In Rust** the timer is the same two fields, an `AtomicBool` behind an `Arc` and a `JoinHandle`, and `Drop` does the join — which is why the handle is an `Option`: `join` consumes it, and a destructor only gets `&mut self`.

> [!WARNING]
> **Trap:** a timer whose tick touches an object must not outlive it — and C# let you forget `Stop()` because the GC kept the target alive; here the join in the destructor *is* the Stop, and skipping it (a detached thread) is a tick delivered into freed memory.

### Recipe 17 — Convert between UTF-8 and UTF-16

**In C#:** `Encoding.UTF8.GetBytes(s)` / `Encoding.UTF8.GetString(bytes)` — or nothing at all, because `string` *was* UTF-16 and the runtime converted at every boundary without telling you.

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/strings.cpp:recipe-17"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/strings.rs:recipe-17"
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
bit-work at documented offsets — [Chapter 34](34-parse-this-capture.md#chapter-34--every-capture-rejected-as-malformed)'s
wire discipline applied to text — and damaged input becomes `U+FFFD` (the
browser convention) instead of an exception, which is the policy question
every converter must answer and most APIs bury. `char16_t` is the portable
spelling of "16-bit unit"; on Windows it and `wchar_t` are the same bits.
Needs `<string>`, `<string_view>`. **In Rust** the two directions are one call each, `encode_utf16` and `String::from_utf16_lossy`, and the third half of the problem — bytes that are not valid UTF-8 — is `String::from_utf8_lossy`, because a `&str` cannot hold them in the first place.

> [!WARNING]
> **Trap:** none of the three `size()`s counts characters — "Grüße" is five characters, seven UTF-8 bytes and five UTF-16 units, while one 𝄞 is one, four and two. A length check that "worked for years" on ASCII is an encoding bug with a long fuse.

### Recipe 18 — Find an element, an index, or a substring

**In C#:** `list.IndexOf(x)`, `list.Contains(x)`, `text.Contains("word")`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/lookups.cpp:recipe-18"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/lookups.rs:recipe-18"
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

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/alternatives.cpp:recipe-19"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/alternatives.rs:recipe-19"
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

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/alternatives.cpp:recipe-20"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/alternatives.rs:recipe-20"
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
written in C#. Needs `<variant>`, `<string>`. **In Rust** the closed set is an `enum` with data on its variants and the switch is `match`, which the compiler holds to exhaustiveness: add a fourth kind and every `match` without it stops compiling, where `std::visit` over an incomplete `overloaded` does the same one template error at a time.

> [!WARNING]
> **Trap:** leave one alternative out of the visitor and the build fails — which is the feature; the same omission in a `switch` on a `kind` field compiles and falls through, and that is how a vendor's new event type crashes a plug-in a year after it shipped.

### Recipe 21 — Throw and catch your own exception type

**In C#:** `class ParseException : Exception { public int Line { get; } }` … `catch (ParseException e)`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/errors.cpp:recipe-21"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/errors.rs:recipe-21"
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
`<charconv>`. **In Rust** there is no throw: the type is a struct with `Display` and `std::error::Error`, the function returns `Result<i32, ParseError>`, and the catch-order rule disappears because there is nothing to catch in order — the caller matches on the value.

> [!WARNING]
> **Trap:** catch clauses are tried in order, so a `catch (const std::exception&)` written above the `catch (const ParseError&)` makes the second handler dead code — both compilers warn by default (clang names it `-Wexceptions`), so a codebase that silences warnings ships it.

### Recipe 22 — Return a value or an error

**In C#:** `if (!int.TryParse(text, out var n)) …` when the caller needs the *reason* — or a `Result<T, TError>` from a library

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/errors.cpp:recipe-22"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/errors.rs:recipe-22"
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
`<variant>`, `<utility>`, `<cstddef>`, `<string>`, `<string_view>`. **In Rust** `Result<T, E>` is the standard library's, `?` is the translation layer in one character, and this recipe's only work is the `map_err` from one error type to the other.

> [!WARNING]
> **Trap:** `value()` on the error side throws — `bad_variant_access` here, `bad_expected_access<E>` in C++23 — so a caller that skips the check has not written error-code style, it has written an exception with a worse name; test with `if (r)` first, and `value()` is for the one frame allowed to throw.

### Recipe 23 — Test for an empty string, and for no string at all

**In C#:** `if (string.IsNullOrEmpty(name))` … `name ?? "unnamed"`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/strings.cpp:recipe-23"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/strings.rs:recipe-23"
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
Needs `<string>`. **In Rust** the null cannot reach a `&str` at all; the check happens where the raw pointer arrives, inside an `unsafe` function whose contract is written above it.

> [!WARNING]
> **Trap:** `std::string name = Thing_GetName(h);` with a null return is undefined behavior that reads like an assignment — libc++ dies inside the constructor and libstdc++ throws `std::logic_error` — and `scripts/check_platform_claims.sh` asserts both, because neither is a report you would expect from that line.

### Recipe 24 — Compile a diagnostic out of Release

**In C#:** `[Conditional("DEBUG")] static void CheckInvariant(...)` — or `#if DEBUG … #endif`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/logging.cpp:recipe-24"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/logging.rs:recipe-24"
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

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/json.cpp:recipe-25"
    ```

=== "Rust"

    Rust's standard library has no JSON. The ecosystem's answer is `serde` with `serde_json` — `#[derive(Serialize, Deserialize)]` on the struct is the whole `[JsonPropertyName]` table — and it is a dependency, which is Chapter 27's decision, not this page's; the crate stays dependency-free.

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
as text. A `std::optional<T>` member serializes as `null` when empty —
the library has written one since 3.12 — never as a missing key, and in
the vendored 3.12.0 reads back only by hand (Recipe 35's
`contains`-then-`at`, with `is_null()` for the value: the read-side
overload is declared behind a guard that is never open, fixed upstream
in #4742 and unreleased at the time of writing); there is no
`JsonIgnoreCondition.WhenWritingNull`, so strip the null before `dump`,
or accept that absent and null are one word on your wire and write that
down ([Chapter 34](34-parse-this-capture.md#chapter-34--every-capture-rejected-as-malformed)). Needs `<nlohmann/json.hpp>` (`-isystem exercises/third_party` on the
compile line, which `scripts/check.sh` adds), `<string>`, `<vector>`, and
`using json = nlohmann::json;`.

> [!WARNING]
> **Trap:** `j["missing"]` on a non-const document *inserts* a null for the key (Recipe 8's trap), so a read that meant to check has changed what you serialize next. `const` is not the fix: the write no longer compiles, but a *read* of a missing key on a `const json` is an assertion failure, undefined behavior under `NDEBUG`. `at()` is the read, on both.

### Recipe 26 — Read a JSON config with defaults

**In C#:** `var cfg = JsonSerializer.Deserialize<Config>(text)!;` — `public int Timeout { get; set; } = 30;` for the field that may be absent, `required` for the one that must not be

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/json.cpp:recipe-26"
    ```

=== "Rust"

    As for Recipe 25: `serde_json::from_str` into a struct with `#[serde(default)]` on the optional fields is the idiom, and it is a dependency this crate does not take.

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

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/containers.cpp:recipe-27"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/containers.rs:recipe-27"
    ```

**Why it looks like this.** A vector carries two numbers and C# showed you
one: `size()` is `Count`, the elements that exist; `capacity()` is the room
allocated for them. `reserve` sets the second and leaves the first alone —
nothing is constructed, and
[Chapter 11](11-stl-containers-and-algorithms.md#chapter-11--stl-containers-algorithms-and-iterator-invalidation)'s
reallocation is paid once, up front, at the moment you know the count —
while `vector(n)` and `resize(n)` set both, constructing `n`
value-initialized elements, which is `new T[n]`'s contract and not
`List<T>(n)`'s: `resize(n)` followed by `n` calls to `push_back` gives you
`2n` elements, the first `n` of them zero. Reserve once, before the loop —
a reserve inside it is either a no-op or, at `size() + 1`, a reallocation
on every pass, the amortized doubling switched off by hand. On
[Chapter 36](36-the-host-stutters.md#chapter-36--dropouts-with-the-plug-in-loaded)'s
deadline path a reserve at setup keeps a `push_back` in the callback from
allocating, for as long as the count stays inside it; and it does not
*pin* — [Chapter 33](33-here-is-the-report.md#chapter-33--a-value-reads-zero-after-hot-plug)'s
pitfall stands. Needs `<vector>`.

> [!WARNING]
> **Trap:** `reserve(n)` then `v[i] = x` compiles and writes into room that holds no element — undefined behavior that AddressSanitizer names `container-overflow` under libc++ ([Chapter 21](21-exercise-iterator-invalidation.md#chapter-21--exercise-iterator-invalidation)'s report shape) and, under libstdc++, only when `-D_GLIBCXX_SANITIZE_VECTOR` switches the annotations on.

### Recipe 28 — Time a block on every exit, and a call for its result

**In C#:** `var sw = Stopwatch.StartNew(); try { ... } finally { Log(sw.Elapsed); }` — or a helper that times a delegate and returns its result

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/timing.cpp:recipe-28"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/timing.rs:recipe-28"
    ```

**Why it looks like this.** The `finally` is a destructor —
[Chapter 1](01-ownership-and-raii.md#chapter-1--ownership-and-raii)'s
shape applied to a measurement, so the stop runs on the early return and
on the throw without a `try` at the call site, and the copies are deleted
because a copy would be a second stopwatch with the same start writing the
same record, and the number would no longer be the block's — Chapter 1's
reason for deleting the file handle's copies, in miniature. The wrapper is a template with `&&` on a deduced type:
`F&&` and `Args&&...` are
[Chapter 6](06-the-rule-of-five-and-move-semantics.md#chapter-6--the-rule-of-five-and-move-semantics)'s
forwarding references, and `std::forward` hands each argument on as it
arrived — an lvalue stays borrowed, an rvalue stays stealable — where
`std::move` would have gutted the caller's variable and a plain pass would
have copied. `std::invoke_result_t` names the return type without running
the call ([Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency)'s
`decltype` with the plumbing hidden), and `std::invoke` accepts a lambda,
a function pointer or a member pointer alike. Both write into a record you
own rather than printing, so a test can assert on it — and a number from
either is a mean, which
[Chapter 36](36-the-host-stutters.md#chapter-36--dropouts-with-the-plug-in-loaded) says
can only acquit a mean; count allocations when the question is the worst
case. Needs `<chrono>`, `<functional>`, `<type_traits>`, `<utility>`.

> [!WARNING]
> **Trap:** a timed call whose result nobody reads is a call the optimizer may delete outright, so the timer brackets nothing and reports nanoseconds — use the result, and measure at `-O2` without the sanitizers, because [Chapter 36](36-the-host-stutters.md#chapter-36--dropouts-with-the-plug-in-loaded)'s factor of twenty is not uniform across code shapes.

### Recipe 29 — Stamp a log line with the time

**In C#:** `DateTime.UtcNow.ToString("o")` — to the millisecond, where `"o"` prints seven fractional digits

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/timing.cpp:recipe-29"
    ```

=== "Rust"

    The standard library has the wall clock (`SystemTime`) but no calendar: formatting a `SystemTime` as a date is the `time` or `chrono` crate's job, a dependency this crate does not take. What std gives you honestly is `SystemTime::now().duration_since(UNIX_EPOCH)` — seconds and millis since the epoch, which a log line can carry as a number.

**Why it looks like this.** Recipe 6 said intervals come from
`steady_clock`; a timestamp is the other clock's job, because
`system_clock` is the only one whose `now()` means a calendar date. C++17
can turn it into text only by stepping down into C: the whole seconds
since the epoch become a `time_t` (`floor` rather than `to_time_t`, which
the standard lets round or truncate as it pleases), the `<ctime>`
breakdown gives a `std::tm`, and `std::put_time` formats it with
`strftime`'s specifiers — so the milliseconds, which `time_t` cannot
hold, come from the same `duration` the seconds were cut from. The `_r`
/ `_s` split is not pedantry: on POSIX `std::gmtime` returns a pointer
into storage shared by every thread, which is
[Chapter 29](29-concurrency.md#chapter-29--concurrency)'s data race
hiding in a formatting function, and on Windows it is the spelling MSVC
deprecates. C++20 collapses the whole
recipe into `std::format("{:%FT%TZ}", std::chrono::floor<std::chrono::milliseconds>(now))`;
until your toolchain is there, this is the spelling. Needs `<chrono>`,
`<ctime>`, `<iomanip>`, `<sstream>`, `<string>`.

> [!WARNING]
> **Trap:** the `Z` is a character you wrote, not something the clock knows — swap `gmtime_r` for `localtime_r` to get "readable" times and every line now claims UTC while carrying local time, wrong by the offset in every log you correlate with another machine's.

### Recipe 30 — Pass a timeout to a C API

**In C#:** `device.Wait(TimeSpan.FromSeconds(2))` — with the unit inside the type

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/timing.cpp:recipe-30"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/timing.rs:recipe-30"
    ```

**Why it looks like this.** A C API has no `TimeSpan`: a timeout arrives
as a bare integer with the unit in the parameter name — the shape of every
C-facing SDK in [Chapter 16](16-the-sdk-bestiary.md#chapter-16--the-sdk-bestiary)'s
Bestiary — and the wrapper is where the typed duration stops: your side speaks
`std::chrono::milliseconds`, and `count()` — the only call that turns a
duration back into a number — sits on the line next to the `_ms`
parameter and nowhere else. The conversions run in one direction for
free: `wait_for_sample(2s)` and `wait_for_sample(std::chrono::minutes(1))`
compile, because seconds to milliseconds loses nothing, while a function
taking `seconds` handed `250ms` does not compile until you write the
`duration_cast` that admits the truncation. The `static_cast` is also
where the *range* leaves: a `milliseconds` count is 64 bits wide and the
vendor's `uint32_t` wraps at forty-nine days, silently, so a wrapper whose
callers can pass anything long clamps before the cast. The literals need
`using namespace std::chrono_literals;` in the scope that uses them. Needs
`<chrono>`, `<cstdint>`. **In Rust** the same conversion is `as_millis()` at the call, `u32::try_from` rather than a cast, and the vendor's declaration is `unsafe extern "C" fn` — the unit still leaves the type in exactly one place.

> [!WARNING]
> **Trap:** `.count()` has no idea what unit it is counting — `seconds(2).count()` handed to a `_ms` parameter compiles and waits two milliseconds — so the parameter type of your wrapper, not the caller's discipline, is what puts the thousand in.

### Recipe 31 — Read a feature flag once

**In C#:** `configuration.GetValue<bool>("FastPath")` — or `await featureManager.IsEnabledAsync("FastPath")` — wherever the code needs it

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/flags.cpp:recipe-31"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/flags.rs:recipe-31"
    ```

**Why it looks like this.** A feature flag is the first of
[Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake)'s
four switches — the only one that involves no build — and its whole
discipline is *when* the read happens: once, at startup, into a struct
whose defaults are the flags' off state, and from then on a member tested
with `if`; the source is whichever channel the plug-in already has —
Recipe 26's JSON, the host's preferences API, the environment as here —
and the struct is what makes that not matter. A flag that must change a
type's *layout* is not this recipe but Chapter 26's `PUBLIC` compile
definition. The harness proves the read-once the only way it can be
proved — it changes the environment after the constructor ran and asserts
the member did not follow — and the broken shape, `std::getenv` inside
`process`, stays book-only, because it would pass every assertion there.
Needs `<charconv>`, `<cstdlib>`, `<string_view>`.

> [!WARNING]
> **Trap:** reading the flag at the point of use — `std::getenv` in the loop, a configuration lookup per call — is a walk of a shared table — under a lock, on macOS and Windows — on [Chapter 36](36-the-host-stutters.md#chapter-36--dropouts-with-the-plug-in-loaded)'s deadline path, and nothing names it: it compiles, runs, passes, and the sanitizers are silent, so the constructor is the only place the read may live.

### Recipe 32 — Combine flags as an enum class

**In C#:** `[Flags] enum Channel { Left = 1, Right = 2 }`, then `Channel.Left | Channel.Right` and `set.HasFlag(Channel.Left)`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/flags.cpp:recipe-32"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/flags.rs:recipe-32"
    ```

**Why it looks like this.** `[Flags]` is a promise to the formatter and to
`HasFlag`; the arithmetic itself C# gives every enum for free. An
`enum class` gives you the type and refuses the arithmetic —
`Channel::Left | Channel::Right` is *invalid operands to binary
expression* until you write the operator — and the two operators plus
`has` are the attribute's entire job, once per enum; a plain `enum` would
compile the `|` and hand back an `int`, the type gone. The
`std::uint8_t` base fixes the width the standard leaves to the compiler,
which is [Chapter 39](39-the-round-trip-home.md#chapter-39--the-round-trip-home)'s
reason for never publishing an enum across a boundary at all; inside one,
it keeps the bits the width the field holds. And `has(set, Channel::None)`
is true for every set, exactly as `HasFlag(0)` is. Needs `<cstdint>`. **In Rust** a `[Flags]` enum is not an `enum` — an enum value must be one of its variants — but a newtype over the bits with associated constants and `BitOr`; the `bitflags` crate generates exactly that.

> [!WARNING]
> **Trap:** `(set & flag) != Channel::None` reads as `HasFlag` and is wrong for a *combined* flag — with `Stereo = Left | Right`, a set holding only `Left` tests true — which is why `has` compares against the flag itself, as `HasFlag` does.

### Recipe 33 — Hold an owned object as a field

**In C#:** `private readonly Log _log;` — and, if `Log` is `IDisposable`, an `IDisposable` on the owner whose `Dispose` calls `_log.Dispose()` by hand

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/ownership.cpp:recipe-33"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/ownership.rs:recipe-33"
    ```

**Why it looks like this.** The C# question — can a field own something,
and who disposes it — has a shorter answer here: every field is destroyed
when its owner is, with nothing to write. What is yours to decide is the
field's *shape*, and it is
[Appendix H](H-choosing.md#appendix-h--choosing-signatures-containers-and-storage)'s
fourth procedure applied to one member: by value until something forces
otherwise — a `std::string` or `std::vector` field already keeps its bulk
on the heap; behind a `unique_ptr` when the type is a polymorphic base
([Chapter 2](02-value-semantics.md#chapter-2--value-semantics)), may be
absent, is incomplete
([Chapter 30](30-authoring-an-abi-boundary.md#chapter-30--authoring-an-abi-boundary)'s
PIMPL) or is too big to carry (Recipe 34); behind a `shared_ptr` only when
co-owned and the cycle question is answered
([Chapter 1](01-ownership-and-raii.md#chapter-1--ownership-and-raii)). A
`unique_ptr` field also settles the class's copies —
[Chapter 6](06-the-rule-of-five-and-move-semantics.md#chapter-6--the-rule-of-five-and-move-semantics)'s
Rule of Zero: copy deleted, move generated, nothing written. Needs
`<memory>`, `<string>`, `<vector>`. **In Rust** `Box<dyn Log>` is the polymorphic owner, `Option` is where absence lives rather than the pointer, and the shared sink needs `Rc<RefCell<Sink>>` because sharing and mutating are two separate permissions.

> [!WARNING]
> **Trap:** fields die in reverse *declaration* order, so a field that another field's destructor uses must be declared before it — declare a by-value `Sink` after a `Log` whose destructor writes a last line into it, and that line lands in a dead field; nothing warns, because `-Wreorder` is about the constructor's list, not the class's, and under libc++ the sanitizers stay quiet too, since the container annotation un-poisons the slot before the write ([Chapter 32](32-it-crashes-on-exit.md#chapter-32--crash-on-exit)'s first pitfall).

### Recipe 34 — An object too big for the stack

**In C#:** nothing to decide — a class instance is on the heap at forty bytes and at forty megabytes; only a struct or a `stackalloc` sees the stack, and when one of those is too big the runtime at least says `StackOverflowException`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/ownership.cpp:recipe-34"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/ownership.rs:recipe-34"
    ```

**Why it looks like this.** [Chapter 1](01-ownership-and-raii.md#chapter-1--ownership-and-raii)'s
decision asked whether the object outlives its scope and whether it has
one owner; this is the third question, and C# never asked it because the
runtime answered it for every class. A stack frame is small —
[Chapter 3](03-stack-heap-and-undefined-behavior.md#chapter-3--stack-heap-and-undefined-behavior)'s
numbers: 1 MB per thread on Windows, 512 KB for a thread you spawn on
macOS, 8 MB for the main thread on both Linux and macOS — and `sizeof`
is transitive, so a `std::array` member this size makes every object that
holds one, and every function that holds one of those, a stack overflow
waiting for a thread whose stack you did not size —
[Chapter 29](29-concurrency.md#chapter-29--concurrency)'s driver thread. `make_unique`
puts the bytes on the heap and leaves a pointer-sized owner behind, which
is the same ownership as before at a different address. When the size is
not a compile-time constant, `std::vector<std::uint8_t>(n)` is the same
answer with the count decided at run time. The `static_assert` is the
reason for the heap written down where it cannot go stale
([Chapter 41](41-templates-you-will-write.md#chapter-41--templates-you-will-write)'s
judge): a reviewer who changes the array's size meets the sentence.
Needs `<array>`, `<cstdint>`, `<memory>`. **In Rust** the trap has a spelling: `Box::new([0u8; N])` builds the array on the stack and then moves it, so a large object is made with `vec![0; N].into_boxed_slice()`, which allocates in place — the test builds one on a 256 KB thread stack to prove it.

> [!WARNING]
> **Trap:** the failure is a crash on *entry* to the function, before its first line runs, and the report is none of [Chapter 31](31-reading-what-the-tools-tell-you.md#chapter-31--reading-what-the-tools-tell-you)'s four shapes — AddressSanitizer names it `stack-overflow` only when the faulting write lands within 64 KB of the stack pointer, and a bare `SEGV`/`BUS` "on unknown address" otherwise, which depends on what happens to be mapped below the thread's stack rather than on the platform or the frame size (the same binary answers differently between runs on Linux) — with no allocation site to read either way.

### Recipe 35 — Walk a JSON document you do not own

**In C#:** `foreach (var p in root.EnumerateObject())`, `element.TryGetProperty("delay_ms", out var d)`, `EnumerateArray()`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/json.cpp:recipe-35"
    ```

=== "Rust"

    As for Recipe 25: walking a document you do not own is `serde_json::Value`, matched on as an enum — the `is_structured` question becomes a `match` arm — and it is a dependency this crate does not take.

**Why it looks like this.** Recipes 25 and 26 mapped a document onto a
type you own; this is the other case, a document whose shape belongs to
somebody else — the host's project file, a vendor's telemetry — where you
walk what is there rather than declare what must be. `items()` is
`EnumerateObject`, a key and a value per pass, unpacked with
[Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency)'s
structured bindings; `contains` plus `at` is `TryGetProperty` split into
its two halves, and a field that may be absent lands in a
`std::optional` (Recipe 19) rather than in a sentinel — `value` covers
absence only, and it converts by the *default's* type, so present with
the wrong kind throws `type_error` and `3.5` against an `int` default
truncates (the trap below). A plain range-`for`
over a node is the generic walk: an array yields its elements, an object
its values, and `is_structured()` is the guard that stops a string, a bool or a
number from being iterated as a one-element sequence of itself — which
the library does, and which turns a recursive walk into Recipe 34's
`stack-overflow`. The walk's depth is the document's nesting, and the
parser will not refuse a deep one for you (its callback form takes a
depth), so on a hostile document it is the walk, not the parse, that
dies.
Keys iterate in sorted order, not file order, because the object is a
map — Recipe 25's point from the reading side. And `items()` is a *view*
of the document — Recipe 26's trap in loop form: `for (... :
json::parse(text).items())` keeps the range expression alive and not the
temporary its member call was made on, a `stack-use-after-scope` under
ASan until C++23, so the document is named first. Needs
`<nlohmann/json.hpp>` (`-isystem exercises/third_party`), `<optional>`,
`<string>`, `<vector>`, and `using json = nlohmann::json;`.

> [!WARNING]
> **Trap:** `get<int>()` is a `static_cast` per number kind — on `3.5` it returns `3` and on `3000000000` it wraps, with no error and no sanitizer opinion, while on `1e300` it is undefined behavior that UBSan reports from inside `json.hpp` — so a field that must be an integer is checked for kind with `is_number_integer()` and for range by you, and `is_number()` guards neither.

### Recipe 36 — Hash bytes

**In C#:** `SHA256.HashData(bytes)`, `Convert.ToHexString(hash)`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/crypto.cpp:recipe-36"
    ```

=== "Rust"

    Rust's standard library has no cryptography, by design. The ecosystem's answers are the RustCrypto crates (`sha2` here) and `ring`; a hash is a dependency, which is Chapter 27's decision, not this page's, and the crate stays dependency-free.

**Why it looks like this.** There is no `System.Security.Cryptography`:
the standard library ships no hash, no cipher and no random source
guaranteed fit for a key, which puts every one of them where
[Chapter 27](27-dependency-management.md#chapter-27--dependency-management)
put networking — a dependency you choose, add and build, and read as a
[Chapter 16](16-the-sdk-bestiary.md#chapter-16--the-sdk-bestiary) shape
when it arrives; this one is OpenSSL's libcrypto through its EVP
interface, a Shape 1 C API with an integer status from every operation,
and Chapter 27 names the alternatives. The rule is Chapter 27's, and
harder than C#'s because nothing is in the box: never write a primitive,
and prove the one you chose against a *published* vector — the harness
holds this function to NIST's digest of `abc` — since a hash that agrees
with itself proves nothing about whether it agrees with the .NET side.
Compare the bytes, not the strings: `Convert.ToHexString` is upper-case,
this `hex` is lower-case, and `ToHexStringLower` arrived in .NET 9. Needs
`<openssl/evp.h>` and a link against libcrypto — `pkg-config --cflags
--libs libcrypto`, which `build_all.sh` adds under its probe and
`check.sh` does not — `<cstdint>`, `<stdexcept>`, `<string>`,
`<string_view>`, `<vector>`. In CMake the same link is
`find_package(OpenSSL)` and `OpenSSL::Crypto` — [Appendix J](J-cmake-catalogue.md#appendix-j--the-cmake-catalogue)'s
entry.

> [!WARNING]
> **Trap:** `sha256(text)` hashes *bytes*, and a C# string is UTF-16 — `SHA256.HashData(Encoding.UTF8.GetBytes(s))` and this function agree, `SHA256.HashData(MemoryMarshal.AsBytes(s.AsSpan()))` does not, and both are correct hashes of different bytes; [Chapter 9](09-casts-conversions-and-strings.md#chapter-9--casts-conversions-and-strings)'s rule that the encoding is named applies to every byte that is hashed, signed or sealed.

### Recipe 37 — Seal bytes for a reader in C#

**In C#:** `new AesGcm(key, tagSizeInBytes: 16).Encrypt(nonce, plaintext, ciphertext, tag)` — and the layout of the file or message those three buffers go into, which C# never decided for you either

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/crypto.cpp:recipe-37"
    ```

=== "Rust"

    As for Recipe 36: `aes-gcm` from RustCrypto seals and opens the same nonce‖ciphertext‖tag envelope, and the layout stays the ICD it is here. A dependency this crate does not take.

**Why it looks like this.** The cipher is the easy half — `AesGcm` with
a 32-byte key is AES-256-GCM, and authenticated means a flipped byte is
a refusal rather than garbage,
[Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes)'s
value pole, so the return is an `optional` (a library that cannot even
start is the event pole, and throws, as in Recipe 36). The half nothing
decides for you is the envelope: `AesGcm.Encrypt` hands the C# side
three separate buffers and says nothing about how they travel, and the
moment your bytes must open on another machine, the nonce length, the
tag length and the order the three are written in are a wire format in
[Chapter 34](34-parse-this-capture.md#chapter-34--every-capture-rejected-as-malformed)'s
sense — documented offsets, and a published test vector as the oracle.
The comment above `seal` is that document — the 16-byte tag is one of
its rows, so the C# side hands `Decrypt` a 16-byte tag span, which the
.NET 8 constructor's `tagSizeInBytes` fixes — and the harness holds the
function to the GCM specification's own test cases 13 and 14, because a
round trip proves only that `seal` and `open_sealed` agree with each
other. When the envelope is wrong, the C# side's refusal is
`AuthenticationTagMismatchException` (.NET 8; a bare
`CryptographicException` before), and it names nothing — the envelope
is the first suspect. Needs `<openssl/evp.h>` and libcrypto as Recipe
36, `<array>`, `<algorithm>`, `<memory>`, `<optional>`.

> [!WARNING]
> **Trap:** a nonce reused under one key breaks GCM outright — not weakens, breaks — and a counter that restarts at process start, or a `std::rand()` seeded from the clock, will reuse one; the nonce is twelve bytes from the library's own generator (`RAND_bytes`, which the harness uses), travels in the clear at the front of the envelope, and is never a secret and never repeated — and nothing will tell you when it was.

### Recipe 38 — Save a file without losing the old one

**In C#:** `File.Move(tmp, path, overwrite: true)` (.NET Core 3.0+), or `File.Replace(tmp, path, null)` once `path` exists — the write-then-move everyone ends up writing by hand around `File.WriteAllText`, once a customer has sent in a half-written preferences file

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/files.cpp:recipe-38"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/files.rs:recipe-38"
    ```

**Why it looks like this.** Recipe 9 writes in place, which is fine until
the process dies halfway — a crash, a host that kills the plug-in — and
leaves a file that is neither the old one nor the new one. The fix is two
files and one step: the bytes go to a sibling in the same directory,
flushed and checked, and `rename` moves the name onto them in one step
that POSIX `rename(2)` promises is atomic over an existing file (on
Windows the same call is a replace-existing `MoveFileEx`, which every
atomic-save library there relies on without the documentation saying the
word) — so any reader sees the old contents or the new, never a torn
middle, and a crash before the rename leaves the old file whole and a
`.tmp` beside it that the next save overwrites (unlike `File.Replace`,
this also works when there is no old file yet). That covers the process
dying; a power cut is one step further, because the operating system may
still hold the temp file's bytes in memory when the rename lands, and the
`fsync` that pins them to disk first has no standard-library spelling —
POSIX `fsync`, Windows `FlushFileBuffers` — so this recipe is crash-safe
and one call short of power-safe. Two smaller things Recipe 9 supplies:
its checked `flush` is why the bytes are complete, and its `ofstream`
closing itself at the end of the call
([Chapter 1](01-ownership-and-raii.md#chapter-1--ownership-and-raii)) is
why the handle is gone before `rename` runs — on Windows a file you still
hold open cannot be renamed. *Same directory* is load-bearing: a `rename`
across volumes is a copy that is not atomic, and the two POSIX standard
libraries report it as `std::errc::cross_device_link` rather than doing it
quietly (the harness asserts that on Linux, where the CI runner has a
second volume to try) — while MSVC's does the opposite, passing
`MOVEFILE_COPY_ALLOWED` so that across volumes it copies and deletes,
silently and non-atomically. Same directory is how you never find out
which you got. Needs `<filesystem>`, `<string>`, and Recipe 9. **In Rust** it is the same two calls, `fs::write` to the sibling name and `fs::rename` over the target, and `?` on each is the part C# hid inside `File.Replace`.

> [!WARNING]
> **Trap:** the rename gives the *name* a new file, so anything holding the old one open keeps the old one — on POSIX a stale inode no path reaches any more; on Windows the rename itself fails while a reader holds the target open without `FILE_SHARE_DELETE`, which a default `FileStream` does not — and the harness's own judge is that inode: a save that rewrote the file in place would pass every other check and still tear.

### Recipe 39 — Create, copy, move and delete, and a whole tree

**In C#:** `Directory.CreateDirectory(dir)`, `File.Copy(src, dst, overwrite: true)`, `File.Move(src, dst)`, `Directory.Delete(dir, recursive: true)`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/paths.cpp:recipe-39"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/paths.rs:recipe-39"
    ```

**Why it looks like this.** Four calls, four C# names, and two places
the defaults differ — in opposite directions. `create_directories` is
`Directory.CreateDirectory` exactly: parents made, an existing directory
not an error. `copy_file` is `File.Copy`, defaults included: with no
options it refuses an existing target, throwing `filesystem_error` with
`errc::file_exists`, and `overwrite_existing` is the line C# spells
`overwrite: true`. `rename` is `File.Move` with the *opposite* default:
an existing target is replaced — Recipe 38's atomic replace — where
`File.Move` throws `IOException` until you pass `overwrite: true`, so the
one call that quietly destroys a file here is the one that would have
thrown in C#. `remove_all` is `Directory.Delete(..., recursive: true)`
and `File.Delete` in one — it returns the count, and where
`Directory.Delete` throws for a path that was never there, it returns
zero. The rest of the family maps by name: `file_size`,
`temp_directory_path`, and `last_write_time`, which hands back a
`file_time_type` that C++17 cannot portably print or convert — compare
two of them, and leave formatting to C++20's `clock_cast` or the
platform. Every one ships as
[Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes)'s
pair, throwing or `error_code`. Needs `<filesystem>`, `<cstdint>`, and
`namespace fs = std::filesystem;`. **In Rust** `fs::copy` overwrites and `fs::rename` replaces without being asked, exactly as here, and `remove_dir_all` on a missing directory is an `Err` the recipe turns back into `Ok` — the one place the C++ and Rust defaults differ.

> [!WARNING]
> **Trap:** `dir / name` with an empty `name` is `dir/` — the separator and nothing after it — so `remove_all(dir / entry)` where `entry` came back empty from a lookup deletes the *directory itself* and everything in it, not one entry, and compiles clean; `Path.Combine(dir, "")` is `dir` by a shorter spelling and the same deletion, and the harness asserts this one: two files and a subdirectory gone, and the directory with them.

### Recipe 40 — Notice a file changed

**In C#:** `var w = new FileSystemWatcher(dir, "settings.json"); w.Changed += OnChanged; w.EnableRaisingEvents = true;`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/watch.cpp:recipe-40"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/watch.rs:recipe-40"
    ```

**Why it looks like this.** The standard library has no watcher, and the
native ones — `inotify` on Linux, FSEvents and `kqueue` on macOS,
`ReadDirectoryChangesW` on Windows — are three shapes with three
coalescing rules, which is where `FileSystemWatcher`'s folklore comes
from (two `Changed` events for one save, an `Error` when the buffer
overflows) and why a plug-in that needs one takes a library (efsw is the
small portable one) or, better, the host's own change notification if the
SDK offers it. A poll is the spelling every platform shares: Recipe 16's
worker thread — and Recipe 16's trap with it, because a
`FileSystemWatcher` you forgot to dispose kept raising into a target the
GC kept alive, where a watcher outliving its target here is a callback
into freed memory, which is what the join in the destructor and the
harness's silence after the brace are for — a `Stamp` of time, size and
presence per interval, and a callback delivered *on that thread*, so
everything it touches is
[Chapter 29](29-concurrency.md#chapter-29--concurrency)'s shared state,
and in a plug-in the callback posts to the host's queue
([Chapter 38](38-the-bridge-out.md#chapter-38--the-bridge-out)) rather
than calling the SDK. Absence is read through
[Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes)'s
`error_code` overloads, and *read*, because a file can vanish between the
three calls and a stamp assembled from their error returns is a change
that never happened — a missing file is a state the watcher must report,
not an exception on a thread with no handler. The comparison is `!=`,
not `>`: a backup restored over the file with its timestamps preserved
carries an *older* time, and a watcher that asked "is it newer?" would
sleep through it — the harness restores one and asserts the wake. And a
poll can still catch a file half-written by an in-place save and report
one change twice, exactly `FileSystemWatcher`'s double event, so the
callback should be safe to run twice. Needs `<atomic>`, `<chrono>`,
`<cstdint>`, `<filesystem>`, `<functional>`, `<system_error>`, `<thread>`,
`<utility>`. **In Rust** the poll is `fs::metadata`, which answers all three questions in one call and reports absence as an `Err` the watcher treats as a state; the OS-notification version is the `notify` crate, and it carries the same two traps.

> [!WARNING]
> **Trap:** a poll reads the timestamp at the filesystem's resolution, not the clock's — nanoseconds on APFS and ext4, hundreds of them on NTFS, whole seconds on HFS+ and many network shares, two on FAT — so two same-size writes inside one tick are one event or none; and many editors save by Recipe 38's rename, so a watch on the *inode* — what `inotify` attaches its watch to, and what `kqueue`'s open descriptor names — is watching a ghost after the first save; watch the path, as this one does.

### Recipe 41 — Call an HTTP endpoint

**In C#:** `var text = await http.GetStringAsync(url);` — one call, and one `HttpRequestException` for the transport's failures and the server's non-success codes alike (the timeout alone arrives as a `TaskCanceledException`)

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/http.cpp:recipe-41"
    ```

=== "Rust"

    The standard library has TCP sockets and nothing above them: an HTTP client is `ureq` (blocking, small) or `reqwest` (async, large), and the two verdicts — the transport's and the server's — come back as an `Err` and a status code respectively. A dependency this crate does not take.

**Why it looks like this.** There is no `HttpClient` because there are no
sockets ([Chapter 27](27-dependency-management.md#chapter-27--dependency-management)),
and the library the ecosystem reaches for is libcurl — which arrives as
[Chapter 16](16-the-sdk-bestiary.md#chapter-16--the-sdk-bestiary)'s Shape 2
with every idiom intact: an opaque `CURL*` from `curl_easy_init` that
`curl_easy_cleanup` must reach exactly once, options set one call at a
time, an integer status from `curl_easy_perform`, and the write callback
carrying the `void*` you handed it, which is the trampoline
[Chapter 18](18-exercise-the-device-sdk.md#chapter-18--exercise-the-device-sdk)
built — called once per chunk, so it appends and never assigns. The two
verdicts are the part `GetStringAsync` hid: `CURLcode` says whether the
wire delivered anything (a name that did not resolve, a timeout, a
certificate it would not trust), and `CURLINFO_RESPONSE_CODE` is what the
server thought of the request — `CURLE_OK` with a 500 is a *successful*
transfer of an error page, and `ok()` reads both; `CURLOPT_FOLLOWLOCATION`
is there because without it a 301 is a *successful* transfer of a redirect
page, where `HttpClient` follows by default. The `.count()` is Recipe
30's: the duration becomes libcurl's bare integer on the one line next to
the `_MS` option. `curl_global_init` runs once per process before any
thread exists — the init entry point in a plug-in, never a static
initializer ([Chapter 32](32-it-crashes-on-exit.md#chapter-32--crash-on-exit))
— and a POST is the same handle turned around, Recipe 46. The harness
needs no network and
has two halves: a `file://` fixture, the one URL scheme with nothing
behind it, exercises the callback and the transport's error path, and a
small loopback server — POSIX sockets, since the standard library
has none — answers with a redirect to follow, a 500 whose body is an error
page, and a stall the deadline cuts short, so both verdicts are judged and
the timeout's unit with them. Needs `<curl/curl.h>` and a link against libcurl —
`pkg-config --cflags --libs libcurl`, which `build_all.sh` adds under its
probe and `check.sh` does not — `<chrono>`, `<memory>`, `<stdexcept>`,
`<string>`. In CMake the same link is `pkg_check_modules(... IMPORTED_TARGET
libcurl)` and `PkgConfig::CURL` — [Appendix J](J-cmake-catalogue.md#appendix-j--the-cmake-catalogue)'s
entry.

> [!WARNING]
> **Trap:** a green `CURLcode` says the bytes arrived, not that they are the answer — the body of a 404 is an HTML page that parses as JSON about as well as it reads, and a caller that checked only `perform`'s return will feed it to Recipe 26 and file the resulting `parse_error` under "the server is flaky".

### Recipe 42 — Open a local database and run a query

**In C#:** `using var conn = new SqliteConnection("Data Source=cache.db"); conn.Open(); using var cmd = conn.CreateCommand(); cmd.CommandText = "SELECT ..."; cmd.Parameters.AddWithValue("$t", 1); using var reader = cmd.ExecuteReader(); while (reader.Read()) ...` — Microsoft.Data.Sqlite, or Dapper over it

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/database.cpp:recipe-42"
    ```

=== "Rust"

    SQLite from Rust is `rusqlite`, a safe wrapper over the same C API: `Connection`, a prepared `Statement` that finalizes on `Drop`, and a `Transaction` that rolls back on `Drop` unless committed — the three shapes this recipe writes by hand. A dependency this crate does not take.

**Why it looks like this.** There is no ADO.NET
([Chapter 27](27-dependency-management.md#chapter-27--dependency-management)),
and the native default for local storage is SQLite through its C API —
[Chapter 16](16-the-sdk-bestiary.md#chapter-16--the-sdk-bestiary)'s Shape
1 masterclass, which means [Chapter 17](17-exercise-the-fakesdk.md#chapter-17--exercise-the-fakesdk)
already trained every line above: a status from every call, results
through pointers, and a matching release for everything you are handed,
which three RAII types make structural — one per thing SQLite hands you
and wants back: `sqlite3_close` for the connection, `sqlite3_finalize`
for the statement, `ROLLBACK` for the transaction nobody committed. Two
things the chapter could only foreshadow arrive here for real.
`sqlite3_step` answers with **100** for a row and **101** for done — two
successes, neither zero — so
[Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes)'s
warning that "success is usually zero" is not a contract is the shape of
the loop. `SQLITE_BUSY` is that chapter's drill scenario 3 — a documented
steady-state condition the drill files under *value* — and the recipe
throws it anyway, deliberately: with one connection and no other process
on the file, busy is exceptional here; a plug-in sharing the file with a
host decides the way the drill does, with `sqlite3_busy_timeout` on the
connection or a retry around `step`, before it reaches for `throw`. And
`sqlite3_column_text` is the loan
[Chapter 33](33-here-is-the-report.md#chapter-33--a-value-reads-zero-after-hot-plug)
quoted as its in-the-wild example — good until the next step, reset or
finalize — so the accessor copies out on the spot; `SQLITE_TRANSIENT` is
the same question asked in the other direction, whether SQLite may keep
*your* pointer, and the harness binds a temporary that dies before the
step to make the answer load-bearing. A prepared statement is what
`SqliteCommand` builds on every `ExecuteReader` (and `Prepare` builds
early): compiled once, rebound through `reset` — every parameter, because
a reset keeps the old bindings. The close code at the end is the recipe's
leak detector: a database with a live statement returns `SQLITE_BUSY`
from `sqlite3_close`, the job `FakeSdk_LiveAllocations` did in Chapter 17
— and a `unique_ptr`'s deleter returns nothing, which is why the listing
ends with a function that takes the handle back and closes it by hand;
a product's shutdown path should do the same and log the verdict, or it
has no leak detector at all. sqlite_orm and SOCI wrap this; most native
codebases speak it raw, and reading it is cheaper than a wrapper nobody
else on the team uses. Needs `<sqlite3.h>` and a
link against libsqlite3 — `pkg-config --cflags --libs sqlite3`, which
`build_all.sh` adds under its probe and `check.sh` does not — `<memory>`,
`<stdexcept>`, `<string>`, `<vector>`. In CMake the same link is
`find_package(SQLite3)` — [Appendix J](J-cmake-catalogue.md#appendix-j--the-cmake-catalogue)'s
entry.

> [!WARNING]
> **Trap:** `sqlite3_close` returning `SQLITE_BUSY` at shutdown is a statement somebody never finalized — Chapter 35's still-live-at-unload, one library over — and the tempting fix, `sqlite3_close_v2`, does not fix it: it defers the close until the last statement is finalized, which for a leaked one is never, so the handle, its open file and any lock a `SELECT` abandoned mid-rows was holding outlive your plug-in in the host's process; and a second close of the same handle is `SQLITE_MISUSE` returned into a deleter that discards it, read inside a library no sanitizer instruments.

### Recipe 43 — Share a buffer with another process

**In C#:** `using var mmf = MemoryMappedFile.CreateOrOpen("frames", size); using var view = mmf.CreateViewAccessor(); view.Write(0, ref frame);` — the runtime picks the platform API, keeps the mapping alive, and copies the struct's bytes in exactly as it laid them out, checked against nothing on the other side — which worked, because the other side was usually .NET too (and `CreateOrOpen` is `[SupportedOSPlatform("windows")]`: a *named* map throws `PlatformNotSupportedException` everywhere else, which is its own hint about how platform-shaped this is)

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/shm.cpp:recipe-43"
    ```

=== "Rust"

    The standard library has no shared memory. `memmap2` maps a file (or `/dev/shm` on Linux), `shared_memory` wraps the named-segment calls, and the layout rules — `#[repr(C)]`, atomics that are lock-free, a version field first — are the same ICD discipline as here. A dependency this crate does not take.

**Why it looks like this.** No library, because the platform is the
dependency: POSIX `shm_open` plus `mmap` on Linux and macOS, a
pagefile-backed `CreateFileMapping` plus `MapViewOfFile` on Windows, and
the first listing in the cookbook with no portable spelling at all —
Recipes 29 and 38 guard one call, this one guards every call, since no
standard one exists (Boost.Interprocess is the portable wrapper, and the
price of it is Boost). The class is Recipe 7 twice — an object handle and
a view, each released on every path in reverse; Windows has no `O_EXCL`,
so its create branch reads `ERROR_ALREADY_EXISTS` back to refuse a live
name the way POSIX's flag does — and the lesson that makes it this book's
is in the struct, not the class: **a shared region is a wire format**.
The bytes are read by a process with its own compiler, its own build and
its own address space, so
[Chapter 30](30-authoring-an-abi-boundary.md#chapter-30--authoring-an-abi-boundary)'s
one rule applies with
[Chapter 34](34-parse-this-capture.md#chapter-34--every-capture-rejected-as-malformed)'s
extension — fixed-width fields, a version first, no `std::string`, no
pointer (an address in *your* process) — and the three `static_assert`s
are [Chapter 41](41-templates-you-will-write.md#chapter-41--templates-you-will-write)'s
judge on what they *can* hold: the size, the absence of a vtable, the
lock-free atomic. The pointer and the `std::string` they cannot refuse,
since both are standard-layout, and the fork does not reliably catch them
either — a child's address read by the parent is garbage, or correct
until the day the mapping lands elsewhere; that half of the rule is yours
to keep. (Not `is_trivially_copyable`, which an atomic fails on one
standard library and passes on two.) This is also the overlay
[Chapter 34](34-parse-this-capture.md#chapter-34--every-capture-rejected-as-malformed)
bans for a captured wire, and here it is the tool: the region *is* the
object's storage, the atomic must be operated in place, and the second
view is read through the cast — the standard has no model of a second
mapping, C++23's `std::start_lifetime_as` is its spelling for one, and
the compiler cannot see through `mmap`. The counter is the whole of the
synchronization: a `std::atomic` that is lock-free on every target here,
written with release after the payload and read with acquire before it,
because an atomic that needed a lock would hold one that exists in one
process only (the standard *recommends* that lock-free also mean
address-free, which is the property a second process needs); a
process-shared mutex (`pthread_mutexattr_setpshared`, a named Win32
mutex) is the next step and not this recipe. The reader's wait is bounded
([Chapter 38](38-the-bridge-out.md#chapter-38--the-bridge-out)'s judge),
and the harness is the one place in this book that forks: the child maps
by name, writes, bumps, and leaves, and the parent asserts the frame — on
Windows the `buildlab-msvc` job maps one object twice in one process,
which proves the mechanism and states the cross-process half as
unverified there. Needs `<atomic>`, `<cstdint>`, `<string>`,
`<type_traits>`; `<sys/mman.h>`, `<fcntl.h>`, `<unistd.h>` on POSIX (and
`-lrt` before glibc 2.34); `<windows.h>` on Windows.

> [!WARNING]
> **Trap:** the name outlives every process that mapped it — close every handle, exit, and the name is still there holding the last frame, visibly on Linux as `/dev/shm/name` and with no path to list at all on macOS, until someone calls `shm_unlink` (a harness killed on an assertion leaves one behind) — and ThreadSanitizer instruments one process, so a race between two is invisible to every tool in the book, Finding 10's family with a process boundary through it. Two smaller ones the harness meets: macOS caps the name at 31 characters and allows `ftruncate` on the object exactly once (a second returns `EINVAL`).

### Recipe 44 — Match a pattern

**In C#:** `Regex.IsMatch(id, @"^sensor(\d+)$")`, `Regex.Match(id, @"^sensor(\d+)$").Groups[1].Value`, `Regex.Replace(text, @"\d+", "#")`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/strings.cpp:recipe-44"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/strings.rs:recipe-44"
    ```

**Why it looks like this.** `std::regex` is the `Regex` class with the
static helpers removed: the object *is* the compiled pattern, so the
shape that matters is where it lives. `Regex.IsMatch(s, pattern)` hides
a cache of compiled patterns behind the static call; here nothing caches
for you, and a `std::regex` built inside the function it serves is
parsed and compiled on every call — a function-local `static const`
builds it once, on first use, thread-safely since C++11
([Chapter 32](32-it-crashes-on-exit.md#chapter-32--crash-on-exit)'s
shape). `regex_match` is `IsMatch` with `^` and `$` built in — the
pattern keeps them so it reads as the C# one — and `regex_search` is the
unanchored one; `smatch` is the `Match` object, and `m[1]` is
`Groups[1]`: a pair of iterators *into the string you matched*,
[Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency)'s
view, so the digits are copied out on the spot. The class is written
`[0-9]` where C# wrote `\d` because that is all `\d` means here — bytes,
the ten ASCII digits, never a Unicode category — where .NET's `\d` is
`\p{Nd}` and takes every script's digits
([Chapter 9](09-casts-conversions-and-strings.md#chapter-9--casts-conversions-and-strings)'s
rule: a `std::string` is bytes). The dialect is ECMAScript, close enough
to .NET's for the everyday subset — except `$`, which here does not
match before a final `\n`. Needs `<regex>`, `<optional>`, `<charconv>`,
`<string>`. **In Rust** this particular pattern needs no regex at all — `strip_prefix` is the anchor and the literal, and an all-digits check is the class — which is worth knowing before reaching for the `regex` crate, whose compile-once rule is spelled `OnceLock` or `LazyLock` rather than a function-local static.

> [!WARNING]
> **Trap:** `std::regex` is slow and it allocates — on this machine a match through the `static const` above costs about 800 nanoseconds and eleven heap allocations, where `starts_with` plus Recipe 19's `from_chars` on the same input costs a few nanoseconds and none, which the harness counts with [Chapter 36](36-the-host-stutters.md#chapter-36--dropouts-with-the-plug-in-loaded)'s replaced `operator new`; and one hostile line against a pattern with nested repetition backtracks for seconds under libstdc++ and, under libc++, throws `std::regex_error` out of `regex_match` in milliseconds, which this recipe does not catch — so it belongs in a config parser and never on the per-sample path, and a regex that must be fast is a [Chapter 27](27-dependency-management.md#chapter-27--dependency-management) dependency, RE2 or PCRE2.

### Recipe 45 — Trim, compare ignoring case, prefix and suffix

**In C#:** `s.Trim()`, `string.Equals(a, b, StringComparison.OrdinalIgnoreCase)`, `s.StartsWith("sensor", StringComparison.Ordinal)`, `s.EndsWith(".txt", StringComparison.Ordinal)`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/strings.cpp:recipe-45"
    ```

=== "Rust"

    ```rust
    --8<-- "exercises/cookbook/rust/src/strings.rs:recipe-45"
    ```

**Why it looks like this.** Four one-liners C# has and C++17's
`std::string` does not, each with the same shape: a `string_view` in, so a literal, a
`std::string` and a substring all bind without a copy
([Appendix H](H-choosing.md#appendix-h--choosing-signatures-containers-and-storage)'s
view branch). `trim` hands back a view rather than a new string — free,
and honest about what `Trim` allocated for you — so its result lives as
long as its argument and no longer, which is
[Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency)'s
dangling view the moment the argument was a temporary; copy into a
`std::string` where the trimmed text must outlive the line. `Trim`
strips every Unicode white-space character where `blank` here is four
bytes, so widen it if a no-break space (`C2 A0` in UTF-8) can reach
you. The `find_first_not_of` / `find_last_not_of` pair is the idiom, and
the `npos` check comes first because `substr(npos)` throws
`out_of_range`: without it an all-blank input would throw where `Trim`
returns `""`.
The case-insensitive compare is ordinal and byte-wise — `tolower` on an
`unsigned char`, [Chapter 19](19-exercise-the-word-counter.md#chapter-19--exercise-the-word-counter)'s
cast, because a negative `char` is undefined behavior there — so it is
`OrdinalIgnoreCase` for ASCII and *not* for anything else: `ü` and `Ü`
differ as bytes, and the harness asserts that they do. `starts_with` and
`ends_with` are C++20 members of `string` and `string_view`; on C++17
these two lines are them — and ordinal always, where a bare
`s.StartsWith("x")` in .NET is culture-sensitive, which is what the
analyzers nag about. Needs `<cctype>`, `<string_view>`. **In Rust** `trim` returns a `&str` into its argument the same way, and the lifetime the C++ comment asks you to remember is one the borrow checker refuses to let you forget; `eq_ignore_ascii_case` puts the ASCII limitation in the name.

> [!WARNING]
> **Trap:** `auto t = trim(read_line());` is a view of a string that died at the semicolon — a `stack-use-after-scope` or `heap-use-after-free` under ASan, and plausible text until then; name the string first, or have your own `trim` return a `std::string` if callers keep the result.

### Recipe 46 — Post a JSON body and read a JSON reply

**In C#:** `var resp = await http.PostAsJsonAsync(url, body); resp.EnsureSuccessStatusCode(); var reply = await resp.Content.ReadFromJsonAsync<Reply>();`

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/http.cpp:recipe-46"
    ```

=== "Rust"

    As for Recipe 41: `ureq::post(url).send_json(body)` and `into_json()` on the reply, with the same two verdicts kept apart; `serde_json` as in Recipe 25. Dependencies this crate does not take.

**Why it looks like this.** Recipe 41 with the request turned around and
Recipe 25 on both ends of it. The header list is one more C handle with
a matching free — `curl_slist_free_all`, Recipe 7's shape for the
second time on one page — and `CURLOPT_POSTFIELDS` is the reason the
serialized body has a name: libcurl keeps the *pointer*, not a copy, and
reads the bytes during `perform`
([Chapter 33](33-here-is-the-report.md#chapter-33--a-value-reads-zero-after-hot-plug)'s
loan, with the SDK on the borrowing side). `PostAsJsonAsync` set the
content type for you; here it is the one header the list carries,
because without it libcurl's default is
`application/x-www-form-urlencoded`, and a JSON body under that header
is a form with one nonsense field to any server that decodes forms. A
body of a megabyte or more also gets `Expect: 100-continue`, and a
server that never answers it costs a one-second wait per request —
`curl_slist_append(headers, "Expect:")` removes it. The reply is
[Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes)'s
decision made three times: the transport's verdict and the server's are
Recipe 41's two, and the third — is this JSON at all — is a value too,
because a maintenance page with a 200 on it is an answer the server
gave, not an event; so `json_reply` parses with exceptions off and
returns `nullopt`, where Recipe 26's `parse` throws because
`load_config` is the deepest frame and a broken config abandons the
whole load — `json_reply` stands at the edge, where Chapter 8 turns a
throw into a value. What comes back is Recipe 26's document, and the
required key is still `at()`, which throws `out_of_range` naming it.
`EnsureSuccessStatusCode` and `ReadFromJsonAsync` folded the three
verdicts into `HttpRequestException` for the first two and
`JsonException` for the third (with Recipe 41's `TaskCanceledException`
for the timeout); `HttpResult` keeps the first two apart and the
`optional` carries the third, so the everyday call is `r.ok()`, then
`json_reply(r)`, and the reason for a `nullopt` is still in `r`. It is
also the shape of a call to a hosted language model — JSON in, JSON
out, a vendor's endpoint in the URL, its schema in the body and its
credential as one more entry on the header list — and
[Chapter 27](27-dependency-management.md#chapter-27--dependency-management)
says where in a plug-in such a call runs. Needs
`<curl/curl.h>` and libcurl as Recipe 41, `<nlohmann/json.hpp>` as
Recipe 25, `<chrono>`, `<memory>`, `<optional>`, `<string>`.

> [!WARNING]
> **Trap:** `curl_easy_setopt(easy, CURLOPT_POSTFIELDS, body.dump().c_str())` compiles, and the temporary dies at the semicolon — libcurl reads dead memory during `perform`: a `heap-use-after-free` under ASan, or `stack-use-after-scope` for a body short enough to fit the small-string buffer, which is also the body that *works* without ASan until the payload grows; name the string, and keep it alive until `perform` returns.

### Recipe 47 — Derive a key

**In C#:** `Rfc2898DeriveBytes.Pbkdf2(password, salt, iterations, HashAlgorithmName.SHA256, 32)` for a password; `HKDF.DeriveKey(HashAlgorithmName.SHA256, secret, 32, salt, info)` for a secret that already has entropy

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/crypto.cpp:recipe-47"
    ```

=== "Rust"

    As for Recipe 36: `pbkdf2` and `hkdf` from RustCrypto, each a few lines against the same published vectors. Dependencies this crate does not take.

**Why it looks like this.** Recipe 37 took a `Key` and never said where
one comes from; these are the two answers, and which one is a question
about the input rather than the output. A password has almost no
entropy, so PBKDF2 spends *time* on it — `iterations` rounds of
HMAC-SHA-256, the count chosen so one derivation costs tens of
milliseconds on the machine that will run it rather than chosen as a
number, since any figure quoted today is too few in a few years — to
make each guess cost the attacker what it cost you; a secret that already has
entropy (a key agreed elsewhere, a master key from the platform's store)
only needs *condensing and separating*: HKDF extracts a uniform key from
it and expands that to the length wanted, in a few hashes, with `info`
naming the purpose so one secret yields different keys for different
jobs. Both can produce any length; the 32 bytes here are Recipe 37's,
because that is the key these two exist to feed. The two shapes are two ages of the same
library: `PKCS5_PBKDF2_HMAC` is one call in the old style, and HKDF is
the `EVP_PKEY` derivation context, the spelling that still builds on
1.1.1 (OpenSSL 3 also fetches a KDF by name, the way Recipe 48 fetches
its MAC) — a [Chapter 16](16-the-sdk-bestiary.md#chapter-16--the-sdk-bestiary)
Shape 2 handle, set up one option at a time, with a status from every
call and one free, which is why the chain of `!= 1` reads the way
Recipe 37's `seal` does: one `||` per call, the first failure ending
the chain. The reflex to check at the door is C#'s defaults: the older
`new Rfc2898DeriveBytes(password, salt)` chose SHA-1 and 1000
iterations for you, which is where a drifted count on the C# side
usually comes from, and the C++ call has no defaults at all — every
parameter is yours to write down. Salt, iteration count and `info` are not
secrets, and they travel: a key that must be re-derived on the C# side
needs the same three, so they are a wire format in
[Chapter 34](34-parse-this-capture.md#chapter-34--every-capture-rejected-as-malformed)'s
sense, written down next to the envelope of Recipe 37. The harness
holds both functions to published vectors — RFC 7914's PBKDF2-HMAC-SHA-256
cases and RFC 5869's first HKDF case — because a derivation that agrees
with itself proves nothing about whether it agrees with .NET's — and
then the Trap as a value: the same password one iteration off, and
Recipe 37 refuses to open. Needs
`<openssl/evp.h>`, `<openssl/kdf.h>` and libcrypto as Recipe 36,
`<array>`, `<memory>`, `<string_view>`, `<vector>`.

> [!WARNING]
> **Trap:** the iteration count is part of the key — change it on one side, or let a config default drift, and the two sides derive different keys from the same password with no error anywhere, only Recipe 37's `open_sealed` returning `nullopt`; store the count and the salt beside the ciphertext, as part of the envelope.

### Recipe 48 — Sign and verify bytes

**In C#:** `new HMACSHA256(key).ComputeHash(data)` (or `HMACSHA256.HashData(key, data)`), and `CryptographicOperations.FixedTimeEquals(expected, tag)` to check one

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/crypto.cpp:recipe-48"
    ```

=== "Rust"

    As for Recipe 36: `hmac` with `sha2`, and its `verify_slice` is the constant-time comparison this recipe's `==` warns about. Dependencies this crate does not take.

**Why it looks like this.** An HMAC is the answer to a question Recipe
37 does not ask — *did the bytes I can read come from someone holding
the key?* — for the file or the message that is not secret but must not
be forged: a settings file the plug-in wrote and must trust on re-read,
telemetry for a backend of your own, a request between two processes of
yours. `EVP_MAC` is OpenSSL 3's
spelling: fetch the algorithm by name, make a context, initialise it with
the key and a parameter list naming the digest — the `OSSL_PARAM` array
is the C API's way of passing options without a function per option,
and its string slot is a writable `char*` by declaration, which is why
the name sits in a local array rather than a literal — then update and
finalise, a handle with a status from every call, one more time. The
verifier is the half that matters. `==` on two vectors stops at the
first differing byte, and how long it took is something an attacker can
measure, given enough samples, even over a network; `CRYPTO_memcmp` compares every byte whatever
the answer, which is what `FixedTimeEquals` exists for in .NET and what
`SequenceEqual` is not. The harness holds the function to RFC 4231's
vectors and then to its own verifier: a flipped byte, a wrong key, a
changed message and a short tag all refuse. Needs `<openssl/evp.h>`,
`<openssl/core_names.h>`, `<openssl/params.h>`, `<openssl/crypto.h>` and
libcrypto 3 as Recipe 36, `<memory>`, `<vector>`.

> [!WARNING]
> **Trap:** `verify_hmac_sha256` on a licence blob compiles, runs and verifies — and the key that verifies is the key that signs, so the plug-in checking the licence on the customer's machine carries everything needed to forge one; when the verifier must not be able to sign, that is a signature (Ed25519, `EVP_DigestSign` with a private key), a different recipe with a different key shape.

### Recipe 49 — Read a large file without copying it

**In C#:** `using var mmf = MemoryMappedFile.CreateFromFile(path, FileMode.Open, null, 0, MemoryMappedFileAccess.Read); using var view = mmf.CreateViewAccessor(0, 0, MemoryMappedFileAccess.Read);` — or, the reflex, `File.ReadAllBytes(path)`, fine until the file was the size of the machine's memory; the mapped form pages the file in as it is touched, and the runtime holds the file and mapping handles until the `using`s end, where the recipe closes both as soon as the view exists

**The recipe:**

=== "C++"

    ```cpp
    --8<-- "exercises/cookbook/files.cpp:recipe-49"
    ```

=== "Rust"

    The standard library has no memory mapping. `memmap2::Mmap` is the view, `unsafe` because the file can change under it — the `SIGBUS` this recipe's trap names is why the constructor is unsafe there — and the honest std-only alternative is `std::fs::read`, which is the copy this recipe exists to avoid. A dependency this crate does not take.

**Why it looks like this.** Recipe 1 copies the file into a `std::string`,
the right shape for a config and the wrong one for a capture, a log or a
media file: the copy costs heap allocations the size of the file and a
read of every byte before the first is looked at. A mapping asks the OS
to make the file's pages appear in the process's address space as they
are touched — the same `mmap` and `MapViewOfFile` as Recipe 43, with a
file where that recipe had a name, and read-only, private, so the file
cannot change through the view. The class is Recipe 7 for a view: the
descriptor is closed the moment the mapping exists, because the mapping
holds its own reference to the file — which is also why the file can be
deleted under a live mapping and the bytes still read, which the harness
asserts on every platform it runs on: on POSIX by design, and on Windows
because the STL's `remove` asks for POSIX delete semantics on NTFS (the
older `DeleteFile` refused a mapped file with `ERROR_USER_MAPPED_FILE`,
which is what a hand-rolled delete still meets). The empty file is the branch a first draft lacks: `mmap` of zero bytes is
`EINVAL` and `CreateFileMapping` of an empty file fails outright, so an
empty file is an empty view, not an exception. `bytes()` is a
`string_view`, [Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency)'s
non-owning window with that chapter's rule attached: valid exactly as
long as the `MappedFile` is, and a view kept past the object reads
unmapped memory, which ASan reports as a `SEGV on unknown address` with
no allocation site — the pages were the kernel's, never the allocator's —
or, if the allocator has since reused the range, as an overflow on some
unrelated heap object. The other side of that: a `munmap` left out of
the destructor is a leak no sanitizer counts, which is the destructor's
whole reason to exist. The harness maps four megabytes, compares every byte against Recipe 1's
copy, and — [Chapter 36](36-the-host-stutters.md#chapter-36--dropouts-with-the-plug-in-loaded)'s
instrument — counts heap allocations across the mapping with a replaced
`operator new`: zero, which is the recipe's whole claim over
`ReadAllBytes` (both forms of `operator new` are replaced, because under
ASan the array form does not route through the scalar one, and a copy
made with `new char[]` would otherwise count as zero). Needs
`<filesystem>`, `<stdexcept>`, `<string>`, `<string_view>`; `<sys/mman.h>`, `<sys/stat.h>`,
`<fcntl.h>`, `<unistd.h>` on POSIX; `<windows.h>` on Windows.

> [!WARNING]
> **Trap:** a file that shrinks while it is mapped — another process truncating the log you are reading — is, on Linux, a `SIGBUS` on the first touch of a page past the new end: plain memory, no allocation site, none of Chapter 31's shapes, and no sanitizer names it; on macOS the same read completes with the old byte — `scripts/check_platform_claims.sh` holds each platform to its own answer. Map files nobody else writes, or copy what you need out of the view before anyone can — a mapping is not a copy, and the bytes change under you if the writer keeps writing.
