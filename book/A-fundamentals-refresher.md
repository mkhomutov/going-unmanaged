# Appendices

---

## Appendix A — Fundamentals Refresher

### A.1 Pointers and the arrow operator

A pointer is a variable that stores a **memory address**. Two operators: `&x` = "address of x"; `*p` = "dereference p" (go to the address and get the value).

```cpp
int x = 42;
int* p = &x;       // p holds the ADDRESS of x, not 42
std::cout << *p;   // 42 (followed the pointer)
*p = 100;          // write THROUGH the pointer
std::cout << x;    // 100 - we changed x itself
```

C# analogy: every C# class variable is secretly a pointer. C++ makes it explicit and gives you the choice. Null is **nullptr**; dereferencing it is not a nice NullReferenceException — it is undefined behavior, usually a crash. Always check: `if (p) { ... }`.

```cpp
Widget w;   Widget* p = &w;
w.size;      // dot: I have the object itself
(*p).size;   // dereference, then dot - clunky
p->size;     // arrow: same thing, nicer.  -> is shorthand for (*).
p->Draw();
```

Rule: **object → dot, pointer → arrow.** Smart pointers overload `->` and `*`, which is why unique_ptr feels like a raw pointer. C-style APIs ask for addresses to fill in: `Thing_GetData(index, &data)` (Chapter 17) means "here is where my struct lives, write into it".

### A.2 References (the & in a type)

Same symbol, two meanings: `&x` in an *expression* = address-of; `T&` in a *type* = **reference** — an alias, another name for an existing object.

| | Pointer `int*` | Reference `int&` |
|---|---|---|
| Can be null | yes | no — must be bound at creation |
| Can be reseated | yes | no — bound forever |
| Access syntax | `p->size`, `*p` | plain `r.size`, `r` |

```cpp
void Rename(Widget w)  { w.name = "new"; }  // edits a COPY - useless
void Rename(Widget& w) { w.name = "new"; }  // caller's actual object (C# ref)
```

(For Java readers: the `T&` line behaves like every object parameter you have ever passed — the callee sees the caller's object. The genuinely new thing is the first line, C++'s default: the entire object, copied.)

Parameter decision guide: small type (int, double, GUID) — by value; big object, read only — **`const T&`** (the workhorse of C++); need to modify caller's object — `T&`; "no object" must be valid — pointer `T*` (can be null). C has no references, which is why C-style APIs use pointers.

### A.3 explicit

A single-argument constructor doubles as an *implicit conversion* in C++. **explicit** forbids the silent conversion — like C#'s explicit vs implicit conversion operators.

```cpp
class Buffer {
public:
    explicit Buffer(int size);
};
void Send(Buffer b);
Send(5);           // ERROR: no implicit conversion (good!)
Send(Buffer(5));   // OK: you clearly meant it
```

> [!TIP]
> **Key principle:** Mark every single-argument constructor explicit unless you deliberately want implicit conversion. It costs nothing and prevents a whole category of silent bugs.

### A.4 = delete (and = default)

Not the delete operator (which frees memory). After a function declaration, **= delete** means "this function is forbidden; calling it is a compile error." Needed because the compiler **auto-generates** copy constructor, copy assignment, destructor, and move operations — and for a class owning a resource, the auto-generated shallow copy causes a double-free.

```cpp
FileHandle a("data.txt");   // a.f points to the open file
FileHandle b = a;           // auto-generated copy: b.f = a.f - same FILE*!
// both destructors run: fclose called TWICE. Crash/corruption.

class FileHandle {
public:
    FileHandle(const FileHandle&) = delete;             // no copy
    FileHandle& operator=(const FileHandle&) = delete;
    FileHandle() = default;   // = default: "generate the standard version"
};
```

std::unique_ptr's copy operations are deleted — that IS the mechanism enforcing "unique". In C#, `b = a` on a class copies a reference, so this problem cannot exist.

### A.5 const in parameters and methods

```cpp
void Save(Widget w);          // safe but copies - wasteful for big objects
void Save(Widget& w);         // fast, but may modify caller's object
void Save(const Widget& w);   // fast AND harmless - the C++ default idiom

void Greet(std::string& s);        // Greet("hello") - ERROR
void Greet(const std::string& s);  // Greet("hello") - OK (temporaries bind)

class Widget {
public:
    int GetSize() const;   // "this method doesn't modify the object"
};                         // only const methods callable via const Widget&
```

Those three are the common cases, not the whole menu: sinks, views, and optional parameters have their own shapes, and [Appendix H](H-choosing.md#appendix-h--choosing-signatures-containers-and-storage) is the procedure that picks between all of them. `const char*` = pointer to characters I won't modify (data is const); `char* const` = the pointer itself is const. Read right-to-left. Write const by reflex — const-correctness is a visible marker of current, careful C++.

That is the syntax. [Appendix I](I-const.md#appendix-i--const-correctness) is the model underneath it — why the same object can be writable through one reference and not another, what `mutable` is narrowly for, and the procedure for adding const to a class that has gone without it for three years.

### A.6 What is a .lib file?

A **static library** is just an archive of .obj files with a symbol index (Linux: .a). At link time the library's code is **copied into your binary** — after linking you don't need the .lib anymore.

| | .lib (static) | .dll (dynamic) |
|---|---|---|
| When resolved | link time | runtime |
| Code ends up | inside your binary | stays in the DLL |
| Ship with product? | no — baked in | yes |
| Update without rebuild? | no | yes |
| C# analogy | (none, really) | closest to a referenced assembly |

The confusing part: on Windows, DLLs ship with a companion .lib — an **import library** of stubs ("function X lives in Foo.dll"). Same extension, two different animals. Consuming a C++ library needs the trio: **.h** (compiler knows signatures), **.lib** (linker resolves calls), **.dll** if dynamic (present at runtime). Miss the header = compile error; miss the .lib = LNK2019; miss the DLL = "DLL not found" at startup.

### A.7 signed, unsigned, and size_t

`size_t` is the standard library's **unsigned** integer type for sizes and indices — 64-bit on a desktop build, and 32-bit on a 32-bit target such as the peripheral firmware behind Chapter 18's device (where every wrapped value below is 4294967295 rather than 18446744073709551615; the arithmetic is identical, the number is not). Every container's `.size()` returns it, `sizeof` yields it, and every index-taking member function takes it.

In C#, `Count` and `Length` are `int` — signed, always. `uint` and `ulong` exist, and the framework guidelines steer you away from them in public APIs, so in practice nobody uses them. The consequence is worth naming plainly: a C# developer arrives with **no instinct for unsigned arithmetic at all**, and meets it on day one, in the first loop they write.

**Collision 1: the mixed-sign comparison.** The loop everyone writes from muscle memory:

```cpp
for (int i = 0; i < v.size(); ++i)      // int vs size_t
    std::printf("%d\n", v[i]);
```

```
warning: comparison of integers of different signs: 'int' and 'size_type'
      (aka 'unsigned long') [-Wsign-compare]
```

Two fixes, and the second is the better habit. Match the type — `for (size_t i = 0; ...)` — or stop indexing at all and use a range-for, which is the same `const auto&` reflex Chapter 2 asks for everywhere else:

```cpp
for (size_t i = 0; i < v.size(); ++i)   // matched types, warning gone
    std::printf("%d\n", v[i]);

for (const auto& x : v)                 // no index, no signedness, no bounds
    std::printf("%d\n", x);
```

**Collision 2: `size() - 1` on an empty container.** This one is not a warning; it is a wrong answer.

```cpp
bool InRange(const std::vector<int>& v, size_t i) {
    return i <= v.size() - 1;   // empty vector: 0 - 1 does NOT give -1
}                               // it gives 18446744073709551615, so this is
                                // always true, for every i
```

`v.size()` is unsigned, so `v.size() - 1` on an empty vector wraps to the largest `size_t` there is. The guard that was supposed to reject every index accepts every index. The fix is to stop subtracting:

```cpp
bool InRange(const std::vector<int>& v, size_t i) {
    return i < v.size();        // no subtraction, nothing to wrap
}
```

**The asymmetry worth stating plainly.** Unsigned overflow **wraps**, and that is *defined* behavior — the standard says so. Signed overflow is **undefined** (Chapter 3's greatest-hits list). The counter-intuitive part is that being legal is exactly what makes the unsigned case dangerous: UBSan reports a signed overflow the moment it happens, and it stays silent on the wrap above, because nothing went wrong as far as the language is concerned. The broken `InRange` compiles clean under `-Wall -Wextra` and runs clean under `-fsanitize=address,undefined`. Clang does ship a check for it — `-fsanitize=unsigned-integer-overflow`, also in the `-fsanitize=integer` group — but it is deliberately left out of `-fsanitize=undefined`, because legal wrapping is ordinary in real code and the noise would bury the genuine findings. Off by default is the part that bites: under the flags you actually build with, the only symptom is the answer.

Know-they-exist, for when you meet them: C++20 adds `std::ssize(c)` — the same count, as a signed type — and the `std::cmp_less` family, which compare across signedness and give the mathematically true answer (`std::cmp_less(-1, v.size())` is `true`, where `-1 < v.size()` is `false`). Both exist for exactly this friction.

> [!TIP]
> **Key principle:** "`size()` is unsigned, so `size() - 1` on an empty container is a huge number, not -1 — I compare with `<` instead of subtracting, because the wrap is legal and my `-fsanitize=address,undefined` build stays silent about it."

### A.8 Naming: there is no house style, so learn to read three

C# has one naming convention and a whole ecosystem obeys it: `PascalCase` for everything public, `_camelCase` fields, `I` on every interface. C++ has none. The standard library, the big style guides and the big frameworks each chose differently, and a codebase inherits whichever its founders read first — so the reflex to bring is not a convention but the habit of reading one off the page. Three dialects cover nearly everything you will open:

| Dialect | Types | Functions and members | Where you meet it |
|---|---|---|---|
| standard-library | `snake_case` — `string_view`, `size_t` | `snake_case` functions and members — `push_back` | the STL, Boost, most header-only libraries, this book's cookbook |
| Google (Chromium, Abseil) | `PascalCase` | `PascalCase` functions, `name_` members, `kConstant` constants | most SDK samples, this book's chapters |
| LLVM | `PascalCase` | `camelCase` functions, `PascalCase` members, no `k` prefix | LLVM, Clang, and code written by people who came from them |
| frameworks | `PascalCase`, often with a prefix letter | `camelCase` functions and `m_name` members (Qt); `PascalCase` functions and prefixed bare members like `bEnabled` (Unreal) | Qt, Unreal, JUCE, most C++-native SDKs |

Members are the column worth a second look, because every spelling is legal and one is a trap: `name_` (this book), `m_name` (the frameworks), and `_name` — safe as a member, and one capital letter away from the form the language reserves.

> [!WARNING]
> **Trap:** an identifier that starts with an underscore and a capital letter, or contains a double underscore, is reserved to the implementation *anywhere* — `_Foo`, `__count`, and the include guard `_WIDGET_H`, the commonest violation in the wild — and using one is undefined behavior that this book's flags never mention: clang's `-Wreserved-identifier` is off even under `-Wall -Wextra`, and clang-tidy's `bugprone-reserved-identifier` is the check a team turns on.

The narrower half of the rule: a leading underscore followed by a *lowercase* letter is reserved only in the global namespace, which is why `_name` members survive and a global `_helper()` does not. Two more spellings carry meaning of their own — macros are `SCREAMING_CASE` and nothing else is, so a macro can never pass for a function, and the `I` on an interface is COM's habit rather than C++'s (Chapter 30's `IScorer` wears it deliberately, for the shape it imitates). Reading across dialects on one page is the skill: the chapters here spell like an SDK sample, the cookbook like the standard library, and a wrapper that imitates a standard type — Chapter 35's `ThingHandle`, with its `get` and `swap` — spells like the thing it imitates.

Layout has a tool and names mostly do not. `.clang-format` at the repository root is the `.editorconfig` you know, and `clang-format -i` rewrites whitespace and line breaks, never names; clang-tidy's `readability-identifier-naming` checks names where a team bothers. Neither is universal, so the working rule is older than both.

> [!TIP]
> **Key principle:** "I read fifty lines of a codebase before I write one, and match what is there — never spelling an identifier with a leading underscore and a capital."

---


<!-- nav:begin -->
[← Chapter 39 — The Round Trip Home](39-the-round-trip-home.md) · [Contents](README.md) · [Appendix B — Core Principles (Cheat Sheet) →](B-core-principles.md)
<!-- nav:end -->
