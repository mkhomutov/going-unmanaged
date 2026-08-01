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

`const char*` = pointer to characters I won't modify (data is const); `char* const` = the pointer itself is const. Read right-to-left. Write const by reflex — const-correctness is a visible marker of current, careful C++.

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

---

---


<!-- nav:begin -->
[← Chapter 31 — Reading What the Tools Tell You](31-reading-what-the-tools-tell-you.md) · [Contents](README.md) · [Appendix B — Core Principles (Cheat Sheet) →](B-core-principles.md)
<!-- nav:end -->
