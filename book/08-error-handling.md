## Chapter 8 — Error Handling: Exceptions and Error Codes

C# is exceptions everywhere. C++ is split-brained: exceptions exist, but large parts of the ecosystem — most C-flavored SDKs, OS APIs, and plug-in interfaces — use C-style **error codes**. You need both. You also need the thing no C# developer has ever had to decide, because the language decided it for them: *which mechanism a given failure deserves*. That decision is the second half of this chapter.

```cpp
// C++ exceptions - familiar, with differences:
try {
    auto w = LoadWidget(path);
} catch (const std::exception& e) {   // catch by CONST REFERENCE, always
    Log(e.what());                    // (by value would slice - Chapter 2!)
    throw;                            // rethrow: plain 'throw', like C#
}
// NO 'finally' in C++. RAII *is* the finally:
// cleanup lives in destructors, which run during stack unwinding.
```

Standard exceptions derive from `std::exception` (`std::runtime_error`, `std::logic_error`, `std::out_of_range`...). Throw by value, catch by const reference — catching by value slices derived exceptions.

### Why half the ecosystem says no

Before you decide when to throw, know that in a sizeable fraction of professional C++ you will not be allowed to. Entire serious ecosystems build with exceptions switched off at the compiler — `-fno-exceptions` on GCC and Clang, `/EHs-c-` on MSVC. Most game engines (Unreal disables them by default), most embedded and freestanding targets, LLVM's own codebase, and several of the most-copied style guides, Google's among them, all sit in that world. It is not a fringe: it is a second dialect of the language with the same syntax.

The reasons stack up: the cost model of the next section, determinism (an unwind's duration is unbounded and unpredictable, which is unacceptable inside a 16-millisecond frame or an interrupt handler), binary size on targets counting kilobytes, and history — code written before exception-safety was understood cannot have exceptions switched on retroactively, because every function that was never audited for it is a leak waiting for its first throw.

What it means in practice is the part that matters on your first day: **which world you live in is a property of the codebase, not a choice you make per function.** It is a flag on the whole build. With `-fno-exceptions`, `throw` and `catch` are compile errors, and the standard library's throwing paths — `vector::at` on a bad index, an allocation that fails — end the process instead of propagating. So finding out is a day-one task: grep the build description for the flag, and read the project's style guide. Writing three days of code in the wrong dialect is a rewrite, not a fix.

> [!NOTE]
> **Surprise for C# devs:** this split simply does not exist for you today — there is no `-fno-exceptions` for the CLR, no .NET codebase where `throw` fails to compile, and no library that assumes you cannot catch. In C++, "can I throw here?" is a real question with a per-codebase answer.

### What a throw actually costs

Modern C++ implementations use **table-based** ("zero-cost") exceptions. The compiler emits side tables mapping instruction ranges to the cleanup actions and handlers for that range, and the generated code contains no checks at all: entering a `try` block costs nothing, and neither does calling a function that might throw. Compare that with an error code, which pays a compare-and-branch at *every* call site on *every* call, whether or not anything ever fails. On the success path, exceptions are the cheaper of the two mechanisms — this is the half of the trade-off that surprises people.

The throw path is where the bill arrives. A throw allocates the exception object, then hands control to the unwinder, which on mainstream implementations walks the stack **twice**. The first pass looks up the tables for each frame and RTTI-matches the exception's type against its handlers until one accepts, running no destructors at all; only once a handler is located does the second pass unwind toward it, running the destructors each frame owns on the way. That machinery is cold — it is not in your instruction cache, and often not even paged in — and the work scales with how many frames and destructors lie between the throw and the catch. Treat the ratio as **thousands of times a plain return**: nanoseconds become microseconds, and, more importantly, a *variable* number of microseconds.

That is why "exceptions are for exceptional" is a literal engineering statement in C++ and not style advice. Two consequences worth carrying:

- **A throw in a hot loop is a design bug**, not a slow function. Per-frame, per-sample, or per-row failure signalled by throwing turns a steady workload into one with latency spikes — and it profiles as a timing outlier, never as a wrong answer, which is why it survives so long.
- **The unwind tables are in the binary whether or not you ever throw.** They cost image size, along with the RTTI the matching needs and the unwinder itself. On a microcontroller with a few tens of kilobytes of flash, that single fact ends the discussion — which is the embedded half of the previous section.

> [!WARNING]
> **Trap:** reaching for exceptions as control flow because "the happy path is free." The happy path is free; the sad path is the most expensive thing in the function, and it is the path a parser or a network reader takes constantly. Cost follows how *often* you throw, not how cheaply you don't.

### The other pole: error codes

The error-code world you'll actually live in:

```cpp
ErrCode err = Thing_GetData(index, &data);      // Chapter 17's SDK idiom
if (err != NoErr)
    return err;                  // check EVERY call. No exception will save you.

err = Thing_SumValues(&data, &sum);
if (err != NoErr)
    return err;
// The tedium is real. Mitigations: early returns (not nested ifs),
// RAII guards so early returns can't leak, small helper functions.
```

The tedium buys one real property, and it is worth naming: the failure is **on the signature**, so it is impossible for a caller to be unaware that the call can fail. An exception is invisible at the call site — nothing in C++ declares which exceptions a function throws. The feature that did — the *dynamic* exception specification, `throw(NetworkError, ParseError)` — was deprecated in C++11 and removed in C++17; `noexcept` is not its replacement, because all it can say is "none". C# at least gives you documentation conventions and an analyzer culture; C++ gives you nothing. That asymmetry, not the syntax, is what keeps error codes alive in code that must be audited.

### Between the two poles: the standard vocabulary

The two mechanisms are the poles, not the whole map. Three types in the standard library sit between them, and you should recognize all three on sight.

**`std::error_code` and `std::system_error`** — a portable carrier for a platform error number: a value plus a *category* that says which numbering scheme it belongs to, comparable against the portable `std::errc` constants. `std::filesystem` is the visible example, because every one of its functions ships in two overloads:

```cpp
namespace fs = std::filesystem;

std::error_code ec;
auto n = fs::file_size(path, ec);   // non-throwing overload: reports into ec
if (ec)                             // set == failure (contextually bool)
    Log(ec.message());              // "No such file or directory"

auto m = fs::file_size(path);       // throwing overload: fs::filesystem_error,
                                    // which derives from std::system_error
```

The standard library shipping both forms of every function is the clearest admission available that neither mechanism won.

**`std::optional<T>`** ([Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency)) — for when *absence is not an error*. A lookup that finds nothing, a parse of an optional field: ordinary outcomes with nothing to explain. The moment the caller needs to know *why*, `optional` is the wrong type — it has room for exactly one bit of bad news.

**`std::expected<T, E>`** — value or error in one return, the Result shape you may know from Rust or F#. **This is C++23**; check your standard before reaching for it.

```cpp
// C++23. Most codebases are not here yet - see below.
std::expected<Config, ParseError> LoadConfig(std::string_view text);

auto cfg = LoadConfig(text);
if (!cfg)
    return Report(cfg.error());     // the E side
Use(*cfg);                          // the T side - dereference, like optional
```

Most codebases are not on C++23, so most codebases that want this shape ship their own: Abseil's `StatusOr`, Boost.Outcome, LLVM's `Expected`, or a house `Result<T, E>`. They differ in spelling and in how loudly they complain when you get it wrong; they are all the same idea, and recognizing the shape when you meet it under a new name is the actual skill.

> [!WARNING]
> **Trap:** assuming these types force the check. `*cfg` without testing `cfg` first is undefined behavior on the error side, exactly as `optional`'s `operator*` is — the same bug class as an ignored error code, wearing a nicer type. Only an exception is impossible to ignore.

### Choosing: is the failure a bug, a value, or an event?

This is the decision C# never asks you to make, and the one that separates C++ code that reads well from C++ code that merely compiles. Ask what *kind of thing* the failure is.

**A bug** — a broken precondition, a violated contract, a state that cannot occur unless someone upstream is wrong. This gets an **`assert`**. It dies loudly in Debug, compiles to nothing under `NDEBUG`, and documents the contract in the one place that cannot go stale. Finding 8 of [Chapter 25](25-findings-from-practice.md#chapter-25--findings-from-practice-a-living-log) is the book's own example: `int& At(size_t i) { assert(i < size_); ... }`. Do not "handle" this case — a caller cannot do anything sensible with the news, because the caller is what is broken. (The same Finding names the alternative: `.at()`, which *throws* `std::out_of_range`. That is the right choice when the index arrives from outside your program — a file, a user, a socket. At that point it stopped being a bug and became the next category.)

**A value** — an expected, recoverable runtime failure. The file is missing, the device is busy, the input does not parse, the key is not in the map. This gets an **error code, `optional`, or `expected`**: failure is *data*, it appears in the signature, and the caller handles it as a normal part of using the function. Most failures in most programs are here.

**An event** — a rare, genuinely exceptional failure that must travel through frames which can do nothing about it. Allocation failure; an invariant broken six levels down; the whole operation being abandoned. This gets a **`throw`**. And so does the one structural case with no alternative: **constructors cannot return anything.** [Chapter 18](18-exercise-the-device-sdk.md#chapter-18--exercise-the-device-sdk)'s `DeviceSession::Open` static factory exists precisely because `Device_Open` can fail and a constructor has no channel to say so — in an exception-enabled codebase, the constructor throws and that workaround is unnecessary; in a `-fno-exceptions` codebase, the factory *is* the answer.

| The failure is… | C++ says | C# says |
|---|---|---|
| A caller bug — broken precondition, impossible state | `assert` — die loudly in Debug | `ArgumentNullException`, `InvalidOperationException` |
| A value — file missing, device busy, parse failed | error code / `optional<T>` / `expected<T,E>` | `FileNotFoundException` (or the `TryX` pattern) |
| An event — rare, non-local; *and every failing constructor* | `throw` | exception |

The right-hand column is the point. In C# all three rows are one mechanism: `ArgumentNullException` **is** the assert case (nobody catches it; it exists to blame the caller), and `FileNotFoundException` **is** the expected-failure case (everybody catches it; it is the return value in disguise). One keyword covers a bug, a value, and an event, and the distinction lives only in your head and the documentation. C++ splits them by mechanism, so a signature carries the author's answer — and reading it back is a design review: a parser that throws once per malformed line has filed a value under events, and it will be the slowest part of the program.

> [!TIP]
> **Key principle:** "I ask whether a failure is a bug, a value, or an event — a bug gets an assert, a value gets a return the caller must look at, and only a rare non-local failure, or a constructor with no return channel, gets a throw."

> [!TIP]
> **Key principle:** "Whether exceptions exist at all is a property of the build, not a per-function choice — I find out which dialect a codebase speaks before I write my first function in it."

### The rules that do not bend

Two rules to hold: **(1)** exceptions must never cross a DLL/plug-in boundary into a host application or a C API — catch everything at your entry points and convert to error codes; the code on the other side is not prepared for your exceptions. **(2)** destructors must never throw — one is already running during unwinding; a second exception terminates the program.

Exception-safety guarantees (worth knowing cold): **basic** — no leaks, object in some valid state; **strong** — operation succeeds or has no effect (copy-and-swap from Chapter 6 delivers this); **noexcept** — cannot throw. If a constructor throws, the object never existed — its destructor does NOT run, but already-constructed members ARE destroyed. That is why acquiring resources through RAII members is safe and raw acquisition in ctor bodies is not.

> [!TIP]
> **Key principle:** "I check every error code, use RAII so early returns can't leak, and never let an exception escape the plug-in boundary — I catch at entry points and translate to the SDK's error codes."

### In the wild: C-style SDKs

Native SDKs are the error-code pole in its purest form, and Chapter 16's Bestiary tells you what to expect before you open the header: Shape 1 returns a status from every function with results in out-parameters; Shape 4's embedded HALs do the same with an enum per subsystem (`HAL_OK`, `HAL_BUSY`, `HAL_TIMEOUT`). Do not assume the encoding: success is usually zero, but "usually" is not a contract — COM's `HRESULT` needs a `SUCCEEDED()` test rather than `== 0`, and plenty of C libraries return negative for failure and a useful count for success.

Then answer the Bestiary's failure-contract question, which is the one people skip: **on failure, is the out-parameter touched or untouched?** Chapter 17's documentation trap is built on exactly this sentence — *"on ANY failure, `data` is left untouched and nothing is allocated"* — because whether your cleanup path is safe after a failed call is a *documented promise*, not a property you can see. If the docs are silent, write a small program that finds out before your dispose path depends on the answer.

The spine of all of it is **translation at the boundary**, in both directions. Inbound: an SDK's error enum is not an error value you should keep — map it once, at the wrapper, into your own vocabulary, or the vendor's enum spreads through your codebase and swapping the SDK becomes a rewrite. Outbound: at every entry point the host calls, nothing escapes.

```cpp
// The status crosses the boundary, so its width is pinned rather than
// compiler-chosen - a plain enum's underlying type is the compiler's choice.
typedef int32_t PluginStatus;
enum : int32_t { PluginOk = 0, PluginFailed = 1, PluginOutOfMemory = 2 };

// The entry point the host calls. Nothing escapes it.
extern "C" PluginStatus Plugin_Process(Ctx* ctx) {
    try {
        DoWork(ctx);                       // my code, my rules: it may throw
        return PluginOk;
    } catch (const std::bad_alloc&) {
        return PluginOutOfMemory;          // a code the host understands
    } catch (const std::exception& e) {
        Log(e.what());                     // the last place with a message
        return PluginFailed;
    } catch (...) {                        // even the ones I don't know about
        return PluginFailed;
    }
}
```

The `catch (...)` is not paranoia. The host's frames were compiled by someone else's toolchain, quite possibly with exceptions disabled entirely — unwinding into them is undefined behavior on a good day and a silent `terminate` on a normal one. [Chapter 30](30-authoring-an-abi-boundary.md#chapter-30--authoring-an-abi-boundary) makes this one of the rules of authoring a boundary of your own.

---


<!-- nav:begin -->
[← Chapter 7 — Templates vs C# Generics](07-templates-vs-csharp-generics.md) · [Contents](README.md) · [Chapter 9 — Casts, Conversions, and Strings →](09-casts-conversions-and-strings.md)
<!-- nav:end -->
