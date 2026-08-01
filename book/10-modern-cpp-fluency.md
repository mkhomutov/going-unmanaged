# Part III — The Standard Library

---

## Chapter 10 — Modern C++ Fluency

C++ has had a major update every 3 years since 2011 (C++11/14/17/20/23). These features, used casually, are the difference between current C++ and 2008-era C++.

### auto — type inference (C#'s var)

```cpp
auto count = 42;                 // int
auto it = widgets.begin();       // saves the long iterator type
auto& w = widgets[0];            // auto alone COPIES - add & to alias
const auto& name = GetName();    // the read-only idiom
```

> **Trap:** auto strips references: `auto w = widgets[0]` is a copy. Muscle memory: `const auto&` for reading, `auto&` for modifying, plain `auto` only when you want a copy.

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

> **Key principle:** "A function that can fail to produce a value returns optional\<T\>, not a null pointer or a magic value like -1."

### std::string_view — non-owning view of a string

```cpp
void Print(std::string_view sv);  // accepts std::string, literals,
                                  // substrings - NO copy
```

A pointer + length, like C#'s `ReadOnlySpan<char>`. Replaces `const std::string&` for read-only string parameters. Danger: non-owning means it can dangle — never store a string_view to a temporary.

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

Most actively maintained SDKs now require C++17, so all of this is usable in your plug-in or driver code. The professional style: modern C++ in *your* logic — optional, lambdas, RAII wrappers — with a thin, disciplined layer where you touch the raw C API. The older the SDK's surface, the more valuable the modern layer you build on top of it.

---

---

