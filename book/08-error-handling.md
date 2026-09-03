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

Standard exceptions derive from `std::exception` (`std::runtime_error`, `std::logic_error`, `std::out_of_range`...). Throw by value, catch by const reference — catching by value slices derived exceptions. Your own exception types derive from `std::runtime_error` (or `std::logic_error`, for a caller's bug), build their message in the constructor, and carry what `what()` cannot — a line number, a code — as a member; Recipe 21 in [Appendix F](F-rosetta-cookbook.md#appendix-f--the-rosetta-cookbook) is the shape. And catch clauses are tried in order, so a derived type goes before its base or its handler is dead code — both compilers warn about it by default.

### Why half the ecosystem says no

Before you decide when to throw, know that in a sizeable fraction of professional C++ you will not be allowed to. Entire serious ecosystems build with exceptions switched off at the compiler — `-fno-exceptions` on GCC and Clang, `/EHs-c-` on MSVC. Most game engines (Unreal disables them by default), most embedded and freestanding targets, LLVM's own codebase, and several of the most-copied style guides, Google's among them, all sit in that world. It is not a fringe: it is a second dialect of the language with the same syntax.

The reasons stack up: the cost model of the next section, determinism (an unwind's duration is unbounded and unpredictable, which is unacceptable inside a 16-millisecond frame or an interrupt handler), binary size on targets counting kilobytes, and history — code written before exception-safety was understood cannot have exceptions switched on retroactively, because every function that was never audited for it is a leak waiting for its first throw.

What it means in practice is the part that matters on your first day: **which world you live in is a property of the codebase, not a choice you make per function.** It is a flag on the whole build. With `-fno-exceptions`, `throw` and `catch` are compile errors, and the standard library's throwing paths — `vector::at` on a bad index, an allocation that fails — end the process instead of propagating. So finding out is a day-one task: grep the build description for the flag, and read the project's style guide. Writing three days of code in the wrong dialect is a rewrite, not a fix.

> [!NOTE]
> **Surprise for C# devs:** this split simply does not exist for you today — there is no `-fno-exceptions` for the CLR, no .NET codebase where `throw` fails to compile, and no library that assumes you cannot catch. In C++, "can I throw here?" is a real question with a per-codebase answer.

**Try it (30 seconds).** Compile `int main() { throw 1; }` with `-fno-exceptions` and read the refusal — one line, and you have seen the other dialect with your own eyes.

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

Most codebases are not on C++23, so most codebases that want this shape ship their own: Abseil's `StatusOr`, Boost.Outcome, LLVM's `Expected`, or a house `Result<T, E>` — a screenful over `std::variant`, and the translation-layer section below writes one. They differ in spelling and in how loudly they complain when you get it wrong; they are all the same idea, and recognizing the shape when you meet it under a new name is the actual skill.

> [!WARNING]
> **Trap:** assuming these types force the check. `*cfg` without testing `cfg` first is undefined behavior on the error side, exactly as `optional`'s `operator*` is — the same bug class as an ignored error code, wearing a nicer type. Only an exception is impossible to ignore.

### Choosing: is the failure a bug, a value, or an event?

This is the decision C# never asks you to make, and the one that separates C++ code that reads well from C++ code that merely compiles. Ask what *kind of thing* the failure is.

**For Java readers:** you *were* asked — checked versus unchecked is this exact split, with unchecked as the bug row and checked as the value row. An error code is a checked exception without the compiler, and C++'s removed `throw(A, B)` specification was your `throws` clause; the instinct transfers, only the enforcement is gone.

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

// The entry point the host calls, exported under its plain C name -
// extern "C" switches off name mangling (Chapter 12). Nothing escapes it.
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

### Living in both dialects: the translation layer

The section above called translation at the boundary the spine of SDK work — a vendor's codes mapped into your vocabulary on the way in, and nothing escaping on the way out. The same discipline runs along a second axis, between the two dialects themselves, and most real codebases speak both: exceptions inside a module, where every frame between a throw and its handler is yours and RAII makes the flight leak-free; values at the module's edge, where the caller is a different team, a different build setting, or a different language.

**At a module's edge, a throw becomes a value.** The parser throws — it is the deepest frame, and from where it stands a malformed file is the event pole — and the function that owns the edge catches once and returns:

```cpp
Result<Config, ConfigError> load_config(std::string_view text) {
    try {
        return Result<Config, ConfigError>::ok(Config{parse_channel_count(text, 1)});
    } catch (const ParseError& e) {          // the throw stops here: failure becomes a value
        return Result<Config, ConfigError>::fail(ConfigError{e.line(), e.what()});
    }
}
```

Nothing above that function ever sees a `ParseError`. The caller sees a result it must look at, on the signature — the property the error-code pole was always about. `Plugin_Process` above did the same thing into an `int32_t`; this is that entry point with a richer value on the way out.

**At the top of the program, a value becomes a throw again.** `value()` on the error side throws — `bad_variant_access` for the type below, `bad_expected_access<E>` in C++23, `bad_optional_access` for `optional` — and that is the one place it is allowed to: the request loop, the menu handler, the `main` of a tool, where "this operation failed, and here is why" is actionable and there is nobody left to hand a value to. Lower than that, it is the hot-loop trap of the cost section again. (The reverse trip has a carrier of its own: `std::exception_ptr` holds a thrown exception as a value for a later rethrow, which is how `std::packaged_task` carries a throw from [Chapter 38](38-the-bridge-out.md#chapter-38--the-bridge-out)'s main thread into the client's `future`, and how `std::async` delivers Recipe 13's rethrow at `.get()`.)

**The type in the middle.** `std::expected` is C++23 and this book pins C++17, so the type most readers write is a `Result<T, E>` of their own — a screenful over [Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency)'s `std::variant`:

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
```

Two named doors, and the value behind an accessor rather than in a field, so no caller reads it without a line that says which door they expect. Recipe 22 in [Appendix F](F-rosetta-cookbook.md#appendix-f--the-rosetta-cookbook) sets this beside `optional`, and `exercises/cookbook/errors.cpp` asserts all of it — including that `value()` on a failure throws, and `error()` on a success.

**Chaining.** Five steps that may each fail read, in C++17, as five early returns — `if (!r) return Result<U, E>::fail(r.error());` — the error-code pole's own shape, and nothing to apologize for. C++23's `expected` adds `and_then` for a step that may itself fail and `transform` for one that cannot, and the five lines become one expression:

```cpp
std::expected<int, ConfigError> channels_doubled(std::string_view text) {
    return load_config(text)
        .and_then([](Config c) -> std::expected<int, ConfigError> {
            if (c.channels > 64) return std::unexpected(ConfigError{1, "too many channels"});
            return c.channels;
        })
        .transform([](int n) { return n * 2; });
}
```

That is the cookbook's one C++23 listing, `exercises/cookbook/expected.cpp`, which `build_all.sh` builds as its own probe because the book's pin is C++17. Abseil's `StatusOr` and Boost.Outcome spell the same two verbs their own way; recognize the shape when a house type offers it, and do not build it before you need it.

> [!TIP]
> **Key principle:** "A value becomes a throw again only at the top of the program — the one frame with nobody left to hand it to — and never in between."

### The drill: ten failures, three verdicts

The bug/value/event decision is the chapter's whole point, and reading about a decision trains nothing — so make it ten times, now, on paper. For each scenario write one of **assert**, **value** (error code / `optional` / `expected`), or **throw**, plus one sentence of why. Some are deliberately not what they first smell like; the reasoning is worth more than the verdict. Answers are in the fold below — no peeking until all ten are written.

1. `Buffer::At(i)` receives `i >= size_`. The index came from your own loop, `for (size_t i = 0; i < buf.Size(); ++i)` — three lines above, in code you wrote.
2. The same accessor, same class — but this time the index is a row number the operator typed into a "go to row" box on the dashboard.
3. `Device_Read` returns `DeviceBusy`. The scanner is shared with the vendor's own control panel, and the manual says concurrent access is expected.
4. Row 412,809 of a million-row import fails to parse. The spec says: skip malformed rows, finish the import, report how many were skipped.
5. The same importer, first thing it does: the file's magic number is wrong. This is not our format at all — no row of it will mean anything.
6. `SessionLog`'s constructor cannot open its log file.
7. Deep inside a document operation, a `push_back` throws `std::bad_alloc`.
8. Your plug-in exports `Plugin_GetData(Ctx*, Data*)`, and the host calls it with a null `Data*`. The SDK documentation says the host never does that.
9. On first launch there is no preferences file yet. Defaults exist for everything.
10. After a refactor added a fourth value to your own `enum class Mode`, a `switch` in your own dispatch function receives a `Mode` that matches no case.

<details>
<summary>Show the verdicts — write your ten down first</summary>

1. **Assert.** A broken precondition in code you control on both sides: if `i` escapes your own loop bound, the loop is wrong, and no caller can "handle" its own arithmetic being broken. This is Finding 8's line exactly: `assert(i < size_)` — loud in Debug, free in Release, and the contract documented where it cannot go stale.
2. **Value.** Same line of code, opposite verdict — because the classification follows the *source of the data*, not the function it lands in. An operator's typo is an expected runtime input, not a broken program: validate it and return something the dashboard can turn into a red border. (`.at()`, which throws `std::out_of_range`, is the standard library's shortcut here — defensible if the catch sits right at the input boundary and translates, but a validate-and-return reads better than a throw used as a range check.)
3. **Value.** The manual told you this happens in normal operation, which is the definition of expected and recoverable: `DeviceBusy` is data, the caller's retry-or-report loop is ordinary control flow. Throwing here would file a documented steady-state condition under "exceptional" — and it will fire constantly.
4. **Value.** Per-row failure at row 412,809 of a million is the hot-loop case from the cost section: signalled by throwing, a steady workload becomes one with latency spikes, and the spec even told you failure is part of the normal result (a skip count). The malformed row is a *datum* the importer accumulates, not an event that abandons it.
5. **Throw.** One failure, at the start, that invalidates the entire operation — nothing to accumulate, nothing to resume, every frame between the magic check and the "Import…" menu handler can do nothing about it. This is the event pole: rare, non-local, abandon-the-operation. (Once per operation is cheap; note the contrast with 4, which is the same importer at a different frequency.)
6. **Throw.** Constructors have no return channel — the one structural case with no alternative. If the codebase builds `-fno-exceptions`, the answer becomes [Chapter 18](18-exercise-the-device-sdk.md#chapter-18--exercise-the-device-sdk)'s static factory returning `optional<SessionLog>` — which is the same verdict routed around a missing mechanism, not a different verdict.
7. **Throw — by doing nothing.** `push_back` already throws; your job is to *not* catch it six frames down where no sensible recovery exists. Let it unwind to the operation boundary (the menu handler, the request loop) where "the operation failed, memory is short" is actionable; RAII (Chapter 1) makes the flight through your frames leak-free. A local `try/catch` that logs and continues manufactures a half-completed operation.
8. **Value — an error code — even though it is a bug.** The trap in the list. Between your own functions this is scenario 1 and gets an assert; at an ABI boundary you do not crash the host's process in Release to punish the host's mistake, and you certainly do not throw at it (the rules that do not bend, plus [Chapter 30](30-authoring-an-abi-boundary.md#chapter-30--authoring-an-abi-boundary)). Check, return `PluginBadArgument`, optionally assert as well so *your* Debug builds still catch it loudly. The boundary changes the verdict.
9. **Neither — absence is not a failure.** A missing preferences file on first launch is a normal state with a defined meaning, so the lookup's type is `optional` and the "handling" is applying defaults. Filing this under errors at all is the mistake; there is nothing to report and nobody to warn.
10. **Assert.** Your enum, your switch, your refactor: a `Mode` matching no case means the program contradicts itself, and the missing case must die loudly in Debug at the switch, not limp onward. Belt-and-braces in Release: a `default:` that logs and returns a failure keeps a shipped binary from walking off the map — but the assert is the classification, the `default` is damage control.

Score yourself the way a review would: 8 and 2 are the ones worth re-reading the chapter over, because both flip on context — *who produced the data* and *where the call crosses a boundary* — rather than on the line of code itself.

</details>

---


<!-- nav:begin -->
[← Chapter 7 — Templates vs C# Generics](07-templates-vs-csharp-generics.md) · [Contents](README.md) · [Chapter 9 — Casts, Conversions, and Strings →](09-casts-conversions-and-strings.md)
<!-- nav:end -->
