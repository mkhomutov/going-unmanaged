# Part III — The Standard Library

---

## Chapter 10 — Modern C++ Fluency

C++ has had a major update every 3 years since 2011 (C++11/14/17/20/23/26). These features, used casually, are the difference between current C++ and 2008-era C++.

### auto — type inference (C#'s var)

```cpp
auto count = 42;                 // int
auto it = widgets.begin();       // saves the long iterator type
auto& w = widgets[0];            // auto alone COPIES - add & to alias
const auto& name = GetName();    // the read-only idiom
```

> [!WARNING]
> **Trap:** auto strips references: `auto w = widgets[0]` is a copy. Muscle memory: `const auto&` for reading, `auto&` for modifying, plain `auto` only when you want a copy.

`auto` has a sibling you will read long before you write it: **`decltype(expr)`** is "the type this expression has", answered by the compiler and never at runtime — C# has no counterpart, since `typeof` hands you a runtime object and `var` is only the `auto` half. Two spellings cover almost every sighting. `using SetCallback = decltype(&Device_SetCallback);` names a vendor function-pointer type without retyping its signature, so it cannot drift from the header; and `decltype(auto)` in library forwarding code means "whatever this returns, reference and all", which you may skip over. Chapter 38's queue uses the library form, `std::invoke_result_t<F>`, which is `decltype` of a call with the plumbing hidden.

### Lambdas — capture is explicit (no GC to keep captures alive)

```cpp
int threshold = 10;
auto f1 = [threshold](const Widget& w) { return w.size > threshold; };  // COPY
auto f2 = [&threshold](const Widget& w){ return w.size > threshold; };  // REF
auto f3 = [=](...) { ... };    // everything used, by copy
auto f4 = [&](...) { ... };    // everything used, by reference
auto f5 = [this](...) { ... }; // capture enclosing object's this

auto MakeGetter() {
    int local = 5;
    return [&local] { return local; };  // BUG: dangling reference!
    // fix: [local] - copy it
}
```

> [!TIP]
> **Key principle:** "Capture by reference only when the lambda won't outlive the scope; by copy (or move) when it escapes — stored, returned, or run async."

### Algorithms + lambdas (C++'s LINQ, roughly)

```cpp
auto it = std::find_if(v.begin(), v.end(),
                       [](const Widget& w) { return w.selected; });

std::sort(v.begin(), v.end(),
          [](const Widget& a, const Widget& b) { return a.size < b.size; });

// C++20 ranges - even closer to LINQ (Where + Select, lazy):
auto big = v | std::views::filter([](auto& w){ return w.size > 10; })
             | std::views::transform([](auto& w){ return w.name; });
```

### std::optional\<T\> — "maybe a value" (C#'s T?)

```cpp
std::optional<Widget> FindByName(const std::string& name);

if (auto w = FindByName("wall"); w.has_value()) {
    Use(*w);                                   // or w->name
}
auto w2 = FindByName("x").value_or(Widget{});  // ?? equivalent
```

> [!TIP]
> **Key principle:** "A function that can fail to produce a value returns optional\<T\>, not a null pointer or a magic value like -1."

What `optional` is *not*, because each row is a place the `T?` reflex misfires:

| In C# | In C++ | What is missing |
|---|---|---|
| `Widget? w` — a nullable *reference* | `const Widget*`, or `std::reference_wrapper` | `std::optional<T&>` does not exist; the compiler refuses it outright |
| `w?.Name` | `if (w) Use(w->Name)` | no null-propagating call in C++17 — and `w->Name` on an empty optional is not a null-reference exception |
| `w = null` | `w = std::nullopt`, or `w.reset()` | — |
| `bool?` | a small `enum class` | `optional<bool>` compiles and reads as three-state nothing |

The second row is the one to carry. `*` and `->` on an empty `optional` are Chapter 3's quiet undefined behaviour: the value reads as garbage, the program carries on, and this book's sanitizer flags say nothing about it (a hardened standard library, Chapter 13, is what traps it). Check first, or call `value()`, which throws `bad_optional_access` and is the one spelling that fails loudly. C++23 adds `and_then` and `transform`, the `?.` chain; until your toolchain has them, Recipe 19 in [Appendix F](F-rosetta-cookbook.md#appendix-f--the-rosetta-cookbook) is the by-hand form.

### std::variant — the tagged union with the compiler on your side

C# has no closed sum type. When a value is "one of these three things" you write a small class hierarchy and pattern-match on it — `switch (e) { case Fault f: ... }` — and the runtime carries the real type for you. C++ has that too (Chapter 5), and it also has the older spelling you will read in every vendor event struct: a `kind` field next to a `union`, with nothing checking that the tag and the payload agree. `std::variant` is that union with the tag enforced.

```cpp
struct Temperature { int centi; };
struct Fault       { int code; };
struct Heartbeat   {};
using Event = std::variant<Temperature, Fault, Heartbeat>;   // exactly one of these

Event e = Fault{7};
if (auto* f = std::get_if<Fault>(&e)) Alarm(f->code);       // the 'as' test: nullptr if not
std::holds_alternative<Fault>(e);                            // the 'is' test
std::get<Temperature>(e);                                    // throws bad_variant_access - never garbage

std::visit(overloaded{                                       // the switch: one lambda per alternative
    [](const Temperature& t) { Plot(t.centi); },
    [](const Fault& f)       { Alarm(f.code); },
    [](Heartbeat)            { Tick(); },
}, e);
```

`overloaded` is a two-line idiom that turns a handful of lambdas into one callable; C++17 does not ship it, every codebase has one, and Recipe 20 in [Appendix F](F-rosetta-cookbook.md#appendix-f--the-rosetta-cookbook) spells it out. `std::monostate` is the "not yet set" alternative for a variant that must be default-constructible. And the property the C `union` never had: leave one alternative out of the `visit` and the program does not compile — libc++ says so in as many words, *`std::visit` requires the visitor to be exhaustive*. A `switch` on a `kind` field with a missing `case` compiles and falls through.

When to reach for which. A **closed** set of unrelated types that you own — the events a device sends, the result of a parse, the states of a small machine — is a variant: stored by value, no base class, no heap, nothing to slice ([Appendix H](H-choosing.md#appendix-h--choosing-signatures-containers-and-storage)'s procedure 4 has this branch). An **open** set that someone else extends — plug-ins, a Shape hierarchy a customer subclasses — is a virtual base behind `unique_ptr` (Chapter 5), because a variant's list of alternatives is fixed at the point it is spelled. `std::any` exists too, as the `object` box; it is almost never what you want, and reaching for it usually means the set was closed and nobody wrote it down.

> [!TIP]
> **Key principle:** "A closed set of unrelated alternatives is a std::variant — stored by value, visited exhaustively, and the compiler refuses a visitor that forgets one; an open set someone else extends is a virtual base behind unique_ptr."

### std::string_view — non-owning view of a string

```cpp
void Print(std::string_view sv);  // accepts std::string, literals,
                                  // substrings - NO copy
```

A pointer + length, like C#'s `ReadOnlySpan<char>`. Replaces `const std::string&` for read-only string parameters. Danger: non-owning means it can dangle — never store a string_view to a temporary.

**Try it (30 seconds).** Return a `string_view` of a local `std::string` from a function and read it at the call site under ASan. clang already objects at compile time (`-Wreturn-stack-address`), and the run is a textbook heap-use-after-free — Chapter 31 teaches you to read that report; here it is enough to watch the trap fire.

And string_view is the string-shaped case of an idea your C# already names in general: `Span<T>`/`ReadOnlySpan<T>` over *any* contiguous buffer is **`std::span<T>`** — C++20, so this book's C++17 exercises spell the same thing as the pointer-plus-length pair you will meet in every C API of Chapter 16. When your codebase has span, use it; until then you are writing span by hand and should feel no shame. *When* a view is the right parameter shape at all — against `const&`, a sink, or a plain pointer — is [Appendix H](H-choosing.md#appendix-h--choosing-signatures-containers-and-storage)'s parameter procedure, which routes here for the mechanism and adds only the choice.

### Structured bindings (C# 7 deconstruction)

```cpp
auto [it, inserted] = myMap.insert({key, value});

for (const auto& [name, widget] : widgetMap) {  // KeyValuePair unpacked
    std::cout << name;
}
```

### constexpr — computation at compile time

```cpp
constexpr int Square(int x) { return x * x; }
constexpr int area = Square(12);    // computed by the COMPILER
std::array<int, Square(4)> buffer;  // usable where constants are required
```

### Small but telling details

```cpp
nullptr                          // never NULL or 0
enum class Color { Red, Blue };  // scoped enum, like C# enums
using WidgetList = std::vector<Widget>;  // modern typedef
uint32_t, int64_t                // from <cstdint>: 'int' size isn't
                                 // guaranteed! (C# int is always 32-bit)
```

### In the wild: C-style SDKs

Most actively maintained SDKs now require C++17, so nearly all of this is usable in your plug-in or driver code — the exception being the ranges above, which are C++20; every maintained toolchain has had them for years, so whether you may write them is a property of the codebase's `-std=` setting and policy, not of your compiler's age — Chapter 8's dialect lesson again. The professional style: modern C++ in *your* logic — optional, lambdas, RAII wrappers — with a thin, disciplined layer where you touch the raw C API. The older the SDK's surface, the more valuable the modern layer you build on top of it.

The baseline question deserves a straight answer while we are here, because a current reader is entitled to ask why this book pins `-std=c++17` when C++20 is complete in all three compilers and GCC now defaults to it. Because vendor-SDK work inherits its host's toolset: the pinned compiler of Chapter 13's checklist, the plug-in ABI, the embedded toolchain two versions behind — your floor is set by the oldest thing you must link against, and C++17 is what that world lets you *rely* on. Everything taught here is valid C++20 and C++23. A newer-standard codebase changes spellings — `jthread` for thread-plus-join, `erase_if` for erase-remove, `format` for the stream dance, `span` for pointer-plus-length — and this book flags each of those where it teaches the C++17 form. The lessons don't move; only the spellings do.

---


<!-- nav:begin -->
[← Chapter 9 — Casts, Conversions, and Strings](09-casts-conversions-and-strings.md) · [Contents](README.md) · [Chapter 11 — STL Containers, Algorithms, and Iterator Invalidation →](11-stl-containers-and-algorithms.md)
<!-- nav:end -->
