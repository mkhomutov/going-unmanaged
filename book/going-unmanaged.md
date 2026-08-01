# Going Unmanaged

**A Hands-On C++ Handbook for C# Developers**

You have spent years in managed code — the runtime tracked your objects, the GC cleaned up after you, and "unmanaged" was the scary word in the P/Invoke docs. This book is the journey to the other side: refresh, learn, practice.

*Who this is for:* developers with solid C# (or Java) experience who once knew C++ or are learning it now, and need to become productive in a real C++ codebase — typically one built around a vendor SDK: a plug-in API for a desktop application, a peripheral-device SDK, a game or media engine, an embedded HAL. Each chapter therefore ends with an "In the wild" section connecting the concept to the C-flavored APIs you will actually meet, and Part V trains on two miniature SDKs written in those idioms.

*How to use it:* Parts I–IV are the syllabus — read once, then return by chapter when a topic resurfaces at work. Part V is where knowledge becomes skill: a worked example, a practice plan, and a growing log of real findings from real exercises. The appendices are the survival kit: the fundamentals refresher, the one-page cheat sheet for any morning, and the offline-work playbook.

## Contents

**[Part I — The Mental Shift](#part-i--the-mental-shift)**

1. [Ownership and RAII](#chapter-1--ownership-and-raii)
2. [Value Semantics](#chapter-2--value-semantics)
3. [Stack, Heap, and Undefined Behavior](#chapter-3--stack-heap-and-undefined-behavior)

**[Part II — The Language, Side by Side](#part-ii--the-language-side-by-side)**

4. [Classes, Inheritance, Interfaces](#chapter-4--classes-inheritance-interfaces)
5. [Virtual Dispatch and the Virtual Destructor](#chapter-5--virtual-dispatch-and-the-virtual-destructor)
6. [The Rule of Five and Move Semantics](#chapter-6--the-rule-of-five-and-move-semantics)
7. [Templates vs C# Generics](#chapter-7--templates-vs-c-generics)
8. [Error Handling: Exceptions and Error Codes](#chapter-8--error-handling-exceptions-and-error-codes)
9. [Casts, Conversions, and Strings](#chapter-9--casts-conversions-and-strings)

**[Part III — The Standard Library](#part-iii--the-standard-library)**

10. [Modern C++ Fluency](#chapter-10--modern-c-fluency)
11. [STL Containers, Algorithms, and Iterator Invalidation](#chapter-11--stl-containers-algorithms-and-iterator-invalidation)

**[Part IV — The Build and the Toolchain](#part-iv--the-build-and-the-toolchain)**

12. [The Compilation Model](#chapter-12--the-compilation-model)
13. [Toolchain Quick Reference](#chapter-13--toolchain-quick-reference)

**[Part V — Learning by Doing](#part-v--learning-by-doing)**

14. [Exercise: The Lifetime Tracer](#chapter-14--exercise-the-lifetime-tracer) — seeing every copy, move, and death
15. [Exercise: The Buffer](#chapter-15--exercise-the-buffer) — the Rule of Five, for real
16. [The SDK Bestiary](#chapter-16--the-sdk-bestiary) — the shapes vendor APIs take in the wild
17. [Exercise: The FakeSDK](#chapter-17--exercise-the-fakesdk) — error codes and owned payloads (desktop-app style)
18. [Exercise: The Device SDK](#chapter-18--exercise-the-device-sdk) — opaque handles and C callbacks (peripheral style)
19. [Exercise: The Word Counter](#chapter-19--exercise-the-word-counter) — STL fluency end to end
20. [Exercise: Slicing and Polymorphism](#chapter-20--exercise-slicing-and-polymorphism) — the container that loses your data
21. [Exercise: Iterator Invalidation](#chapter-21--exercise-iterator-invalidation) — mutating while iterating, safely
22. [Exercise: Lambda Lifetimes](#chapter-22--exercise-lambda-lifetimes) — captures that outlive their scope
23. [Exercise: The Build-Model Lab](#chapter-23--exercise-the-build-model-lab) — provoking and reading every error stage
24. [Practice Plan](#chapter-24--practice-plan)
25. [Findings from Practice — a Living Log](#chapter-25--findings-from-practice-a-living-log)

**[Appendices](#appendices)**

- A. [Fundamentals Refresher](#appendix-a--fundamentals-refresher): pointers, references, explicit, = delete, const, .lib files
- B. [Core Principles](#appendix-b--core-principles-cheat-sheet) — the one-page cheat sheet
- C. [Working Without AI Assistants](#appendix-c--working-without-ai-assistants)
- D. [Resources, Further Reading, and First-Week Tips](#appendix-d--resources-further-reading-and-first-week-tips)

---

# Part I — The Mental Shift

---

## Chapter 1 — Ownership and RAII

In C#, you create objects and forget about them — the garbage collector cleans up eventually. In C++, **someone** must be responsible for deleting every object. That someone is the **owner**. RAII is the technique that makes ownership automatic instead of manual.

**RAII = Resource Acquisition Is Initialization.** Terrible name, simple idea: tie a resource's lifetime to an object's lifetime. Acquire the resource (memory, file, mutex lock) in the constructor, release it in the destructor. C++ **guarantees** the destructor runs when an object goes out of scope — even if an exception is thrown — so cleanup becomes automatic. Think of it as C#'s `using` block, except it is the default behavior of the whole language.

```cpp
void ProcessFile() {
    std::ifstream file("data.txt"); // opened here
    // ... use file ...
}   // <- destructor runs HERE, file closed. Always. No finally needed.
```

### The old bad way that RAII replaces

```cpp
Widget* w = new Widget();
DoStuff(w);      // if this throws...
delete w;        // ...this never runs. Memory leak.
```

### Smart pointers — RAII for heap memory

**`std::unique_ptr<T>`** — your default. Exactly one owner. Cannot be copied, only *moved* (ownership transfers). Zero runtime cost — compiles down to a raw pointer with an automatic delete.

```cpp
auto w = std::make_unique<Widget>();
// no delete anywhere, ever. When w goes out of scope, Widget dies.
```

**`std::shared_ptr<T>`** — multiple owners with a reference count; the object dies when the count hits zero. Closest to C# object semantics, but it has real cost (atomic counter). Using it everywhere is a code smell. Rule of thumb: *unique_ptr unless you can explain why shared*.

**`std::weak_ptr<T>`** — observes a shared_ptr without owning it. Solves the cycle problem: two objects holding shared_ptrs to each other never hit zero and leak — there is no GC to detect cycles like in C#. Pattern: parent holds shared_ptr to child, child holds weak_ptr back.

### Transferring ownership with unique_ptr

```cpp
std::unique_ptr<Widget> MakeWidget() {
    return std::make_unique<Widget>();   // ownership moves out
}
void Take(std::unique_ptr<Widget> w);    // ownership moves in

int main() {
    auto w = MakeWidget();       // I own it now
    // auto w2 = w;              // ERROR: can't copy
    auto w2 = std::move(w);      // OK: ownership transferred, w now empty
    Take(std::move(w2));         // gave it away
}   // nothing to clean up - Take's parameter deleted it
```

### The shared_ptr cycle trap

```cpp
struct Child;
struct Parent { std::shared_ptr<Child> child; };
struct Child  {
    std::shared_ptr<Parent> parent;  // BAD: cycle! Neither hits refcount 0
    // std::weak_ptr<Parent> parent; // GOOD: breaks the cycle
};
```

### Writing your own RAII wrapper (a shape to know cold)

```cpp
class FileHandle {
    FILE* f;
public:
    explicit FileHandle(const char* path) : f(std::fopen(path, "r")) {
        if (!f) throw std::runtime_error("open failed");
    }
    ~FileHandle() { if (f) std::fclose(f); }   // cleanup guaranteed

    // forbid copying (two copies would close the same file twice)
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    FILE* get() const { return f; }
};
```

The shape: acquire in constructor, release in destructor, delete the copy operations.

### RAII for a lock — compare to C# lock statement

```cpp
std::mutex m;
void AddItem() {
    std::lock_guard<std::mutex> guard(m);  // locks now
    items.push_back(42);
}                                          // unlocks here, always
```

### In the wild: C-style SDKs

Vendor SDKs — plug-in APIs, device SDKs, OS APIs — hand you raw resources (allocated payloads, handles, sessions) that you must release manually via a matching dispose/close/free function. The pro move is a small RAII guard per resource type, so the release runs on every path, including early error returns. Here is the shape against the miniature SDK you will meet in Chapter 17 (`ThingData` is a struct whose payload the SDK allocates and you must dispose):

```cpp
class ThingDataGuard {
    ThingData& data_;
public:
    explicit ThingDataGuard(ThingData& d) : data_(d) {}
    ~ThingDataGuard() { Thing_DisposeData(&data_); }
    ThingDataGuard(const ThingDataGuard&) = delete;
    ThingDataGuard& operator=(const ThingDataGuard&) = delete;
};

ErrCode ReadThing(size_t index, double* sum) {
    ThingData data = {};                       // zero-init: API struct idiom
    ErrCode err = Thing_GetData(index, &data);
    if (err != NoErr) return err;              // nothing allocated on failure

    ThingDataGuard guard(data);                // from here, disposal guaranteed
    return Thing_SumValues(&data, sum);        // early returns are now safe
}   // payload disposed automatically
```

Without the guard, every early `return` needs its own dispose call — the exact bug pattern legacy plug-in code is full of. Writing the guard is a five-minute investment that eliminates a whole leak class; this pattern is the workhorse of professional SDK code.

> **Key principle:** "In C++ I think in terms of ownership. Every resource has exactly one clear owner, expressed with unique_ptr or stack allocation. I basically never write new or delete by hand — raw new/delete in code is a bug waiting to happen."

---

---

## Chapter 2 — Value Semantics

**The single biggest mental shift from C#.** In C#, the type decides: class = reference, struct = value. In C++, **everything behaves like a C# struct by default** — assignment copies, passing copies, returning copies. Whether something is shared is decided *at the point of use*, not by the type's author.

```cpp
class Widget {          // 'class' keyword, but behaves like a C# STRUCT
public:
    std::string name;
};

Widget a;  a.name = "first";
Widget b = a;            // FULL COPY - two independent objects
b.name = "second";
std::cout << a.name;     // still "first"
```

(In C++, class and struct are identical except default visibility — private vs public. Nothing to do with copy semantics.)

### You choose the semantics per variable

```cpp
Widget b = a;    // copy - independent object
Widget& r = a;   // reference - alias, same object (C# class behavior)
Widget* p = &a;  // pointer - same object, can be null / reseated
auto s = std::make_shared<Widget>();  // shared ownership, closest to C# feel
```

### Trap 1 — accidental copies in loops

```cpp
for (auto w : widgets)         // copies EVERY widget
    w.selected = true;         // modifies the copies - vector unchanged!

for (auto& w : widgets)        // reference - the fix
    w.selected = true;

for (const auto& w : widgets)  // read-only pass - the idiom for viewing
    std::cout << w.name;
```

> **Trap:** The missing `&` is dead silent — compiles, runs, does nothing. One of the most common real-world C++ bugs.

### Trap 2 — modifying a copy returned from a function

```cpp
Widget GetSelected() { return selected_; }   // returns a COPY
GetSelected().name = "new";  // edits a temporary that dies instantly. No-op.
```

### Trap 3 — object slicing (the nastiest)

```cpp
class Shape  { public: virtual void Draw(); int x, y; };
class Circle : public Shape { public: void Draw() override; int radius; };

Circle c;
Shape s = c;          // COPIES only the Shape part. radius is GONE.
s.Draw();             // calls Shape::Draw - polymorphism lost!

std::vector<Shape> shapes;
shapes.push_back(c);  // sliced again
```

Polymorphism in C++ therefore requires pointers or references:

```cpp
Shape& r = c;
r.Draw();             // Circle::Draw - virtual dispatch works

std::vector<std::unique_ptr<Shape>> shapes;   // correct polymorphic container
shapes.push_back(std::make_unique<Circle>());
shapes[0]->Draw();    // Circle::Draw
```

Rule: **value types for data, pointers/references for polymorphism.** In C# every class object lives behind a reference automatically, so slicing cannot happen; in C++ you must ask for reference behavior.

### Why C++ is built this way — the payoff

Value semantics means objects live on the stack or inline inside containers — contiguous memory, no GC pressure, no pointer-chasing. A `std::vector<Point>` of a million points is one solid block of memory, cache-friendly and fast. The equivalent `List<Point>` with a Point class in C# is a million scattered heap objects. This is a big part of why C++ is the language of CAD engines — geometry code lives and dies by this. Copies also mean isolation: a function taking Widget by value cannot cause spooky action at a distance.

### In the wild: C-style SDKs

SDK structs are typically plain value types — created on the stack, zeroed with `= {}`, address passed to API functions to fill in. No heap, no ownership questions. But some structs *contain pointers to SDK-allocated data* (like Chapter 17's `ThingData.values`) — those need the RAII treatment from Chapter 1. Reading a vendor header and classifying each struct — pure value, or value-with-owned-payload? — is a daily skill in SDK work.

> **Key principle:** "C++ is value-semantic by default; I opt into reference semantics explicitly." | "I iterate with const auto& to avoid accidental copies." | "Polymorphic objects go behind unique_ptr — storing them by value slices them."

---

---

## Chapter 3 — Stack, Heap, and Undefined Behavior

The physical model underneath everything else in this book. C# hides it behind the GC; C++ makes you its manager — and punishes ignorance with undefined behavior.

### Stack vs heap, explicitly

A question worth being able to answer instantly: "where does this variable live?"

```cpp
void F() {
    int x = 5;                        // STACK: freed automatically at }
    Widget w;                         // STACK: whole object, dtor at }
    Widget* p = new Widget();         // w on HEAP, p itself on stack
    auto u = std::make_unique<Widget>();  // heap object, stack owner
    std::vector<int> v(1000);         // v's bookkeeping on stack,
}                                     // the 1000 ints on HEAP
```

**Stack**: allocation is one pointer bump — near-free; freed in reverse order automatically; small (~1 MB per thread typically) — huge arrays as locals overflow it; the natural home of value semantics and RAII. **Heap**: for objects that outlive the current scope, sizes unknown at compile time, or big data; slower (allocator work, cache misses); in modern C++ you touch it almost exclusively through containers and smart pointers, never bare new.

Contrast to internalize: in C# every class instance is heap + GC, full stop. In C++ heap use is a deliberate choice — and good C++ minimizes it. "Why is this on the heap?" is a legitimate code review question.

### Undefined behavior (UB) as a concept

UB is not "an exception is thrown" and not "the program crashes". It means **the standard places no requirements whatsoever** on what happens — and crucially, **the compiler is allowed to assume UB never occurs** and optimize accordingly. Result: code that works in Debug, breaks in Release; works on your machine, corrupts data in production; appears to work for years.

The greatest hits, all met in this book:

- dereferencing null or dangling pointers/references (lambda capturing dead locals, string_view to a temporary, c_str() outliving its string)
- out-of-bounds access: v[i] past the end, iterator invalidation (Chapter 11)
- use-after-move beyond assign/destroy (Chapter 6)
- deleting through a base pointer without a virtual destructor (Chapter 5)
- double-free (the shallow-copy bug behind = delete)
- signed integer overflow (unsigned wraps; signed is UB!), data races on unsynchronized shared data

Why C++ tolerates this: checks cost cycles, and C++'s contract is "you don't pay for what you don't use". The language trusts you; tooling backs you up:

```bash
# AddressSanitizer - catches heap/stack corruption, use-after-free at runtime
clang++ -fsanitize=address,undefined -g main.cpp
# also: MSVC /fsanitize=address, valgrind, and static analysis (clang-tidy)
```

> **Key principle:** "I treat warnings as errors, run sanitizers regularly, and reach for AddressSanitizer the moment anything smells like memory corruption."

The plug-in angle: when your code runs inside a host application — a CAD package, a DAW, an office suite — a memory bug in your plug-in doesn't crash your plug-in. It crashes the *host*, possibly minutes later in unrelated code, taking the user's unsaved work with it. That is why the discipline in this book (RAII, ownership, invalidation rules) is the job, not pedantry.

---

---

# Part II — The Language, Side by Side

---

## Chapter 4 — Classes, Inheritance, Interfaces

The mechanics you already know, in C++ spelling — plus the parts C# doesn't have at all.

### Class anatomy — the differences at a glance

```cpp
class Widget : public Shape {   // 'public' inheritance - see below
public:                          // access specifiers are SECTIONS,
    Widget();                    // not per-member keywords
    Widget(int size, std::string name);
    int GetSize() const { return size_; }        // no properties in C++ -
    void SetSize(int s) { size_ = s; }           // getters/setters by hand

protected:
    void Recalc();

private:
    int size_ = 0;               // default member initializers (C++11)
    std::string name_;
    static int count_;           // declared here, DEFINED in the .cpp:
};                               // <-- the semicolon! forget it and enjoy
                                 //     a cascade of bizarre errors

// Widget.cpp
int Widget::count_ = 0;                    // static member definition
Widget::Widget() : Widget(0, "unnamed") {} // delegating ctor (C++11)
```

Key deltas from C#: access specifiers label whole sections rather than each member; there are no properties (write Get/Set methods — no `get; set;` sugar); static data members need a separate definition in a .cpp (pre-C++17; `inline static` fixes it now); and the closing brace takes a **semicolon** — the classic returning-developer stumble.

### Constructors and the member initializer list

```cpp
class Widget {
    const int id_;          // const member - CANNOT be assigned in the body
    std::string name_;
    Shape& canvas_;         // reference member - same, must be initialized
public:
    Widget(int id, std::string name, Shape& canvas)
        : id_(id),                    // initializer list: members are
          name_(std::move(name)),     // CONSTRUCTED here, directly
          canvas_(canvas)
    {
        // body runs AFTER all members are already constructed.
        // name_ = name; here = construct empty, then assign - wasted work
    }
};
```

In C# assigning fields in the constructor body is normal. In C++ the **member initializer list** is the proper way: members are constructed once, directly, in **declaration order** (not list order — a compiler warning and a common surprise). const and reference members can ONLY be initialized here.

### Calling the base class

```cpp
class Circle : public Shape {
public:
    Circle(int x, int y, int r)
        : Shape(x, y),        // base ctor call - in the initializer list,
          radius_(r) {}       // not 'base(x, y)' like C#

    void Draw() override {
        Shape::Draw();        // no 'base.' keyword - name the class
        DrawOutline();        // explicitly: ClassName::Member
    }
private:
    int radius_;
};
```

### Inheritance access — public / protected / private

C# has one kind of inheritance. C++ has three; the keyword before the base name sets a ceiling on inherited member visibility:

```cpp
class Circle : public Shape    { };  // "is-a" - what C# does. Use this 99%.
class Circle : protected Shape { };  // inherited publics become protected
class Circle : private Shape   { };  // "implemented-in-terms-of" - outsiders
                                     // can't even treat Circle as a Shape
```

> **Trap:** For 'class' the DEFAULT is private inheritance — writing `class Circle : Shape` silently breaks polymorphism (`Shape* p = &circle;` won't compile). Always write `public` explicitly.

### Interfaces — no keyword, just a convention

```cpp
// C#: interface IDrawable { void Draw(); }
// C++: an abstract class with only pure virtuals and a virtual dtor
class IDrawable {
public:
    virtual ~IDrawable() = default;   // Chapter 5: always!
    virtual void Draw() = 0;          // = 0 -> pure virtual -> "abstract"
    virtual bool IsVisible() const { return true; }  // default impl allowed
};

class Widget : public IDrawable, public ISerializable {  // multiple bases OK
public:
    void Draw() override;
    void Serialize(Stream& s) override;
};
```

### Multiple inheritance and the diamond

C# forbids multiple base classes; C++ allows them — which is exactly how it does "implementing multiple interfaces". Full multiple inheritance of classes *with data* brings the famous diamond problem:

```cpp
class Device        { int id_; };
class Scanner : public Device { };
class Printer : public Device { };
class Copier  : public Scanner, public Printer { };
// Copier now contains TWO Device subobjects; copier.id_ is ambiguous!

// The fix - virtual inheritance (one shared Device):
class Scanner : virtual public Device { };
class Printer : virtual public Device { };
```

The stance to hold: "I keep multiple inheritance to interface-style bases — pure virtual, no data — which sidesteps the diamond entirely. Virtual inheritance exists but I treat needing it as a design smell."

### Odds and ends worth 10 seconds each

```cpp
struct Point { double x, y; };  // struct == class, just public by default;
                                // convention: struct for plain data bags

class Widget final { };         // 'final' = C# sealed (also per-method)

friend class Serializer;        // 'friend': grants ANOTHER class/function
                                // access to privates. No C# equivalent
                                // (closest: internal). Use sparingly.

// No universal root: C++ classes do NOT inherit from anything by default.
// There is no Object, no ToString/Equals/GetHashCode for free.

// 'this' is a POINTER (this->size_), not a reference like C#'s this.
```

| Feature | C# | C++ |
|---|---|---|
| Interfaces | interface keyword | abstract class, all pure virtual |
| Multiple base classes | no (interfaces only) | yes — use for interfaces; beware diamond |
| Properties | get; set; sugar | hand-written Get/Set methods |
| Base access | base.Method() | ClassName::Method() |
| Base ctor call | : base(args) | : BaseName(args) in init list |
| Seal a class | sealed | final |
| Universal root | System.Object | none — no default ToString/Equals |
| Field init | assign in ctor body | member initializer list (required for const/refs) |
| Default inheritance | always public | private for class! Write 'public' explicitly |

### In the wild: C-style SDKs

C++-side SDK layers use these patterns heavily: vendor base classes, interface-style pure virtual classes for observers and callbacks, and the occasional 'friend' in container internals. Your own model layer on top of any SDK is where you apply this — interface bases behind unique_ptr, always public inheritance, always virtual destructors.

---

---

## Chapter 5 — Virtual Dispatch and the Virtual Destructor

If a method is not marked **virtual**, the compiler decides which function to call based on the **variable's type, not the object's actual type**. C# would at least warn about hiding; C++ silently does the wrong thing.

```cpp
class Shape  { public: void Draw() { std::cout << "Shape"; } };   // NOT virtual
class Circle : public Shape {
public: void Draw() { std::cout << "Circle"; } };  // hides, doesn't override

Circle c;
Shape* p = &c;
p->Draw();     // prints "Shape"!  - static dispatch, compile-time decision
```

The fix, and the modern habit:

```cpp
class Shape  { public: virtual void Draw() { std::cout << "Shape"; } };
class Circle : public Shape {
public: void Draw() override { std::cout << "Circle"; } };  // prints "Circle"
```

### Always write override

It makes the compiler verify you are actually overriding. Without it, this classic bug compiles silently:

```cpp
class Circle : public Shape {
public:
    virtual void Draw() const { ... }  // oops: const mismatch - this is a NEW
                                       // function, not an override. No error!
};
// with 'override' the compiler catches it:
    void Draw() const override;        // ERROR: doesn't override anything
```

### Under the hood — the vtable

Each class with virtual functions gets a **vtable** — a hidden array of function pointers. Each object carries one hidden pointer to its class's vtable. A virtual call is a lookup through that pointer. Cost: one pointer per object, one indirection per call, no inlining. That is why C++ makes it opt-in — you do not pay unless you ask.

### The question that always comes up: why must a base destructor be virtual?

```cpp
class Shape { public: ~Shape() {} };            // NOT virtual - bug incoming
class Circle : public Shape {
    std::vector<Point> points_;                 // owns memory
public: ~Circle() { /* cleanup */ }
};

Shape* p = new Circle();
delete p;   // UNDEFINED BEHAVIOR: only ~Shape() runs.
            // ~Circle never called -> points_ leaks. Or worse.
```

```cpp
class Shape {
public:
    virtual ~Shape() = default;  // now delete p runs ~Circle, then ~Shape
};
```

> **Rule to recite:** "If a class has any virtual function, or is ever deleted through a base pointer, its destructor must be virtual." Corollary: if a class is not a polymorphic base, don't give it virtual anything.

Destructors chain automatically — derived first, then base. You never call the base destructor manually.

### Smaller relatives

```cpp
class Circle final : public Shape { ... };  // 'final' = C# sealed
void Draw() final;                          // no further overriding

class Shape {
public:
    virtual void Draw() = 0;      // pure virtual = C# abstract method
    virtual ~Shape() = default;
};  // class with only pure virtuals and no data = C++'s "interface"
```

> **Surprise for C# devs:** Virtual calls inside constructors/destructors do NOT dispatch to the derived class — during base construction the object still IS just the base. C# dispatches to the derived override (its own famous pitfall, in the opposite direction).

### In the wild: C-style SDKs

SDK surfaces are mostly C-style callbacks, but their C++ layers — and everything you build on top — use polymorphic hierarchies. Any class hierarchy you design for your own model needs virtual destructors the moment you store `unique_ptr<Base>` in a container. Which you will.

---

---

## Chapter 6 — The Rule of Five and Move Semantics

The heart of resource-owning classes — and the explanation of what `std::move` actually does.

### Setup: the special member functions

The compiler auto-generates up to five functions for every class. The generated versions copy/move each member field:

```cpp
class Buffer {
public:
    ~Buffer();                          // 1. destructor
    Buffer(const Buffer&);              // 2. copy constructor
    Buffer& operator=(const Buffer&);   // 3. copy assignment
    Buffer(Buffer&&);                   // 4. move constructor   (C++11)
    Buffer& operator=(Buffer&&);        // 5. move assignment    (C++11)
};
```

### The Rule of Zero (the modern ideal)

**Write none of the five.** Compose your class out of members that manage themselves (std::string, std::vector, unique_ptr) and the compiler-generated versions are automatically correct:

```cpp
class Document {
    std::string title_;
    std::vector<Page> pages_;
    std::unique_ptr<Renderer> renderer_;
    // NOTHING to write. Copy, move, destruction - all correct for free.
    // (copy is deleted because of unique_ptr - which is correct too)
};
```

### The Rule of Five

**If you must write any one of the five** (usually because you hold a raw resource), **you almost certainly need to write — or explicitly delete — all five.** Destructor without copy operations = the double-free bug. Copy without move = silent performance loss everywhere.

### What "move" actually means

Copying a resource-owning object duplicates the resource — expensive. Moving means **stealing**: the new object takes the guts (the pointer), the old object is left empty but valid.

```cpp
std::vector<int> a = MakeMillion();
std::vector<int> b = a;             // COPY: allocate + copy 1M ints
std::vector<int> c = std::move(a);  // MOVE: c takes a's pointer.
                                    // ~3 pointer assignments. a now empty.
```

> **The big reveal:** std::move moves nothing. It is just a cast — it marks a value as "you may steal from this", making the compiler select the move overload instead of the copy one. The stealing happens inside the move constructor. After moving from a variable, don't use it except to assign or destroy it.

### Rvalue references — the && syntax

`Buffer&&` means "reference to something I'm allowed to steal from": a temporary, or something explicitly marked with `std::move`.

```cpp
void Take(const Buffer& b);   // called for normal variables (copy)
void Take(Buffer&& b);        // called for temporaries / std::move'd things

Buffer x;
Take(x);              // first overload
Take(std::move(x));   // second - you granted permission to steal
Take(MakeBuffer());   // second - temporaries are fair game automatically
```

That last line is why C++11 made returning containers by value cheap — the return value is a temporary, so it is moved, not copied. Often not even that: compilers elide the move entirely ("RVO").

### The canonical exercise: Rule of Five for a raw buffer

Learn this shape cold:

```cpp
class Buffer {
    size_t size_ = 0;
    int*   data_ = nullptr;

public:
    explicit Buffer(size_t size)
        : size_(size), data_(new int[size]{}) {}

    // 1. Destructor
    ~Buffer() { delete[] data_; }

    // 2. Copy constructor - deep copy
    Buffer(const Buffer& other)
        : size_(other.size_), data_(new int[other.size_])
    {
        std::copy(other.data_, other.data_ + size_, data_);
    }

    // 3. Copy assignment - copy-and-swap idiom
    Buffer& operator=(const Buffer& other) {
        Buffer tmp(other);   // deep copy (may throw - fine, we're untouched)
        swap(tmp);           // steal tmp's guts
        return *this;
    }                        // tmp's destructor frees OUR old data

    // 4. Move constructor - steal and null out
    Buffer(Buffer&& other) noexcept
        : size_(other.size_), data_(other.data_)
    {
        other.size_ = 0;
        other.data_ = nullptr;  // CRITICAL: or its destructor frees OUR data
    }

    // 5. Move assignment
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            delete[] data_;           // free what we hold
            size_ = other.size_;
            data_ = other.data_;      // steal
            other.size_ = 0;
            other.data_ = nullptr;    // leave source empty-but-valid
        }
        return *this;
    }

private:
    void swap(Buffer& other) noexcept {
        std::swap(size_, other.size_);
        std::swap(data_, other.data_);
    }
};
```

### Details that separate working code from correct code

- **Nulling out the source in moves.** Forget it and the moved-from object's destructor deletes the data you just stole — double-free. The #1 bug in first attempts.
- **noexcept on move operations.** Not decoration: std::vector checks it. When reallocating, vector only moves your elements if the move can't throw — otherwise it falls back to copying for exception-safety. Omit noexcept and your type silently copies inside vectors.
- **Self-assignment check** (`if (this != &other)`) in move assignment — `a = std::move(a)` shouldn't destroy the data.
- **Copy-and-swap** for copy assignment: copy into a temp, then swap. If allocation throws, your object is untouched (the strong exception guarantee), and self-assignment is handled for free.

> **The stance to hold:** "In real code I'd never write this class — I'd hold `std::vector<int>` or `unique_ptr<int[]>` and get all five for free. Rule of Zero beats Rule of Five." Hand-rolling the five is a last resort; knowing how is what makes the shortcut safe.

### Where moves matter in daily code

```cpp
std::vector<Buffer> buffers;
buffers.push_back(std::move(myBuffer));  // move into container, no copy

widget.SetName(std::move(longString));   // sink params take by value + move

std::unique_ptr<Shape> s = std::make_unique<Circle>();
shapes.push_back(std::move(s));          // unique_ptr can ONLY move - this
                                         // is how ownership transfer is spelled
```

### In the wild: C-style SDKs

Large C++ SDKs often ship their own unique_ptr analog (an "Owner" or "ScopedRef" type) with the same move-only behavior. Any RAII guard you write around SDK handles is exactly the "class holding a raw resource" case — either delete copy/move entirely (simplest, as in Chapter 1's guard), or implement moves properly when guards must be stored in containers or returned from factories (as Chapter 18's DeviceSession does — with a subtle twist worth meeting there).

---

---

## Chapter 7 — Templates vs C# Generics

They look identical — `List<T>` vs `std::vector<T>` — but the machinery is completely different. **C# generics are one compiled thing that works for any T at runtime; C++ templates are a code-generation machine** — the compiler stamps out a separate, fully compiled version for *each* T you use, at compile time. This is called **instantiation**.

```cpp
template <typename T>
T Max(T a, T b) { return (a > b) ? a : b; }

Max(3, 5);        // compiler GENERATES int Max(int, int)
Max(2.5, 1.0);    // compiler GENERATES double Max(double, double)
Max(str1, str2);  // generates a std::string version
```

Nothing is decided at runtime — no boxing, no type checks, zero overhead. That is why `std::sort` on a `vector<int>` beats C's qsort: the comparison inlines completely.

### Consequence 1 — duck typing (and C++20 concepts)

C# demands constraints up front (`where T : IComparable<T>`). Templates declare nothing — the compiler just tries to compile your code with T. If T has `operator>`, it works; if not, you get an error at the point of use, often a notoriously long one ("template error novels").

```cpp
// C++20 concepts = C#'s where clauses, 20 years late
template <typename T>
requires std::totally_ordered<T>
T Max(T a, T b) { return a > b ? a : b; }

void Sort(std::ranges::range auto& container);   // terse form
```

### Consequence 2 — templates live in headers

The compiler must see the full template source to stamp out a version for your T, so template code cannot hide in a .cpp file — implementation and all go in the header. Put it in a .cpp and consumers get **linker errors** (unresolved external).

### Consequence 3 — templates are more powerful than generics

```cpp
template <typename T, int N>       // value parameters! C# cannot do this
class FixedArray {
    T data[N];                     // size baked in at compile time
};
FixedArray<double, 3> vec3;        // this is how std::array works
```

Non-type parameters, specialization, compile-time metaprogramming. Modern C++ prefers constexpr functions and concepts over the old arcane template tricks.

### Consequence 4 — no runtime type info via templates

`typeof(T)`, reflection, `GetType()` — none of that exists. T is gone after compilation. The little runtime typing C++ has is RTTI, working only on polymorphic types:

```cpp
Shape* p = GetShape();
Circle* c = dynamic_cast<Circle*>(p);   // like C# 'as' - nullptr if not Circle
if (c) c->radius = 5;
```

> **Key principle:** dynamic_cast is legal but culturally frowned upon — needing it often signals the virtual interface is designed wrong. "I'd prefer adding a virtual method over dynamic_cast chains."

### Trade-off summary

| | C# generics | C++ templates |
|---|---|---|
| When resolved | runtime (JIT) | compile time |
| Constraints | where, enforced upfront | none pre-C++20; concepts in C++20 |
| Performance | some overhead for ref types | zero — fully specialized code |
| Code location | anywhere | headers |
| Cost | — | slower builds, bigger binaries, ugly errors |
| Reflection on T | yes | no |

### In the wild: C-style SDKs

Established C++ SDKs frequently ship their own template container libraries paralleling the STL — Qt's `QVector`/`QMap`, Unreal's `TArray`/`TMap`, and many vendor equivalents born before the STL was trustworthy on all platforms. STL fluency translates directly: the concepts (and the invalidation rules) are the same, only the spelling differs. Expect to read the vendor's containers in API samples and convert at the boundary.

---

---

## Chapter 8 — Error Handling: Exceptions and Error Codes

C# is exceptions everywhere. C++ is split-brained: exceptions exist, but large parts of the ecosystem — most C-flavored SDKs, OS APIs, and plug-in interfaces — use C-style **error codes**. You need both.

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

Two rules to hold: **(1)** exceptions must never cross a DLL/plug-in boundary into a host application or a C API — catch everything at your entry points and convert to error codes; the code on the other side is not prepared for your exceptions. **(2)** destructors must never throw — one is already running during unwinding; a second exception terminates the program.

Exception-safety guarantees (worth knowing cold): **basic** — no leaks, object in some valid state; **strong** — operation succeeds or has no effect (copy-and-swap from Chapter 6 delivers this); **noexcept** — cannot throw. If a constructor throws, the object never existed — its destructor does NOT run, but already-constructed members ARE destroyed. That is why acquiring resources through RAII members is safe and raw acquisition in ctor bodies is not.

> **Key principle:** "I check every error code, use RAII so early returns can't leak, and never let an exception escape the plug-in boundary — I catch at entry points and translate to the SDK's error codes."

---

## Chapter 9 — Casts, Conversions, and Strings

### The four C++ casts

C-style casts like `(int)x` work but are a red flag in reviews — they can silently do any of four different things. Modern C++ names the intent:

| Cast | Purpose | C# analogy |
|---|---|---|
| `static_cast<T>(x)` | 'sensible' conversions: numeric, up/down class hierarchy when YOU know the type. No runtime check. | (int)x, explicit conversions |
| `dynamic_cast<T*>(x)` | checked downcast on polymorphic types; nullptr on failure (reference form throws). | as / is |
| `const_cast<T>(x)` | add or REMOVE const. Legitimate ~only for bad legacy APIs. Modifying an originally-const object is UB. | (none) |
| `reinterpret_cast<T>(x)` | reinterpret the bits: pointer-to-integer, unrelated pointer types. Danger zone; serialization/interop only. | unsafe pointer tricks |

```cpp
double d = 3.7;
int i = static_cast<int>(d);              // explicit, searchable, intentional

Shape* s = GetShape();
if (auto* c = dynamic_cast<Circle*>(s))   // checked downcast (needs vtable)
    c->radius = 5;

Derived* d2 = static_cast<Derived*>(s);   // UNchecked downcast: fast, but if
                                          // s isn't really a Derived -> UB
```

> **Key principle:** "I use static_cast for conversions I can prove, dynamic_cast when I must query at runtime — and I treat const_cast or reinterpret_cast in a code review as a question mark."

### Strings and encodings

C# strings are immutable, interned, UTF-16 objects. **std::string is a mutable byte buffer with no encoding awareness** — it stores bytes; whether they're ASCII, UTF-8, or garbage is your problem. The modern convention: keep std::string as UTF-8 everywhere.

```cpp
std::string s = "hello";
s += " world";              // mutable in place - no C# immutability
s[0] = 'H';                 // legal!
s.size();                   // BYTES, not characters - differs from what
                            // you'd expect with non-ASCII text!

const char* c = s.c_str();  // borrow a C-style pointer (valid only while
                            // s lives and is unmodified - dangling trap)
std::string_view v = s;     // non-owning view (Chapter 10)
```

Comparison is by value out of the box (`s1 == s2` compares contents), formatting is `std::format` (C++20, like string interpolation) or the classic streams.

The SDK reality — multiple string types in one function. Most large SDKs ship their own string class (Qt's `QString`, Windows' `BSTR`/`std::wstring`, many vendor "UniString" types), typically UTF-16 like C# strings internally. Conversions at the boundary are daily work:

```cpp
VendorString title("Wall label");          // vendor string: UTF-16 inside
// vendor <-> std::string conversions, encoding NAMED explicitly:
std::string utf8 = title.ToUtf8();
VendorString back = VendorString::FromUtf8(utf8);
```

> **Trap:** Encoding bugs are THE classic plug-in pitfall: user file and project names with non-ASCII characters (German umlauts, Cyrillic, CJK...) silently corrupt if you treat UTF-16 vendor strings as byte strings. Always convert explicitly with the encoding named.

---

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

## Chapter 11 — STL Containers, Algorithms, and Iterator Invalidation

### The container map

| C# | C++ | Notes |
|---|---|---|
| `List<T>` | `std::vector<T>` | your default, 95% of the time |
| `Dictionary<K,V>` | `std::unordered_map<K,V>` | hash table, O(1) |
| `SortedDictionary<K,V>` | `std::map<K,V>` | tree, O(log n), sorted iteration |
| `HashSet<T>` | `std::unordered_set<T>` | |
| `Queue<T>` / `Stack<T>` | `std::queue` / `std::stack` | |
| `LinkedList<T>` | `std::list<T>` | almost never the right choice |
| `T[]` | `std::array<T, N>` | fixed size, stack-allocated |

> **Gotcha:** plain `std::map` is the TREE (sorted, O(log n)); the Dictionary equivalent is `unordered_map`. "I'd use unordered_map for lookups unless I need sorted order."

### Why vector dominates — cache locality

A vector is one contiguous memory block. Even where a list is theoretically better, vector usually wins because CPUs prefetch contiguous memory. `push_back` is amortized O(1): when capacity runs out, the vector **reallocates** (typically doubling) and moves everything to a new block — remember that reallocation, it matters below.

### Key operations

```cpp
std::vector<Widget> v;
v.push_back(w);
v.emplace_back("name", 5);   // construct in place - prefer it
v[3];                        // no bounds check (fast, UB if out of range)
v.at(3);                     // bounds-checked, throws - like C# indexer

std::unordered_map<std::string, Widget> m;
m["wall"] = w;               // insert or overwrite
// TRAP: reading with [] INSERTS a default value if key missing!
auto it = m.find("wall");    // the safe lookup
if (it != m.end()) Use(it->second);   // ->first key, ->second value
if (m.contains("wall")) ...  // C++20, like ContainsKey
```

### Iterators

An iterator is a generalized pointer: `begin()` points at the first element, `end()` points **one past the last** (a sentinel — never dereference it). The half-open range [begin, end) is the universal STL currency.

```cpp
auto it = std::find(v.begin(), v.end(), target);
if (it != v.end()) {   // "not found" == end, the idiom
    Use(*it);
}
```

### Algorithms — LINQ's rough equivalent

| LINQ | STL (`<algorithm>` / `<numeric>`) |
|---|---|
| FirstOrDefault(pred) | `std::find_if(begin, end, pred)` — returns iterator |
| Count(pred) | `std::count_if(begin, end, pred)` |
| Any / All | `std::any_of / all_of / none_of` |
| OrderBy | `std::sort(begin, end, cmp)` — but IN PLACE |
| Select | `std::transform(begin, end, std::back_inserter(out), func)` |
| Aggregate / Sum | `std::accumulate(begin, end, 0)` |
| Max | `std::max_element` — returns ITERATOR to max |

Mindset shifts: STL algorithms mutate in place (LINQ returns new lazy sequences), and "Where" is awkward pre-C++20 — the classic filter is the famous **erase-remove idiom**:

```cpp
// remove all small widgets - the pre-C++20 incantation:
v.erase(std::remove_if(v.begin(), v.end(),
        [](const Widget& w){ return w.size < 10; }),
        v.end());
// remove_if only SHIFTS survivors forward and returns the new logical
// end; erase then chops the garbage tail. Two steps, always paired.

std::erase_if(v, [](const Widget& w){ return w.size < 10; });  // C++20
```

### THE trap: iterator invalidation

The C# equivalent — modifying a collection during foreach — throws immediately. C++ gives you **undefined behavior**: maybe a crash, maybe silent corruption, maybe it works on your machine and dies in production.

```cpp
// BUG - the classic:
for (auto it = v.begin(); it != v.end(); ++it) {
    if (it->size < 10)
        v.erase(it);       // 'it' is now INVALID; ++it next loop is UB
}

// FIX - erase returns the next valid iterator:
for (auto it = v.begin(); it != v.end(); /* nothing */) {
    if (it->size < 10)
        it = v.erase(it);  // step forward via the return value
    else
        ++it;
}
// or better: std::erase_if(v, pred);
```

Worse — **push_back can invalidate everything too**: if the vector grows, the whole block moves, and every iterator, pointer, and reference into it dangles. Appending while iterating is UB even though you deleted nothing.

Invalidation rules to memorize: **vector** — insert/erase invalidates iterators at/after the point, and ALL of them if reallocation happens. **unordered_map** — insertion can invalidate iterators (rehash) but references survive; erase kills only the erased. **map/list** — iterators stable except the erased element.

### In the wild: C-style SDKs

Vendor container libraries (Qt, Unreal, and countless in-house ones) mirror the STL: same concepts, same invalidation logic, different spelling. C APIs additionally return dynamic arrays via pointer+count pairs or opaque handles (Chapter 17's payload pattern), which you often immediately wrap or copy into proper containers so the rest of your code lives in STL-land.

---

---

# Part IV — The Build and the Toolchain

---

## Chapter 12 — The Compilation Model

In C# the compiler sees the whole project at once and assemblies carry metadata. C++ compilation is a relic of the 1970s that you must understand, because half of all confusing C++ errors are build-model errors, not logic errors.

### The pipeline

1. **Preprocessor** — dumb text machine. `#include "Widget.h"` literally copy-pastes the file's contents into your source.
2. **Compiler** — compiles each .cpp file completely independently into an object file (.obj/.o). Each .cpp + everything it included = one **translation unit**. The compiler has no idea other .cpp files exist.
3. **Linker** — stitches all object files together, matching "I call function X" with "here's the body of X".

### Declarations, definitions, and why headers exist

```cpp
// Widget.h - declarations: WHAT exists
#pragma once
class Widget {
public:
    void Draw();       // declared, not defined
private:
    int size_ = 0;
};

// Widget.cpp - definitions: HOW it works
#include "Widget.h"
void Widget::Draw() { /* body */ }

// Main.cpp - a consumer
#include "Widget.h"    // now I know Widget's shape
int main() { Widget w; w.Draw(); }  // linker connects call to Widget.cpp's body
```

A thing can be *declared* many times but *defined* exactly once across the program — the **One Definition Rule (ODR)**.

### Compile errors vs linker errors — read which stage failed

```text
error C2065: 'Widget': undeclared identifier
  -> COMPILE error: this translation unit never saw a declaration.
     Fix: missing #include.

error LNK2019: unresolved external symbol "void Widget::Draw(void)"
  -> LINKER error: compiled fine, but no object file contains Draw's body.
     Fix: .cpp not in project, library not linked, or declared-never-defined.
     (Also what you get putting a template's body in a .cpp.)
```

### Include guards

Since #include is paste, a header included twice via diamond paths would define the class twice in one translation unit. Every header, always:

```cpp
#pragma once        // modern

#ifndef WIDGET_H    // classic portable form
#define WIDGET_H
...
#endif
```

### Forward declarations — the build-time optimization

```cpp
// Renderer.h
class Widget;                       // forward declaration - "it exists"
class Renderer {
public:
    void Render(const Widget& w);   // fine - refs/pointers don't need size
private:
    Widget* current_;               // fine
    // Widget value_;               // NOT fine - needs full definition
};

// Renderer.cpp
#include "Widget.h"                 // full include belongs here
```

Why bother: **build times** — including Widget.h means every file including Renderer.h recompiles whenever Widget.h changes; in a CAD-sized codebase header hygiene is the difference between 5-minute and 2-hour builds. And **circular dependencies** — forward declarations break A-needs-B-needs-A deadlocks. Rule: include as little as possible in headers, forward-declare where you can, include fully in .cpp files.

### Two more, 30 seconds each

```cpp
namespace {                      // anonymous namespace in a .cpp:
    int Helper() { return 42; }  // private to this translation unit
}
// 'inline' historically = "definition allowed in multiple translation
// units without ODR violation" - why in-class bodies in headers are fine.
```

C++20 **modules** (import instead of #include) fix this whole mess — but adoption is slow and virtually every SDK ecosystem is headers all the way. Know they exist; expect to live in headers.

### What is a .lib file? (see Appendix A for full detail)

A **static library**: an archive of .obj files. The linker copies needed code into your binary. On Windows, DLLs also ship a tiny companion .lib — an **import library** of stubs telling the linker "function X lives in Foo.dll". Same extension, two different animals.

### In the wild: C-style SDKs

A plug-in is a DLL/bundle loaded by a host application; a device application links a vendor's driver library. Either way the trio applies: you compile against the SDK's headers, link against its .lib/.a files, and the host or driver exports the functions you call at runtime. Miss the header = compile error; miss the .lib = LNK2019; wrong SDK/runtime version = plug-in won't load or device won't open. Binary compatibility across DLL boundaries is a real C++ concern C# assemblies never have.

---

---

## Chapter 13 — Toolchain Quick Reference

### Compiler invocations

```bash
# clang / gcc (Mac, Linux) - strict + sanitizers + debug info:
clang++ -std=c++17 -Wall -Wextra -Wpedantic \
        -fsanitize=address,undefined -g main.cpp -o app

# MSVC (Windows, Developer Command Prompt):
cl /std:c++17 /W4 /EHsc /Zi /fsanitize=address main.cpp
```

- Treat warnings as your first code reviewer; fix them, don't silence them.
- Sanitizer builds are for development runs; they slow execution ~2x and are not for shipping.
- Debug vs Release matters more than in C#: UB often hides in Debug and detonates in Release. Test both.

### Debugging a plug-in inside a host application

- **Visual Studio:** Debug > Attach to Process > the host's .exe (or set your plug-in project's debug command to launch the host directly). Breakpoints in your plug-in code hit once the DLL is loaded.
- **Xcode:** edit the scheme's Run executable to point at the host .app; build-and-run then launches the host with your bundle debuggable.
- Symbols: keep Debug configuration for your plug-in even though the host ships without symbols — your frames are what matter in the call stack.
- If breakpoints don't bind: the loaded plug-in is not the one you just built. Check the file path the host loads vs your build output path — the #1 wasted-afternoon cause.

Learn your debugger's container visualizers (VS: built-in for vector/map; lldb: `frame variable`). Inspecting a vector without them is miserable; with them it's a C#-like experience.

### Vendor SDK setup checklist

- SDK major version usually must match the host application's major version exactly — check before anything else.
- Windows: check the required Visual Studio toolset version in the SDK docs; ABI mismatches produce baffling link and load errors.
- Mac / Apple Silicon: modern hosts expect arm64 or universal binaries; a mismatch shows up as a silent "plug-in won't load".
- Many SDKs have extra build steps beyond compiling (resource compilers, code generators, signing) — if UI elements or metadata don't appear, suspect those steps before the code.
- Plug-in/developer IDs often must be registered with the vendor for real distribution; samples use placeholders. Your employer likely has this handled — ask.

### Mac vs Windows for practice

C++ practice transfers 100% either way (clang + ASan on Mac is first-class). Toolchain muscle memory does not: if the team is a Windows/Visual Studio shop, do the SDK days on Windows so project settings, attach-to-process, and MSVC's error dialect become familiar. Ask the team which platform(s) they develop on before investing setup time.

---

---

# Part V — Learning by Doing

Before starting, skim [Chapter 24](#chapter-24--practice-plan) — the practice plan — which sequences these chapters into a one-week schedule. Then work each exercise **cold**: compiler, debugger, sanitizer, and offline docs as your only feedback loops, opening a chapter's reference solution only after your own attempt. The repository's `exercises/` directory carries a task card for every exercise (plus the vendor code for Chapters 17 and 18), so you can attempt each one without the solution on the next screen.

---

## Chapter 14 — Exercise: The Lifetime Tracer

One small class makes every lifetime rule in this book visible: a **Tracer** that prints from all six special member functions, stamped with an instance ID and its own address. Build it once, keep it forever — it is a diagnostic instrument, not a toy. When container or call behavior is mysterious, drop a Tracer in and the output replaces guesswork. It needs no tools a locked-down work machine lacks.

### The complete instrument

```cpp
// Tracer v2 - makes object identity, lifetime, and moves maximally visible.
// Build:  clang++ -std=c++17 -Wall -Wextra tracer.cpp -o tracer
//    or:  cl /std:c++17 /W4 /EHsc tracer.cpp

#include <iostream>
#include <string>
#include <utility>
#include <vector>

class Tracer {
public:
    explicit Tracer(std::string n)
        : name_(std::move(n)), id_(++counter_)
    {
        ++alive_;
        Log("constructed");
    }

    ~Tracer() {
        --alive_;
        Log("destroyed");
    }

    // ---- copy: duplicates, source untouched -------------------------------
    Tracer(const Tracer& other)
        : name_(other.name_), id_(++counter_)
    {
        ++alive_;
        Log("copy-CONSTRUCTED from " + other.Label());
    }

    Tracer& operator=(const Tracer& other) {
        // std::string::operator= releases our old buffer internally.
        // With a raw resource, WE would have to release it here. (Finding 4)
        name_ = other.name_;
        Log("copy-ASSIGNED from " + other.Label());
        return *this;
    }

    // ---- move: steals, source becomes a husk ------------------------------
    Tracer(Tracer&& other) noexcept
        : name_(std::move(other.name_)), id_(++counter_)   // steal FIRST (Finding 1)
    {
        ++alive_;
        Log("move-CONSTRUCTED, gutting " + other.MarkHusk());
    }

    Tracer& operator=(Tracer&& other) noexcept {
        if (this != &other) {                              // self-move guard (Finding 5)
            name_ = std::move(other.name_);                // steal
            Log("move-ASSIGNED, gutting " + other.MarkHusk());
        }
        return *this;
    }

    // ---- bookkeeping ------------------------------------------------------
    static void Report() {
        std::cout << "---- " << alive_ << " object(s) still alive, "
                  << counter_ << " ever created ----\n";
    }

private:
    // "name#id @address" - id distinguishes objects sharing a name;
    // address proves identity (RVO: 'temp' and 'd' print the SAME address).
    std::string Label() const {
        return name_ + "#" + std::to_string(id_) + " @" + Addr();
    }

    std::string Addr() const {
        char buf[32];
        std::snprintf(buf, sizeof buf, "%p", static_cast<const void*>(this));
        return buf;
    }

    std::string MarkHusk() {
        std::string was = Label();      // capture identity BEFORE overwriting
        name_ = "husk";                 // tracing sugar only - real moved-from
        return was;                     // strings are simply left empty
    }

    void Log(const std::string& what) const {
        std::cout << Label() << "  " << what << '\n';
    }

    std::string name_;
    int id_;
    inline static int counter_ = 0;     // total ever created
    inline static int alive_   = 0;     // live right now (leak detector)
};

// ---------------------------------------------------------------------------

Tracer MakeTracer() {
    Tracer t("temp");
    return t;                           // watch: NO copy, NO move (RVO)
}

void ByValue(Tracer t) { (void)t; }
void ByRef(const Tracer& t) { (void)t; }

int main() {
    std::cout << "--- singles ---\n";
    Tracer a("a");
    Tracer b = a;                       // copy: b is a NEW object (new id)
    Tracer c = std::move(a);            // move: a becomes a husk

    std::cout << "--- calls ---\n";
    ByValue(b);                         // copy in, destroyed at return
    ByRef(b);                           // SILENCE - binds directly, no object
    ByValue(std::move(b));              // move in - b gutted

    std::cout << "--- RVO ---\n";
    Tracer d = MakeTracer();            // one construction; compare the
                                        // address with d's destructor line!
    std::cout << "--- vector ---\n";
    std::vector<Tracer> v;
    v.push_back(Tracer("v1"));          // temp constructed, moved in, temp dies
    v.push_back(Tracer("v2"));          // + REALLOCATION: v1 moves to new block
                                        // (delete noexcept above -> it COPIES)
    Tracer::Report();                   // everything from main still alive

    std::cout << "--- teardown (reverse order per scope) ---\n";
    return 0;
}   // vector first (its elements), then d, c, b, a

// After main, statics persist; a final Report() via atexit would show 0 alive.
```

### Design choices worth stealing

- **Instance IDs separate identity from name.** Copies share a name; the `#id` makes each object unambiguous: `a#2 copy-CONSTRUCTED from a#1`.
- **Addresses turn claims into proofs.** Same address across "constructed" and "destroyed" lines = same object. Stack addresses vs heap addresses are visibly different ranges.
- **Moves steal in the initializer list** — `name_(std::move(other.name_))` — then log. Note the move constructor prints its *own* `name_`, because by the time the body runs the theft has already happened.
- **Husk marking** (`t.name_ = "husk"`) is tracing sugar: it makes the teardown roll-call show which objects ended life gutted. Real moved-from strings are simply left empty.
- **The sink-parameter constructor** — `explicit Tracer(std::string n) : name_(std::move(n))` — takes by value and moves in: one overload optimally handles both copies and moves (Chapter 6).
- **`inline static` counters** (C++17) — static members defined in-class, no separate .cpp definition needed; the modern fix for the Chapter 4 annoyance.
- **The alive counter is a leak detector**: every construction increments, every destruction decrements; a nonzero count at exit means RAII bookkeeping is broken somewhere.

### Annotated output of a real run

```text
--- singles ---
a#1 @0x7fff...d90  constructed
a#2 @0x7fff...dc0  copy-CONSTRUCTED from a#1 @0x7fff...d90
a#3 @0x7fff...df0  move-CONSTRUCTED, gutting #1 @0x7fff...d90
```

`b = a` created a new object (#2) at a new address. `std::move(a)` let #3 steal from #1 — note the gutted label prints an *empty name*: honestly stolen before printing. `a` (#1) still exists as a husk; it gets a normal destructor at the very end.

```text
--- calls ---
a#4 ...  copy-CONSTRUCTED from a#2 ...
a#4 ...  destroyed
a#5 ...  move-CONSTRUCTED, gutting #2 ...
a#5 ...  destroyed
```

`ByValue(b)`: a copy lives exactly for the call. **`ByRef(b)` printed nothing at all** — `const T&` binds directly, no object created; that silence is why it is the default parameter idiom. `ByValue(std::move(b))`: same function, but permission was granted, so the parameter *moved* — and b is now a husk.

```text
--- RVO ---
temp#6 @0x7fff...e20  constructed
```

That is the **entire** output of `Tracer d = MakeTracer()`. One construction; no copy, no move. The compiler built "temp" directly in d's memory — the local, the return value, and d are one object (compare this address with d's destructor line at teardown: identical). This is RVO/copy elision, and it is why returning objects by value in modern C++ is free.

```text
--- vector ---
v1#7 @0x7fff...e50  constructed          <- temporary, on the STACK
v1#8 @0x5622...2c0  move-CONSTRUCTED...  <- moved into the vector: HEAP address!
husk#7 ...  destroyed                    <- temporary husk dies
v2#9 ...   constructed
v2#10 @0x5622...398  move-CONSTRUCTED...
v1#11 @0x5622...370  move-CONSTRUCTED, gutting #8 @0x5622...2c0   <- REALLOCATION
husk#8 ...  destroyed
husk#9 ...  destroyed
```

The address ranges expose stack vs heap directly (Chapter 3). And the second `push_back` triggered **reallocation**: v1 was picked up and move-constructed *again* at a new heap address — you can watch the vector carry its contents to a bigger block. Delete `noexcept` from the move constructor and this line becomes `copy-CONSTRUCTED`: the vector falls back to copying when the move might throw (Chapter 6, Finding 3 of Chapter 25).

```text
---- 6 object(s) still alive, 11 ever created ----
--- teardown (reverse order per scope) ---
v1#11 destroyed   v2#10 destroyed        <- vector's elements first
temp#6 destroyed                          <- d (which IS temp: RVO)
a#3 destroyed
husk#2 destroyed  husk#1 destroyed        <- the gutted b and a, normal destruction
```

Eleven constructions, eleven destructions — balanced books, no leaks. Destruction runs in reverse construction order within each scope. The husks in the roll-call are visual proof of which objects were genuinely emptied.

### Three experiments to run

1. **Delete `noexcept`** from the move constructor. The reallocation line flips from move to copy — vector's exception-safety rule, observed live.
2. **Add `v.reserve(4);`** before the push_backs. The entire reallocation block vanishes — no growth, no transfer.
3. **Add `Tracer x("x"); x = x;`** — self-copy-assignment. The copy assignment here has no self-check and survives only because `std::string::operator=` tolerates it. Ask yourself what happens when the member is a raw pointer: that question is the doorway to the Buffer worked example in Chapter 15.

---

## Chapter 15 — Exercise: The Buffer

The Tracer (Chapter 14) had `std::string` silently doing the dangerous work. The Buffer replaces it with a raw `int*` — the same five functions, but now every ordering mistake is a heap corruption instead of a style nit. This chapter contains the exercise, the reference solution, and the four findings a real first attempt produced (logged as Findings 6–9 in Chapter 25).

### The exercise

Write from memory: a class owning a heap array of ints (`size_`, `data_`). Requirements: destructor frees; copy constructor deep-copies; copy assignment (aim for copy-and-swap); move constructor steals **and nulls the source**; move assignment frees own data, steals, nulls, self-move-safe; `noexcept` where it is *true*; `explicit` where it belongs; a zero-initialized buffer; and an element accessor.

Then: a `main` exercising all five paths with predictions written as comments before running — including assignment over an *existing* buffer, a vector with reallocation, and `b = std::move(b)`.

Then the sabotage runs under AddressSanitizer (see "Experiments" below).

### Reference solution

<details>
<summary><strong>Show the solution — do the exercise cold first</strong></summary>

```cpp
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <utility>
#include <vector>

class Buffer {
public:
    explicit Buffer(size_t size)
        : size_(size), data_(new int[size]{})   // {} => zero-initialized (Finding 7)
    {}

    ~Buffer() { delete[] data_; }               // delete[] on nullptr is a safe no-op

    // ---- copy: deep, exception-safe ---------------------------------------
    Buffer(const Buffer& other)
        : size_(other.size_), data_(new int[other.size_])
    {
        std::copy(other.data_, other.data_ + size_, data_);
    }

    Buffer& operator=(const Buffer& other) {    // copy-and-swap (Finding 6)
        Buffer tmp(other);   // ALL throwing work happens here; *this untouched
        Swap(tmp);           // three noexcept pointer exchanges
        return *this;
    }                        // tmp's destructor frees our OLD block

    // ---- move: steal and null out -----------------------------------------
    Buffer(Buffer&& other) noexcept
        : size_(std::exchange(other.size_, 0)),
          data_(std::exchange(other.data_, nullptr))
    {}

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {                   // self-move guard
            delete[] data_;                     // safe HERE: nothing below throws
            size_ = std::exchange(other.size_, 0);
            data_ = std::exchange(other.data_, nullptr);
        }
        return *this;
    }

    // ---- access -------------------------------------------------------------
    size_t Size() const noexcept { return size_; }

    int&       At(size_t i)       { assert(i < size_); return data_[i]; }  // Finding 8
    const int& At(size_t i) const { assert(i < size_); return data_[i]; }

    void Swap(Buffer& other) noexcept {
        std::swap(size_, other.size_);
        std::swap(data_, other.data_);
    }

private:
    size_t size_ = 0;
    int*   data_ = nullptr;
};

int main() {
    Buffer a(5);
    a.At(2) = 42;                    // writable now (Finding 8)

    Buffer b = a;                    // copy ctor: deep copy
    Buffer c(3);
    c = a;                           // copy assignment via copy-and-swap
    std::cout << "c[2]=" << c.At(2) << " (expect 42)\n";

    const Buffer& view = a;
    std::cout << "view[2]=" << view.At(2) << " (const overload)\n";

    Buffer d = std::move(a);         // move ctor: a is now empty husk
    std::cout << "a.Size()=" << a.Size() << " (expect 0)\n";

    c = std::move(d);                // move assignment
    c = std::move(c);                // self-move: must be harmless
    std::cout << "c[2]=" << c.At(2) << " after self-move (expect 42)\n";

    std::vector<Buffer> v;
    v.push_back(Buffer(2));
    v.push_back(Buffer(4));          // reallocation: moves (noexcept honest)
    std::cout << "vector ok, sizes " << v[0].Size() << "," << v[1].Size() << "\n";
    return 0;
}
```

Notes on the choices:

- **`new int[size]{}`** — the braces zero-initialize. Without them the contents are indeterminate and reading them is UB (Finding 7).
- **Copy assignment is copy-and-swap**: the copy constructor does all throwing work into `tmp` while `*this` is untouched; `Swap` is three noexcept pointer exchanges; `tmp`'s destructor frees the old block on the way out. Strong exception guarantee and self-assignment safety fall out for free (Finding 6).
- **`std::exchange(other.data_, nullptr)`** — "take the old value, leave this one" in a single expression; the idiomatic steal-and-null. Equivalent to two lines, harder to forget the second.
- **Move assignment deletes first and that is fine here** — nothing after the `delete[]` can throw. The same shape inside *copy* assignment is a bug (Finding 6): the difference is what follows the delete.
- **The const-overload pair for `At`** — `int& At(size_t)` plus `const int& At(size_t) const`. Same name; the compiler picks by the constness of the object. This is exactly how `vector::operator[]` works (Finding 8). The `assert` documents the bounds contract; throwing `std::out_of_range` would be the `.at()`-style alternative.
- **Destructor is one line** — `delete[]` on `nullptr` is a safe no-op, and nulling members in a destructor is dead work: the object ceases to exist in the next instant (Finding 9).
- The deliberate `c = std::move(c)` in `main` draws a compiler warning (`-Wself-move`) — good: the compiler flags suspicious code, and the class survives it anyway, which is the requirement.

</details>

### The unhappy path, step by step

The instructive bug from a real first attempt — release-before-acquire copy assignment:

```cpp
Buffer& operator=(const Buffer& other) {
    if (this != &other) {
        delete[] data_;              // old block GONE. data_ dangles.
        size_ = other.size_;
        data_ = new int[size_];      // <- suppose THIS throws bad_alloc
        std::copy(...);
    }
    return *this;
}
```

If the allocation throws, the exception propagates out mid-function and the object is a zombie: `data_` still points at freed memory. During stack unwinding its destructor runs — `delete[] data_` — **freeing the same block twice**. The assignment didn't just fail to copy; it corrupted the heap on the way out. The principle: **do all the throwing work before touching your own state.** Copy-and-swap makes the correct ordering structural rather than a discipline to remember.

### Experiments (sabotage runs under ASan)

Take a working copy, break one thing at a time, build with `-fsanitize=address -g`, run, and read each report until it makes sense:

1. **Remove the null-out in the move constructor** → double-free; ASan shows both freeing stacks.
2. **Make copy assignment shallow** (`data_ = other.data_;`) → double-free plus a leak of the orphaned old block.
3. **Remove the self-move guard**, run `b = std::move(b)` → reason first about whether your implementation gives use-after-free or silent data loss, then verify.
4. **Simulate the unhappy path**: revert to release-before-acquire and insert `throw std::bad_alloc{};` after the `delete[]` — watch Finding 6 detonate.

### Why you would never ship this class

`std::vector<int>` (or `std::unique_ptr<int[]>`) already is this class, written by experts, tested for decades — holding one of those as the member gives all five operations for free. Rule of Zero beats Rule of Five (Chapter 6). Hand-rolling the five is for the rare type that *is* the resource wrapper — and knowing how is precisely what makes the shortcut safe to take everywhere else.

---

## Chapter 16 — The SDK Bestiary

Native SDKs come in a small number of recurring shapes. Learn to recognize the shape and you already know half the SDK before opening its docs — the wrapping strategy, the failure modes, and where the RAII goes. This chapter is the field guide; Chapters 17 and 18 are hands-on training on the two most common shapes.

### Shape 1 — Error codes + out-parameters + owned payloads (desktop-application plug-in SDKs)

Every function returns a status code; results come back through pointers you pass in; some structs carry SDK-allocated payloads with a matching dispose function. This is the classic shape of plug-in APIs for large desktop applications — CAD packages, DAWs, office suites — and of venerable C libraries like **SQLite** and **zlib**, which are worth reading as masterclasses in the style. Your job: check every code, zero-init every struct, guard every payload. **Trained in Chapter 17.**

### Shape 2 — Opaque handles + open/close + callbacks (device and I/O SDKs)

You receive a pointer to a type you cannot see inside (`typedef struct DeviceImpl* DeviceHandle`); a create/open function hands it out, a close/destroy function takes it back, and events arrive through registered C function pointers carrying a `void*` context. This is the shape of essentially every peripheral SDK: **libusb** and **HIDAPI** (USB/HID devices), **PortAudio** and ASIO (audio interfaces), serial-port and camera SDKs, **Vulkan** (`VkDevice`, `VkInstance` — a modern API deliberately built in this classic shape), and printer/scanner vendor SDKs. Your job: a move-only RAII session per handle, and a trampoline bridging the C callback into C++. **Trained in Chapter 18.**

Two hazards specific to this shape, worth knowing before you meet them for real: **callback threading** — real device SDKs often invoke your callback from a driver thread, not yours, so everything the callback touches needs synchronization (the FakeDevice of Chapter 18 calls back synchronously to keep the exercise focused; the chapter notes what changes when it doesn't); and **callback lifetime** — the SDK holds your function pointer and context until you unregister; if the object behind the context dies first, the next event is a use-after-free delivered by the driver.

### Shape 3 — Reference counting (COM and COM-flavored APIs)

Objects expose `AddRef`/`Release` (or retain/release) and you own one reference per acquisition; functions return `HRESULT` status codes; interfaces are queried by ID. This is **COM** — the substrate of huge parts of Windows: Office automation, DirectX, the shell, WinRT underneath its projections — and the retain/release pattern reappears in Core Foundation on Apple platforms. The C++ treatment: never call `Release` by hand; use the ecosystem's RAII smart pointers (`Microsoft::WRL::ComPtr`, `winrt::com_ptr`, `CComPtr`), which are exactly shared_ptr's discipline with a different spelling. If your work touches Windows deeply, this shape deserves dedicated study; recognizing it as "shared_ptr, someone else's implementation" is the starting point.

### Shape 4 — Init/deinit lifecycles and status enums (embedded HALs and middleware)

A global or per-peripheral `X_Init(&config)` / `X_DeInit()` pair, status enums (`HAL_OK`, `HAL_ERROR`, `HAL_BUSY`, `HAL_TIMEOUT`), configuration structs you zero and fill, and callbacks that are actually interrupt handlers. This is the shape of microcontroller vendor HALs (**STM32 HAL**, **ESP-IDF**, Nordic's SDKs) and much industrial middleware (CAN stacks, Modbus libraries). It is Shape 1 wearing work boots: the same discipline applies, with two additions — callbacks may run in interrupt context (minimal work, no allocation, no blocking), and RAII must respect that some resources are singletons whose "ownership" is initialization order.

### Shape 5 — C++-native SDKs (engines and frameworks)

Some SDKs are genuinely C++: **Qt**, **Unreal**, **JUCE**, many game and media engines. Here the vendor ships its own containers, strings, and smart pointers (Chapter 7's "In the wild"), its own object lifetime rules (Qt's parent-child ownership; Unreal's garbage collector for UObjects — yes, a GC in C++), and often its own build layer (moc, UnrealBuildTool). The transition skill: identify which of *their* mechanisms replaces which standard one, use theirs inside their world, and convert at the boundary. Fighting a framework's ownership model with raw standard idioms is a rite of passage best skipped.

### The universal checklist, whatever the shape

For every SDK function you meet, answer four questions before calling it: **Who allocates?** (me, via a struct I fill; or the SDK, via a payload I must release). **Who releases, and with which exact function?** (free/dispose/close/Release are not interchangeable). **What is the failure contract?** (code returned? struct touched or untouched on failure? — Chapter 17's documentation trap). **What threads can this be called on, and what thread calls me back?** Wrap the answers in a guard type, and the rest of your code never thinks about them again. That habit — one small RAII type per SDK resource — is the single highest-leverage practice in native SDK work, and it is what Chapters 17 and 18 drill.

---

## Chapter 17 — Exercise: The FakeSDK

*Trains: Chapter 1 (RAII), Chapter 8 (error codes), Chapter 12 (multiple translation units). Time: ~90 min. This is the closest exercise to real plug-in work: the function you will write is structurally identical to "aggregate a property over all elements" against any desktop-application SDK.*

### The vendor code

Two files, `FakeSDK.h` and `FakeSDK.cpp` — **read them, compile them, link them, never edit them.** The header is the contract; every convention in it mirrors the classic desktop-SDK idiom: every function returns `ErrCode` (0 = success), "Get" functions fill caller-provided structs passed by address, and `Thing_GetData` allocates a payload the caller must release with `Thing_DisposeData` exactly once. The SDK has a built-in leak detector: `FakeSdk_LiveAllocations()` must be **0** after your code runs.

```cpp
// ============================================================================
// FakeSDK.h - a miniature C-style API in the classic desktop-SDK idiom.
// DO NOT MODIFY THIS FILE. Treat it as vendor code: read it, wrap it, obey it.
//
// Conventions (the classic C-flavored desktop-SDK idiom):
//   - every function returns ErrCode; 0 (NoErr) means success
//   - "Get" functions fill caller-provided structs passed by address
//   - ThingData owns heap allocations made by the SDK; the caller MUST
//     release them with Thing_DisposeData exactly once
//   - passing null pointers is an error (ErrNullParam), not a crash
// ============================================================================
#pragma once
#include <cstddef>
#include <cstdint>   // SIZE_MAX, used as "no index" in FakeSdk_Setup

using ErrCode = int;

constexpr ErrCode NoErr        = 0;
constexpr ErrCode ErrNullParam = 1;   // a required pointer was null
constexpr ErrCode ErrBadIndex  = 2;   // no Thing with that index
constexpr ErrCode ErrNoData    = 3;   // Thing exists but has no payload
constexpr ErrCode ErrInternal  = 4;   // simulated transient failure

// A Thing's payload. 'values' is allocated BY THE SDK inside Thing_GetData;
// the caller owns disposal via Thing_DisposeData. All other fields are inline.
struct ThingData {
    int     id;          // stable identifier of the Thing
    size_t  valueCount;  // number of entries in 'values'
    double* values;      // SDK-allocated array; null until Thing_GetData
};

// How many Things exist in the "project". Never fails if count is non-null.
ErrCode Thing_GetCount(size_t* count);

// Fill 'data' for the Thing at 'index' (0-based).
//   - allocates data->values (caller must dispose)
//   - on ANY failure, 'data' is left untouched and nothing is allocated
// Note: some Things in the project legitimately have no payload and
// return ErrNoData. Others may fail transiently with ErrInternal.
ErrCode Thing_GetData(size_t index, ThingData* data);

// Release the payload of 'data'. Safe on a zeroed struct. After the call,
// data->values is null and valueCount is 0. Calling twice is safe;
// calling on a struct whose 'values' you overwrote by hand is not.
ErrCode Thing_DisposeData(ThingData* data);

// Sum of all entries in data->values. Requires a non-null, filled 'data'.
ErrCode Thing_SumValues(const ThingData* data, double* sum);

// Test-support: configure the fake project. 'failAtIndex' makes
// Thing_GetData return ErrInternal for that index (pass SIZE_MAX for none).
void FakeSdk_Setup(size_t thingCount, size_t emptyIndex, size_t failAtIndex);

// Test-support: how many SDK allocations are currently live.
// After your code runs, this MUST be zero - it is the leak detector.
size_t FakeSdk_LiveAllocations();
```

(The matching `FakeSDK.cpp` implements this contract and ships with the repository. Build with both translation units: `g++ -std=c++17 -Wall -Wextra -fsanitize=address -g FakeSDK.cpp yourfile.cpp -o task`.)

### The task

**Part A** — `ThingDataGuard`: an RAII wrapper ensuring disposal on every path (the guard shape from Chapter 1). Decide and be ready to defend: copyable? movable? neither?

**Part B** — the worker:

```cpp
// Sums the values of every Thing in the project.
// Things with no payload (ErrNoData) are skipped and counted, not errors.
// Any other failure aborts and propagates the code - with NO leaks.
ErrCode SumAllThings(double* total, size_t* skippedCount);
```

Check **every** return code; `ErrNoData` is a normal skip; other failures propagate; early returns must not leak; validate your own parameters the way the SDK validates its own. Style target: flat early-return chains, not nested ifs (Chapter 8).

**Part C** — three scenarios in `main`, predictions computed **by hand** as comments before running, asserting *values* — not just "no crash" (Finding 10): a happy path with one empty Thing; a mid-loop transient failure (the critical check: were the payloads of the already-read Things disposed?); and an empty project (decide what "correct" even means there).

### Reference solution

<details>
<summary><strong>Show the solution — do the exercise cold first</strong></summary>

```cpp
// FakeSDK exercise - reference solution.
#include "FakeSDK.h"
#include <cassert>
#include <iostream>

// Part A - the RAII guard. Non-copyable, non-movable: it aliases one struct
// for one scope; copying would double-dispose, moving has no use case here.
class ThingDataGuard {
    ThingData& d_;
public:
    explicit ThingDataGuard(ThingData& d) : d_(d) {}
    ~ThingDataGuard() { Thing_DisposeData(&d_); }   // safe even if never filled
    ThingDataGuard(const ThingDataGuard&) = delete;
    ThingDataGuard& operator=(const ThingDataGuard&) = delete;
};

// Part B - the worker. Flat early returns; every code checked; no leaks.
ErrCode SumAllThings(double* total, size_t* skippedCount) {
    if (!total || !skippedCount) return ErrNullParam;   // validate like the SDK does
    *total = 0;
    *skippedCount = 0;

    size_t count = 0;
    ErrCode err = Thing_GetCount(&count);
    if (err != NoErr) return err;

    for (size_t i = 0; i < count; ++i) {
        ThingData data = {};                    // zero-init: values == nullptr
        err = Thing_GetData(i, &data);
        if (err == ErrNoData) {                 // documented: nothing allocated
            ++*skippedCount;                    // on failure -> safe to just skip
            continue;
        }
        if (err != NoErr) return err;           // ditto: nothing to dispose

        ThingDataGuard guard(data);             // from here, disposal guaranteed

        double sum = 0;
        err = Thing_SumValues(&data, &sum);
        if (err != NoErr) return err;           // guard disposes on this exit
        *total += sum;
    }                                           // guard disposes each iteration
    return NoErr;
}

int main() {
    double total; size_t skipped; ErrCode err;

    // Scenario 1: 4 Things, index 2 empty. Hand-computed expectation:
    // thing0: 3 vals 0,1,2        -> 3
    // thing1: 4 vals 10..13       -> 46
    // thing2: skipped
    // thing3: 3 vals 30,31,32     -> 93        total = 142, skipped = 1
    FakeSdk_Setup(4, 2, SIZE_MAX);
    err = SumAllThings(&total, &skipped);
    assert(err == NoErr && skipped == 1 && total == 142.0);
    assert(FakeSdk_LiveAllocations() == 0);
    std::cout << "scenario1 ok: total=" << total << " skipped=" << skipped << "\n";

    // Scenario 2: Thing 2 fails transiently. Things 0,1 were read first -
    // the CRITICAL check is that their payloads were disposed on the abort.
    FakeSdk_Setup(4, SIZE_MAX, 2);
    err = SumAllThings(&total, &skipped);
    assert(err == ErrInternal);
    assert(FakeSdk_LiveAllocations() == 0);     // Finding 10: check VALUES
    std::cout << "scenario2 ok: propagated err=" << err << ", no leaks\n";

    // Scenario 3: empty project. Correct = NoErr, total 0, skipped 0.
    FakeSdk_Setup(0, SIZE_MAX, SIZE_MAX);
    err = SumAllThings(&total, &skipped);
    assert(err == NoErr && total == 0.0 && skipped == 0);
    std::cout << "scenario3 ok: empty project is a valid, zero result\n";

    // Robustness: our own null-param contract.
    assert(SumAllThings(nullptr, &skipped) == ErrNullParam);
    return 0;
}
```

</details>

### Pitfalls this exercise plants — and why they matter

**The documentation trap.** The header states: *"on ANY failure, 'data' is left untouched and nothing is allocated."* That single sentence is what makes `continue` after `ErrNoData` — and `return` after other errors — safe *without* a guard at those points. Miss it, and you either dispose something never allocated (harmless here because `Thing_DisposeData` tolerates zeroed structs — but only because you zero-initialized with `= {}`), or you wrap the guard too early and reason about it wrongly. Vendor docs reward forensic reading; at work, verify such claims with a test before trusting them, because real SDKs are not always this honest.

**The guard placement decision.** The guard is constructed *after* the success check, not before the call. Both placements can be made correct, but they encode different reasoning: guard-after-success relies on the "nothing allocated on failure" contract; guard-before-call relies on dispose-tolerates-empty plus zero-initialization. The reference chooses guard-after-success because it depends on the *documented* contract rather than on incidental tolerance. Being able to articulate which contract your cleanup depends on is exactly the skill real SDK payload-handling requires.

**Zero-initialization is load-bearing.** `ThingData data = {};` makes `values` null before any SDK call. Skip it and the struct holds stack garbage; on the `ErrNoData` path nothing was written, and any later dispose call would `delete[]` a garbage pointer — undefined behavior with no ASan warning until it detonates. The Chapter 2 idiom (`= {}` on every API struct) is not style; it is the difference between "skip path is safe" and "skip path is a time bomb."

**Why the guard is non-copyable and non-movable.** It aliases one struct for one scope. A copy would mean two guards disposing the same payload — double-dispose (the FileHandle argument from Chapter 1). Movability has no use case at this scope and would complicate the invariant. Deleting both is not a limitation; it is the design stated in code.

**The empty-project scenario is a specification question, not a coding one.** Zero Things means `NoErr`, total 0, skipped 0 — the loop simply never runs. The exercise includes it because real plug-ins constantly meet empty selections and empty documents, and "what does success mean on empty input" is a question to settle *before* writing the loop, not after a bug report.

---

## Chapter 18 — Exercise: The Device SDK

*Trains: Chapter 1 (RAII), Chapter 6 (move-only types — with a twist), Chapter 10 (lambdas/std::function), Chapter 16 Shape 2. Time: ~2 h. This is the peripheral-SDK idiom: after this exercise, libusb, PortAudio, HIDAPI, and serial-port APIs will all look familiar.*

### The vendor code

`FakeDevice.h` / `FakeDevice.cpp` — vendor code, do not edit. Three idioms live in this header, each worth reading twice:

```cpp
// ============================================================================
// FakeDevice.h - a miniature peripheral-device SDK in the classic C idiom:
// opaque handles, open/close lifecycle, and callbacks with a void* context.
// This is the shape of libusb, HIDAPI, PortAudio, serial-port and most
// vendor device SDKs. DO NOT MODIFY. Read it, wrap it, obey it.
// ============================================================================
#pragma once
#include <cstddef>

using DevErr = int;
constexpr DevErr DevOk        = 0;
constexpr DevErr DevNullParam = 1;
constexpr DevErr DevNotFound  = 2;   // no device with that name
constexpr DevErr DevClosed    = 3;   // operation on a closed/invalid handle
constexpr DevErr DevBusy      = 4;   // open() on an already-open device

// Opaque handle: you get a pointer to a type you cannot see inside.
// The SDK owns the memory behind it; you own the OBLIGATION to Close it.
struct DeviceImpl;
using DeviceHandle = DeviceImpl*;

// The C callback idiom: a plain function pointer plus a caller-supplied
// context pointer, passed back verbatim on every invocation. This pair is
// how C APIs deliver events into YOUR code - no closures exist in C.
using SampleCallback = void(*)(int sample, void* userContext);

// Open a device by name ("sensor0".."sensor3" exist). On success writes a
// handle you MUST eventually pass to Device_Close exactly once.
DevErr Device_Open(const char* name, DeviceHandle* outHandle);

// Close and invalidate the handle. Safe to call with null (*no-op*).
// Double-close of the same handle is an error your wrapper must prevent.
DevErr Device_Close(DeviceHandle h);

// Register (or clear, with nullptr) the sample callback for this device.
// The context pointer is stored verbatim and handed back on every sample.
DevErr Device_SetCallback(DeviceHandle h, SampleCallback cb, void* userContext);

// Ask the device to deliver its pending samples NOW, synchronously, by
// invoking the registered callback once per sample on THIS thread.
// (Real SDKs often call back from a driver thread - see the chapter notes.)
DevErr Device_Poll(DeviceHandle h);

// Test-support: number of handles currently open. Must be 0 when you finish.
size_t FakeDevice_OpenHandles();
// Test-support: preload N pending samples (values 100, 101, ...) on a device.
DevErr FakeDevice_InjectSamples(DeviceHandle h, size_t n);
```

**The opaque handle** — `DeviceHandle` is a pointer to a struct whose definition you never see. You cannot copy the device, inspect it, or free it yourself; the handle is a claim ticket, and `Device_Close` is the only way to redeem it. **The open/close lifecycle** — open hands out the obligation; double-close is an *error*, not a no-op, so your wrapper must guarantee exactly-once. **The C callback pair** — a plain function pointer plus a `void*` context returned to you verbatim: this is how C delivers events into your code, because C has no closures. Bridging it to C++ closures is the heart of the exercise.

### The task

**Part A — `DeviceSession`**: a **move-only** RAII wrapper. Unlike Chapter 17's guard (one struct, one scope, copy and move both deleted), a device session is an ownable resource you may want to store in containers or return from factories — so it gets the full Chapter 6 treatment: deleted copies, real moves, `noexcept`, exactly-once close. Opening can fail, and constructors can't return error codes — design around that (the reference uses a static factory writing into an out-parameter, the SDK's own style; returning `std::optional<DeviceSession>` is an equally defensible alternative).

**Part B — the trampoline**: an `OnSample(std::function<void(int)>)` method letting callers register a real C++ closure, bridged to the SDK's C callback via a static function and the `void*` context.

**Part C — prove it**: open, register a lambda capturing a local vector, inject and poll, assert the exact samples arrived; **move the session and verify callbacks still land** (this is the twist — predict what breaks before testing); exercise the error paths (`DevBusy`, `DevNotFound`); and assert `FakeDevice_OpenHandles() == 0` at the end.

### Reference solution

<details>
<summary><strong>Show the solution — do the exercise cold first</strong></summary>

```cpp
// Device SDK exercise - reference solution.
#include "FakeDevice.h"
#include <cassert>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

// Part A - RAII session. MOVE-ONLY: unlike ThingDataGuard (one struct, one
// scope), a device session is an ownable resource you may want to store in
// containers or return from factories - so it gets the full move treatment.
class DeviceSession {
public:
    DeviceSession() = default;                       // empty session

    static DevErr Open(const char* name, DeviceSession& out) {
        DeviceHandle h = nullptr;
        DevErr err = Device_Open(name, &h);
        if (err != DevOk) return err;
        out = DeviceSession(h);                      // move-assign into caller
        return DevOk;
    }

    ~DeviceSession() { Device_Close(h_); }           // null-safe by contract

    DeviceSession(const DeviceSession&) = delete;    // copying a handle would
    DeviceSession& operator=(const DeviceSession&) = delete;   // double-close

    DeviceSession(DeviceSession&& o) noexcept
        : h_(std::exchange(o.h_, nullptr)),
          onSample_(std::move(o.onSample_)) {
        Rebind();                                    // ctx points at *this* -
    }                                                // it moved, so re-register!

    DeviceSession& operator=(DeviceSession&& o) noexcept {
        if (this != &o) {
            Device_Close(h_);
            h_ = std::exchange(o.h_, nullptr);
            onSample_ = std::move(o.onSample_);
            Rebind();
        }
        return *this;
    }

    bool IsOpen() const { return h_ != nullptr; }

    // Part B - the trampoline: bridge the C callback to std::function.
    DevErr OnSample(std::function<void(int)> fn) {
        onSample_ = std::move(fn);
        return Rebind();
    }

    DevErr Poll() { return h_ ? Device_Poll(h_) : DevClosed; }
    DevErr Inject(size_t n) { return h_ ? FakeDevice_InjectSamples(h_, n) : DevClosed; }

private:
    explicit DeviceSession(DeviceHandle h) : h_(h) {}

    static void Trampoline(int sample, void* ctx) {  // the C-shaped landing pad
        auto* self = static_cast<DeviceSession*>(ctx);
        if (self->onSample_) self->onSample_(sample);
    }

    DevErr Rebind() {
        if (!h_) return DevOk;
        return onSample_
            ? Device_SetCallback(h_, &Trampoline, this)
            : Device_SetCallback(h_, nullptr, nullptr);
    }

    DeviceHandle h_ = nullptr;
    std::function<void(int)> onSample_;
};

int main() {
    {
        DeviceSession s;
        DevErr err = DeviceSession::Open("sensor0", s);
        assert(err == DevOk && s.IsOpen());
        assert(FakeDevice_OpenHandles() == 1);

        std::vector<int> got;
        s.OnSample([&got](int v) { got.push_back(v); });   // a real closure,
        s.Inject(3);                                       // riding a C API
        s.Poll();
        assert((got == std::vector<int>{100, 101, 102}));
        std::cout << "callbacks ok: got " << got.size() << " samples\n";

        // move the session - the trampoline context must follow it
        DeviceSession s2 = std::move(s);
        assert(!s.IsOpen() && s2.IsOpen());
        s2.Inject(1);
        s2.Poll();
        assert(got.size() == 4);                    // still lands in 'got'
        std::cout << "moved session still delivers: " << got.back() << "\n";

        // error paths
        DeviceSession dup;
        assert(DeviceSession::Open("sensor0", dup) == DevBusy);   // already open
        DeviceSession nope;
        assert(DeviceSession::Open("sensor9", nope) == DevNotFound);
    }   // s2's destructor closes; s's destructor closes nothing (null handle)

    assert(FakeDevice_OpenHandles() == 0);          // the leak check
    std::cout << "all handles closed\n";
    return 0;
}
```

</details>

### The pitfalls, and what they generalize to

**The trampoline pattern is the whole chapter.** A C API can store only a function pointer — no captures, no state. The trick: register a *static* function whose only job is to cast the `void*` back to your object and forward the call. The context pointer is the closure's state, threaded through the C API by hand. Every callback-based C SDK — every one — is wrapped this way; write it once here and you will recognize it forever.

**The move twist: the context pointer aliases `this`.** The SDK stores the address of your session object as the callback context. Move the session, and the SDK still holds the *old* address — the moved-from husk. The next poll delivers a sample into a gutted object: at best a silent miss, at worst use-after-free when the husk is destroyed first. The reference's `Rebind()` in both move operations re-registers with the new `this`. The general lesson is bigger than this exercise: **any type that hands out pointers to itself (to an SDK, a callback registry, an observer list) must re-register on move — or delete its moves.** `std::function` members, timers, and observer patterns all carry this trap.

**Callback lifetime is a contract with the SDK.** The destructor closes the device, which (per the header) clears the callback — so the SDK can never call into a dead object *in this synchronous design*. Real device SDKs call back from driver threads, which adds two requirements the exercise deliberately excludes: unregister-then-join semantics in the destructor (ensure no callback is mid-flight when the object dies) and synchronization around everything the callback touches. When you meet a real SDK, ask its docs the Chapter 16 question: *what thread calls me back?* — and treat a missing answer as "a thread that isn't yours."

**Exceptions must not escape the trampoline.** The stack above the trampoline is C code (and in real SDKs, a driver). A throwing C++ callback unwinding into C is undefined behavior. Production trampolines wrap the forward in `try/catch(...)` and convert to a stored error or a log — the Chapter 8 boundary rule in its sharpest form. (The reference omits the guard for clarity; adding it is a worthy stretch goal.)

**Double-close prevention is the wrapper's reason to exist.** The SDK punishes double-close with an error; the wrapper makes it structurally impossible — `std::exchange` nulls the handle on move, the destructor tolerates null, and there is no public `Close` to call twice (add one as a stretch goal, and make it idempotent).

### Stretch goals

Add the `try/catch(...)` guard to the trampoline with a `LastError()` accessor. Add an idempotent public `Close()`. Store several sessions in a `std::vector<DeviceSession>` and verify callbacks survive the vector's reallocation (they will — because your move operations rebind; remove `Rebind()` and watch them silently die, then explain the mechanism). Hardest: simulate the threaded case — call `Device_Poll` from a `std::thread` and make the sample collection race-free with a mutex, then explain why the destructor now needs more than it has.

---

## Chapter 19 — Exercise: The Word Counter

*Trains: Chapter 10 (auto, lambdas, structured bindings), Chapter 11 (containers, algorithms, the map/vector dance). Time: ~60 min. Rule: cppreference only — this is the docs-navigation drill.*

### The task

Read a text file; count word frequencies case-insensitively, stripping punctuation from word edges; print the top 10 words that occur at least twice, sorted by count descending, ties broken alphabetically. That one sentence forces: file I/O, `unordered_map` counting, the map-to-vector transfer, a two-key sort lambda, filtering, and bounded output.

### Reference solution

<details>
<summary><strong>Show the solution — do the exercise cold first</strong></summary>

```cpp
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) { std::cerr << "usage: words <file>\n"; return 1; }

    std::ifstream in(argv[1]);                    // RAII: closes itself
    if (!in) { std::cerr << "cannot open " << argv[1] << "\n"; return 1; }

    std::unordered_map<std::string, int> freq;    // Dictionary<string,int>
    std::string word;
    while (in >> word) {
        // normalize: lowercase, strip non-alpha edges
        std::transform(word.begin(), word.end(), word.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        while (!word.empty() && !std::isalpha(static_cast<unsigned char>(word.front())))
            word.erase(word.begin());
        while (!word.empty() && !std::isalpha(static_cast<unsigned char>(word.back())))
            word.pop_back();
        if (!word.empty())
            ++freq[word];              // operator[] inserting IS wanted here
    }

    // map -> vector of pairs, because maps can't be sorted by value
    std::vector<std::pair<std::string, int>> ranked(freq.begin(), freq.end());
    std::sort(ranked.begin(), ranked.end(),
              [](const auto& a, const auto& b) {
                  if (a.second != b.second) return a.second > b.second;
                  return a.first < b.first;        // tie-break: alphabetical
              });

    std::erase_if(ranked, [](const auto& p) { return p.second < 2; });  // C++20
    // pre-C++20: ranked.erase(std::remove_if(...), ranked.end());

    const size_t top = std::min<size_t>(10, ranked.size());
    for (size_t i = 0; i < top; ++i)
        std::cout << ranked[i].first << ": " << ranked[i].second << "\n";
    return 0;
}
```

</details>

### What each part is really teaching

**`++freq[word]` — the one place the map's insert-on-read trap is a feature.** Chapter 11 warns that `operator[]` silently inserts a default value on a missing key. Counting is the idiom where that behavior is exactly what you want: first sight of a word inserts 0, then increments to 1. Knowing when a trap is a tool is the difference between rule-following and fluency.

**The map-to-vector dance.** You cannot sort a map by value — its ordering *is* its identity (`std::map` by key; `unordered_map` by nothing). The universal pattern: copy into a `vector<pair<K,V>>`, sort that. The range constructor `ranked(freq.begin(), freq.end())` does the transfer in one line.

**The two-key comparator.** `if (a.second != b.second) return a.second > b.second; return a.first < b.first;` — descending by count, ascending by word. A comparator must be a *strict weak ordering*; the classic bug is `return a.second >= b.second` (note `>=`), which violates it and produces undefined behavior inside `std::sort` — sometimes a crash, sometimes silently wrong order. If a sort ever crashes deep inside the standard library, audit the comparator first.

**`unsigned char` in the `tolower`/`isalpha` calls.** The `<cctype>` functions take an `int` that must be representable as `unsigned char`; passing a plain `char` that happens to be negative (any non-ASCII byte on signed-char platforms) is undefined behavior. The cast is not pedantry — it is the difference between working and UB the moment the input contains a name like "Müller". (The honest limitation: this solution treats bytes, not Unicode; real text pipelines need proper Unicode handling — Chapter 9's encoding discussion.)

**`std::erase_if` vs the erase-remove idiom.** The C++20 one-liner and its two-step ancestor from Chapter 11; the solution shows the modern form and names the classic in a comment because legacy codebases are full of it.

### Stretch goals

Rewrite the pipeline with C++20 ranges (`views::filter` + `views::take`); time the difference between `map` and `unordered_map` on a large file (then explain it via Chapter 11's cache-locality argument); make the minimum count and top-N command-line arguments with proper validation.

---

## Chapter 20 — Exercise: Slicing and Polymorphism

*Trains: Chapter 2 (slicing), Chapter 5 (virtual dispatch, virtual destructors). Time: ~45 min.*

### The task

Build a small `Shape` hierarchy (`Circle`, `Rect`) with a virtual `Area()`. Then, deliberately, do it wrong first: put shapes into a `std::vector<Shape>` and total the areas. Predict what happens *before* compiling. Then fix the design so polymorphism actually works, and prove with output that the right `Area()` runs for each element. Finally: remove `virtual` from the base destructor and explain (then demonstrate under ASan with a heap-allocated derived object owning memory) what breaks.

### The broken version, and what it teaches

```cpp
std::vector<Shape> shapes;          // a vector of BASE OBJECTS
shapes.push_back(Circle(1.0));      // sliced: only the Shape part is stored
```

Two outcomes depending on the base: if `Shape` is abstract (pure virtual `Area`), this **does not compile** — the container cannot hold instances of an abstract class, and the error message is your first taste of template error novels (Chapter 7). If `Shape` is concrete, it compiles and silently stores amputated objects: `radius` is gone, and `shapes[0].Area()` calls `Shape::Area`. Make the base abstract *first* so the compiler catches the design error — that is itself a design lesson: **pure virtual functions turn slicing from a runtime surprise into a compile error.**

### Reference solution (the fixed design)

<details>
<summary><strong>Show the solution — do the exercise cold first</strong></summary>

```cpp
// Slicing & polymorphism lab - reference solution (the FIXED version).
#include <iostream>
#include <memory>
#include <vector>

class Shape {
public:
    virtual ~Shape() = default;                    // polymorphic base: MUST
    virtual double Area() const = 0;               // pure virtual = abstract
    virtual const char* Name() const = 0;
};

class Circle final : public Shape {
public:
    explicit Circle(double r) : r_(r) {}
    double Area() const override { return 3.14159265 * r_ * r_; }
    const char* Name() const override { return "Circle"; }
private:
    double r_;
};

class Rect final : public Shape {
public:
    Rect(double w, double h) : w_(w), h_(h) {}
    double Area() const override { return w_ * h_; }
    const char* Name() const override { return "Rect"; }
private:
    double w_, h_;
};

int main() {
    // The polymorphic container: unique_ptr<Base>, never Base by value.
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Circle>(1.0));
    shapes.push_back(std::make_unique<Rect>(2.0, 3.0));

    double total = 0;
    for (const auto& s : shapes) {                 // const auto&: no copy,
        std::cout << s->Name() << " area=" << s->Area() << "\n";
        total += s->Area();                        // virtual dispatch works
    }
    std::cout << "total=" << total << "\n";
    return 0;
}   // unique_ptrs delete through Shape* - virtual ~Shape makes it legal
```

</details>

### The details that carry the weight

**`vector<unique_ptr<Shape>>` is *the* polymorphic container.** Objects live on the heap behind pointers; the vector holds owners. Copying the vector is deleted (unique_ptr), which is honest: a hierarchy of polymorphic objects has no cheap, obvious copy semantics — if you need copying, you design a virtual `Clone()` method, explicitly.

**`virtual ~Shape() = default;` is load-bearing, not boilerplate.** Each `unique_ptr<Shape>` deletes through a `Shape*`. Without the virtual destructor that is undefined behavior (Chapter 5): derived destructors never run, derived members leak. The demonstration to run: give `Circle` a `std::vector<double>` member, remove `virtual` from `~Shape`, and ASan's leak report at exit points at the vector's allocation — a leak with one stack, exactly the Finding 10 report shape.

**`final` on the leaves** documents that the hierarchy ends here and lets the compiler devirtualize calls where it can prove the type.

**`const auto&` in the loop** — copying a `unique_ptr` doesn't compile (deleted), so the wrong loop here fails loudly rather than silently. Notice how the ownership design converts Chapter 2's silent copy trap into a compile error: good types make wrong code not compile.

---

## Chapter 21 — Exercise: Iterator Invalidation

*Trains: Chapter 11 (the trap section), Chapter 3 (UB). Time: ~45 min. Format: three broken loops to fix — write predictions, run broken versions under ASan, then fix.*

### The three tasks

1. Remove all odd numbers from a `vector<int>` while iterating.
2. Append a copy of every element to the same vector while iterating it.
3. Hold a reference to `v[0]`, `push_back` once, then use the reference.

Write each the naive way first, predict the failure mode, run under ASan, and only then fix.

### Reference solutions and the reasoning

<details>
<summary><strong>Show the solutions — do the tasks cold first</strong></summary>

```cpp
// Iterator invalidation lab - the FIXED patterns (broken ones live in comments).
#include <iostream>
#include <numeric>
#include <vector>

int main() {
    std::vector<int> v(10);
    std::iota(v.begin(), v.end(), 0);              // 0..9

    // Task 1: remove odd numbers while iterating.
    // BROKEN: for (auto it=v.begin(); it!=v.end(); ++it) if (*it%2) v.erase(it);
    for (auto it = v.begin(); it != v.end(); ) {
        if (*it % 2) it = v.erase(it);             // erase returns next valid
        else         ++it;
    }
    // or simply: std::erase_if(v, [](int x){ return x % 2; });

    // Task 2: append while iterating - by INDEX against a captured size,
    // because push_back may reallocate and kill every iterator/reference.
    // BROKEN: for (int x : v) v.push_back(x);     // UB on reallocation
    const size_t n = v.size();
    v.reserve(v.size() * 2);                       // belt AND suspenders
    for (size_t i = 0; i < n; ++i) v.push_back(v[i]);

    // Task 3: the reference that dies. BROKEN version:
    //   int& first = v[0]; v.push_back(99); std::cout << first;  // maybe UB
    // FIXED: re-acquire after any potentially-reallocating call, or reserve.

    for (int x : v) std::cout << x << ' ';
    std::cout << "\n";
    return 0;
}
```

**Task 1** — `v.erase(it)` invalidates `it`; the loop's `++it` then increments a dead iterator. ASan typically reports `heap-use-after-free` *if* the erase triggered internal movement, but may also stay silent while producing wrong results — the Finding 10 lesson again: predict values, don't rely on the crash. The fix is the erase-returns-next idiom, or `std::erase_if` which encapsulates it.

**Task 2** — the subtle one, because it *sometimes works*. `push_back` invalidates everything only when capacity is exceeded; a run that happens to stay within capacity behaves, then production data grows and it detonates. The fix has two independent parts: iterate by **index** against a **pre-captured size** (indices survive reallocation; the captured size stops the loop from chasing its own tail), and `reserve` up front (which makes reallocation impossible *and* documents intent). Either alone suffices; both together read as deliberate.

**Task 3** — references into a vector are exactly as fragile as iterators. `int& first = v[0];` is a pointer in disguise; after a reallocating `push_back` it dangles. ASan reports `heap-use-after-free` with the free stack inside vector's internals — your first encounter with a report whose "freed by" stack contains no code of yours. Reading it correctly ("the *container* freed the old block during growth") is the skill; the fix is re-acquiring after mutation, or reserving.

**The rule to leave with:** after any potentially-reallocating operation, treat every iterator, pointer, and reference into that vector as dead. The invalidation table in Chapter 11 says which containers are gentler — and why `map`'s stable iterators are sometimes worth its slower lookups.

</details>

---

## Chapter 22 — Exercise: Lambda Lifetimes

*Trains: Chapter 10 (captures), Chapter 1 (ownership), Chapter 3 (dangling). Time: ~45 min. The C# contrast is the whole point: C# closures keep captured objects alive via the GC; C++ captures do exactly what you wrote, including dangling.*

### The three tasks

1. `MakeCounter()`: return a lambda that returns 1, 2, 3... on successive calls. The naive `[&count]` version compiles and dangles — demonstrate, then fix.
2. A `Button` struct storing a `std::function<void()>` callback that prints a label — where the label is created in a narrower scope than the click. Make it correct *by ownership design*, not by luck.
3. Capture-by-move: a lambda that *owns* a `unique_ptr` (something copies can't do).

### Reference solutions

<details>
<summary><strong>Show the solutions — do the tasks cold first</strong></summary>

```cpp
// Lambda lifetime lab - fixed patterns.
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// A "button" storing a callback - the classic escape route for lambdas.
struct Button {
    std::function<void()> onClick;
    void Click() const { if (onClick) onClick(); }
};

std::function<int()> MakeCounter() {
    int count = 0;
    // BROKEN: return [&count]{ return ++count; };   // dangling: count dies
    return [count]() mutable { return ++count; };    // FIXED: own a copy
}   // 'mutable' because captures are const by default inside the lambda

int main() {
    Button b;
    {
        auto label = std::make_shared<std::string>("Save");
        // BROKEN: b.onClick = [&label]{ ... };      // label dies with scope
        b.onClick = [label] { std::cout << "clicked: " << *label << "\n"; };
    }                       // shared_ptr copy keeps the string alive - by design
    b.Click();

    auto counter = MakeCounter();
    std::cout << counter() << counter() << counter() << "\n";   // 123

    // capture-by-move for expensive/unique things (C++14 init-capture):
    auto big = std::make_unique<std::vector<int>>(1000, 7);
    auto owner = [v = std::move(big)] { return v->size(); };
    std::cout << "owned size=" << owner() << "\n";
    return 0;
}
```

</details>

### What each fix teaches

**Task 1 — `[count]() mutable`.** Two lessons in five characters. Capture by copy gives the lambda its *own* `count` that lives as long as the lambda does — the closure now owns its state, which is what the C# version was secretly doing via the heap. And `mutable`: captured copies are `const` inside the lambda by default; incrementing one requires opting out. C# has no equivalent because its captures are references to heap cells. Run the broken `[&count]` version too: it may even print plausible numbers (stack memory not yet reused) — dangling that *works in the demo* is the most dangerous kind, and ASan with `-fsanitize=address` flags the stack-use-after-return only with `ASAN_OPTIONS=detect_stack_use_after_return=1` — worth noting, since it is off by default on some toolchains.

**Task 2 — shared ownership as the design, not a workaround.** The callback outlives the scope that created the label, so *someone* must own the label beyond that scope. Capturing the `shared_ptr` by copy makes the lambda a co-owner: the string lives exactly as long as anything that needs it — which is the C# object-lifetime model, opted into deliberately and visibly. The broken `[&label]` version is the single most common real-world lambda bug: callbacks, timers, and async handlers capturing locals by reference.

**Task 3 — init-capture (`[v = std::move(big)]`).** Some things cannot be copied into a closure — `unique_ptr` foremost. C++14 init-captures move them in; the lambda becomes the owner, and consequently the lambda itself is now move-only (storing it requires `std::function` alternatives like `std::move_only_function` in C++23, or just `auto`). This is the pattern for handing expensive or unique resources to deferred work.

**The rule to leave with,** verbatim from Chapter 10: capture by reference only when the lambda cannot outlive the scope; by copy or move when it escapes — stored, returned, or run async. "Escapes" is the operative test: `std::function` members, callback registries, and thread launches are all escape routes.

---

## Chapter 23 — Exercise: The Build-Model Lab

*Trains: Chapter 12, hands-on. Time: ~45 min. No reference solution file — the artifact is your notes on what each error looks like, because reading build errors is the skill.*

### The setup

Split a trivial `Greeter` class across `Greeter.h` / `Greeter.cpp` with a `main.cpp` consumer. Get it building. Then break it seven ways, one at a time, and for each: predict the error *stage* (preprocessor / compile / link), provoke it, and paste the first error line into your notes with a one-line translation.

### The seven breakages

1. **Delete the `#include "Greeter.h"` from main.cpp** → compile error, `undeclared identifier`. The translation unit never saw the declaration.
2. **Delete Greeter.cpp from the build command** (compile main.cpp alone) → **linker** error, `undefined reference` / `LNK2019 unresolved external`. Everything compiled; the body is missing at link time. Learn to tell this apart from #1 at a glance — it is the single most practical build skill.
3. **Declare a method in the header, never define it anywhere,** and call it → same linker error as #2. Same symptom, different cause; the error text is identical, which is exactly why the *cause list* for unresolved externals belongs in your notes: missing .cpp in build, missing library, declared-never-defined, template body in a .cpp (Chapter 7).
4. **Remove the include guard** (`#pragma once`) and include the header twice via a second header → compile error, `redefinition of 'class Greeter'`. #include is paste; the guard is what makes double-paste harmless.
5. **Define a free function in the header** (outside the class, no `inline`), include it from two .cpp files → **linker** error, `multiple definition` / `LNK2005`. The One Definition Rule enforced. Fix three ways and note the difference: `inline`, move the body to a .cpp, or make it a class member defined in-class (implicitly inline).
6. **Create a circular include** (A.h includes B.h includes A.h, guards present) and use B's type in A → confusing compile errors about incomplete types. Fix with a forward declaration in one of the headers — and note which usages permit forward declaration (pointers, references) and which demand the full definition (members by value, inheritance).
7. **Change a class definition in the header, rebuild only main.cpp** (simulating a stale object file: compile Greeter.cpp, *then* edit the header, then compile only main.cpp and link both) → it links and misbehaves or crashes: an **ODR violation across translation units**, undetectable by the linker. This is why build systems track header dependencies and why "clean build fixes it" is a real phenomenon with a real cause — the moment you understand this breakage, incremental-build weirdness stops being mysterious.

### Why this lab earns its place

Half of all confusing C++ errors are build-model errors (Chapter 12). At work, against a vendor SDK with heavy headers and multi-project solutions, error-stage triage is the first move of every debugging session: *which tool complained — preprocessor, compiler, or linker — and therefore which file do I open?* After this lab, that triage takes five seconds.

---

## Chapter 24 — Practice Plan

A one-week hands-on plan (compress or stretch as needed). Each day's exercise now has a full worked chapter: Day 1 → Chapter 14 (Tracer), Day 2 → Chapter 15 (Buffer), Day 3 → Chapter 19 (Word Counter), plus the SDK track — Chapter 16 (the Bestiary, read first), Chapter 17 (FakeSDK) and Chapter 18 (Device SDK) — and the standalone labs: Chapter 20 (Slicing), Chapter 21 (Invalidation), Chapter 22 (Lambdas), Chapter 23 (Build Model). Do exercises cold first; read the chapter's solution and pitfalls after. The rule that makes it work: **do the exercises cold** — compiler, debugger, sanitizer, and offline docs as your only feedback loops. That trains the self-sufficiency the job requires.

### The week

- **Day 0 — Setup.** IDE with /W4 (or -Wall -Wextra) and C++17. Offline cppreference archive installed. If you are targeting a specific host application or device, download its SDK now. Create notes.md — your permanent gotcha file; first entry: today's setup steps.
- **Day 1 — Hands.** From scratch, no lookups until stuck: a class printing from ctor/dtor (watch RAII fire under the debugger); pass it by value / by ref / by move and predict output before running. Then the FileHandle RAII wrapper from memory.
- **Day 2 — Buffer + sanitizer.** Rule of Five Buffer cold, from memory. Then break it deliberately three ways (remove the null-out in move; non-virtual base dtor; erase-during-iteration) and let AddressSanitizer catch each one. Read its reports until they make sense. Notes: what each ASan report looks like.
- **Day 3 — STL fluency.** Word-frequency exercise: file → vector → unordered_map counts → sort by count with a lambda → erase_if filter → top 10. cppreference-only rule in force. Then step through it in the debugger inspecting containers.
- **Day 4 — Your real SDK (or the FakeSDK/FakeDevice labs).** If you have a target SDK: build its example plug-ins, load one into the host, and — the key skill — attach the debugger to the host process and hit a breakpoint inside your code. Expect friction; friction is the curriculum. If not, do Chapters 17 and 18 back to back. Notes: exact attach steps, build config gotchas.
- **Day 5 — First real build (docs + examples allowed freely).** Against your target SDK: an aggregator in the FakeSDK shape — iterate the SDK's elements/devices, check every error code, present a computed result. Use SDK examples as templates — that IS the legitimate at-work workflow.
- **Day 6 — Second build (cold).** Delete yesterday's code. Rebuild using only docs, examples, and your notes. The gap between the two builds tells you exactly what to add to the notes file. If fast: extend with an owned-payload read behind your RAII guard, or whatever transactional/undo mechanism your SDK offers.
- **Day 7 — Consolidate, then stop.** Re-read Appendix B and your notes; rewrite the Buffer one last time from memory; tidy notes into sections (C++ gotchas / API recipes / toolchain steps). Rest. Walk in curious, not depleted.

### Standing drills (repeatable any time)

- **Bug hunt:** write each classic bug on purpose, watch it misbehave, fix it: missing virtual destructor; auto-without-& loop; dangling lambda capture; map operator[] silent insert; erase during iteration; use-after-move; shallow-copy double-free; double-close of an SDK handle.
- **Predict-then-run:** before every run, say out loud what will print and where each object lives (stack/heap) and dies. Wrong predictions go in the notes file.
- **Cold rewrites:** FileHandle, ThingDataGuard, and the Rule of Five Buffer — until each takes under ten minutes without references.
- **Narrate:** explain your reasoning out loud while coding. You'll be explaining decisions to colleagues; the habit transfers directly to code reviews and pair sessions.

---

---

## Chapter 25 — Findings from Practice: a Living Log

*A living log: weak spots discovered during hands-on exercises, each with the theory behind it, the broken and fixed code side by side, and the habit to build. New findings get appended as practice continues — this chapter is meant to grow.*

### Finding 1 — Copy-shaped moves: a move that doesn't steal is a copy with a misleading name

**Found in:** the Tracer exercise — move operations written as `name = "moved from " + t.name;`.

**The theory.** To understand moving, look at what a `std::string` physically is: roughly three fields — a pointer to heap-allocated characters, a size, and a capacity. "Moving a string" means something concrete about those fields:

```cpp
name = std::move(t.name);
// name's pointer  = t.name's pointer    <- steal the heap block
// t.name's pointer = nullptr (empty)    <- null out the source
```

Three pointer-sized assignments. No allocation, no character copying. That O(1) theft — versus O(n) duplication — is the *entire reason move semantics exist*.

**The broken version, dissected:**

```cpp
Tracer(Tracer&& t) noexcept {
    name = "moved from " + t.name;   // looks like a move, is a copy
}
```

Step by step, `"moved from " + t.name`: (1) **allocates a brand-new heap block**, (2) copies the literal into it, (3) **copies every character of t.name** into it, (4) assigns the temporary to `name`. Meanwhile `t.name` is untouched — still owning its original block, fully intact.

Compare with the definition of copying: allocate, duplicate characters, leave the source whole. **Identical.** This is a copy constructor that prints the word "move".

**Why it matters beyond pedantry.** Scale the member up: imagine the class held a vector of a million points. This pattern duplicates a million points on every vector reallocation and every `std::move` into a container — while *claiming* to be the cheap option. And the lie compounds through `noexcept` (see Finding 3): the allocation in step (1) **can throw**, so the noexcept promise is false.

**The fix — steal in the initializer list, print after:**

```cpp
Tracer(Tracer&& t) noexcept : name(std::move(t.name)) {
    std::cout << name << " move constructor\n";   // name, not t.name -
    t.name = "(husk)";                            // we already stole it!
}

Tracer& operator=(Tracer&& t) noexcept {
    std::cout << t.name << " move assignment\n";
    name = std::move(t.name);    // steal (string drops our old block itself)
    t.name = "(husk)";
    return *this;
}
```

Two details visible only in tracing code: in the move *constructor*, by the time the body runs, `t.name` is already emptied — so print your own `name`, which now holds the stolen value. And `t.name = "(husk)"` is tracing sugar only — it re-fills the source so destructor output shows which objects were gutted; real code leaves moved-from strings empty (strictly, that assignment allocates, slightly compromising noexcept purity — fine in a learning tracer, not in production).

**Habit:** a move operation's body should contain `std::move(member)` for every resource-holding member — and nothing that allocates. If you can't write it that way, question whether the operation is really a move.

### Finding 2 — Member initializer list vs assignment in the body — construction happens before the brace

**Found in:** the same Tracer — all four copy/move operations assigned `name` inside the body.

**The theory.** Members are **constructed before the constructor body runs** (Chapter 4). So this:

```cpp
Tracer(const Tracer& t) {          // <- name ALREADY exists here, empty
    name = "copy of " + t.name;    // step 2: assign over it
}
```

is a two-step dance: default-construct `name` as an empty string, then assign. The initializer-list form constructs it right the first time:

```cpp
Tracer(const Tracer& t) : name("copy of " + t.name) {   // one step
    std::cout << ...;
}
```

**Why build the reflex on easy cases:** for a string the waste is trivial — but for a `const` member or a reference member, the body-assignment version *does not compile at all* (they can only be initialized, never assigned). If the initializer list is already your default, those cases cost you nothing; if body-assignment is your default, they cost you a confused half-hour.

**Habit:** the constructor body is for logic (logging, validation, registration). Member *values* belong in the initializer list — and remember they initialize in declaration order, not list order.

### Finding 3 — noexcept is a promise, not a decoration

**Found in:** adding `noexcept` to a move that internally allocates.

**The theory.** `noexcept` tells callers — and especially `std::vector` — "this operation cannot throw." Vector uses it to choose its reallocation strategy: if your element's move constructor is noexcept, it *moves* elements to the new block; if not, it *copies* them, so that a mid-transfer exception can't leave the container half-destroyed. That is the behavior difference observed live in the Tracer output: `moved from v1 copy constructor` on reallocation before the keyword, a move after it.

**The trap discovered here is the reverse direction:** claiming noexcept on an operation that can throw. String concatenation allocates; allocation can throw `std::bad_alloc`. If an exception ever escapes a noexcept function, the program does not unwind — it calls **`std::terminate` on the spot**. So a false noexcept converts a recoverable out-of-memory into an instant process death, in the rare moment you least want it.

**The rule that makes both directions safe:** noexcept belongs on operations that genuinely just shuffle pointers — which real moves do (Finding 1). Write the move correctly and the promise is automatically true. The two findings are one finding: *steal, don't build — then noexcept is honest and vector moves your elements.*

**Habit:** `noexcept` on every move constructor and move assignment — and a body that justifies it.

### Finding 4 — Assignment must deal with what you already hold

**Found in:** Tracer's assignment operators — bodies identical to the constructors', which *happened* to be safe.

**The theory.** Construction and assignment differ in one crucial way: at assignment time, **the target already owns something**. `name = ...` was safe in Tracer only because `std::string::operator=` internally releases the old buffer before taking the new value — the string did the dangerous step invisibly.

With a raw resource, nothing is invisible:

```cpp
// Buffer holds: int* data_
Buffer& operator=(const Buffer& other) {
    data_ = new int[other.size_];        // BUG: the block data_ USED to
    ...                                  // point at is now leaked forever
}
```

Correct assignment must release-then-acquire — or better, sidestep the ordering problem entirely with **copy-and-swap** (Chapter 6): copy into a temporary, swap guts with it, let the temporary's destructor free your old resource. That also delivers the strong exception guarantee and self-assignment safety for free.

**Habit:** whenever writing `operator=`, ask first: "what happens to what I'm currently holding?" If the answer is "a member type handles it," fine — but know *which* member and *how*, because the day the member is a raw pointer, nobody handles it but you.

### Finding 5 — The moved-from state: valid but unspecified

**Found in:** the question "what state is `a` in?" after `Tracer c = std::move(a);`.

**The theory.** A moved-from object is **not destroyed and not invalid** — it lives until its scope ends and its destructor runs normally (the Tracer output showed every moved-from object still getting a destructor line). The standard's phrase for its state is *valid but unspecified*: it is a real object satisfying its class invariants, but you must not assume anything about its contents.

What is allowed on a moved-from object: destroy it, assign a new value to it, call operations with no preconditions (`clear()`, `empty()`). What is not: read its value expecting anything in particular.

```cpp
std::string s = "hello";
std::string t = std::move(s);
s.size();          // legal, but the value is unspecified - don't rely on it
s = "reborn";      // perfectly fine - assignment gives it a value again
```

One corner worth knowing: **self-move** (`a = std::move(a)`) must not corrupt the object — which is why the Buffer's move assignment carries `if (this != &other)`. Standard types survive self-move (left valid-but-unspecified); your types should too.

**Habit:** after `std::move(x)`, mentally mark `x` as a husk: assign or destroy, nothing else.

### Finding 6 — Release-before-acquire assignment: exception safety is an ordering problem

**Found in:** the Buffer exercise — copy assignment that deleted the old block, then allocated the new one.

**The theory.** Assignment differs from construction in one way: the target already owns something (Finding 4). The naive order — free mine, then acquire the new — has a hidden failure mode:

```cpp
delete[] data_;              // old block gone; data_ dangles
data_ = new int[size_];      // if THIS throws, the function exits here
```

The object is left holding a dangling pointer. Its destructor later runs during stack unwinding and frees the same block a second time — **an exception turned into heap corruption**. The bug is invisible in every test where allocation succeeds, which is all of them until production.

**The principle:** do all the work that can throw *before* modifying your own state. Two shapes deliver it:

```cpp
// allocate-first (minimal change):
int* fresh = new int[other.size_];                    // may throw - still intact
std::copy(other.data_, other.data_ + other.size_, fresh);
delete[] data_;                                       // point of no return
data_ = fresh;  size_ = other.size_;

// copy-and-swap (structural - can't get the order wrong):
Buffer tmp(other);   // throwing work in the copy ctor; *this untouched
Swap(tmp);           // noexcept pointer exchanges
                     // tmp's dtor frees the old block
```

**The contrast that seals the lesson:** *move* assignment legitimately deletes first — because nothing after its `delete[]` can throw. Same first line, opposite verdict. Exception safety is not about which operations you call; it is about **what state you are in if any given line throws**.

**Habit:** in any assignment operator, find the first line that modifies `*this` and ask: "can anything after this line throw?" If yes, reorder.

### Finding 7 — `new T[n]` does not zero: indeterminate values are UB to read

**Found in:** the Buffer constructor — `data_(new int[size])`.

**The theory.** `new int[size]` default-initializes the elements, and for built-in types default-initialization does *nothing*: the memory holds whatever bytes were there. Reading an element before writing it is undefined behavior of the quiet kind — often prints 0 in Debug (fresh pages from the OS are zeroed), garbage in Release or after heap reuse. The Chapter 3 signature: works on my machine.

```cpp
data_(new int[size])     // indeterminate contents
data_(new int[size]{})   // value-initialized: all zeros. One pair of braces.
```

C# contrast worth noting: `new int[5]` in C# is always zeroed — the runtime guarantees it. C++ makes zeroing opt-in because it costs a memset and C++'s contract is "don't pay for what you don't use."

**Habit:** every `new T[n]` gets `{}` unless a measured reason says otherwise — and in real code, prefer `std::vector` which value-initializes anyway.

### Finding 8 — Accessors: return by reference, and provide the const-overload pair

**Found in:** the Buffer — `int At(size_t) const` returning a copy, making the buffer write-only through its own API.

**The theory.** Returning by value hands out a copy; `buf.At(2) = 7` modifies a temporary and is either a compile error or a silent no-op. Containers hand out **references** to their elements — and because a reference-returning method cannot be `const` alone (it would allow mutation through a const object), the idiom is the pair:

```cpp
int&       At(size_t i)       { assert(i < size_); return data_[i]; }
const int& At(size_t i) const { assert(i < size_); return data_[i]; }
```

Same name; overload resolution picks by the constness of the object: mutable buffer gets the writable reference, `const Buffer&` gets the read-only one. This is precisely how `std::vector::operator[]` is declared — the trailing-const material of Appendix A.5 made practical.

Also decide the bounds contract explicitly: `assert` (documented precondition, free in Release) or throw `std::out_of_range` (the `.at()` convention). Unchecked-and-undocumented is the only wrong answer.

**Habit:** when writing any container-like accessor, write both overloads in one motion — needing the second is the rule, not the exception.

### Finding 9 — Destructors: no null checks, no dead stores

**Found in:** the Buffer destructor — `if (data_) delete[] data_; data_ = nullptr; size_ = 0;`.

**The theory.** Three small misunderstandings in one function. `delete`/`delete[]` on a null pointer is a guaranteed safe no-op — the check is redundant. And assigning to members in a destructor is dead work: the object ceases to exist the instant the destructor returns; no code can legally observe those stores. (Compilers routinely eliminate them, confirming their meaninglessness.)

```cpp
~Buffer() { delete[] data_; }    // complete and correct
```

Beyond style, the busywork signals a mental model worth correcting: nulling members "for safety" in a destructor implies the object might be touched afterward — and if anything *does* touch it afterward, that is use-after-destruction UB which no amount of member-nulling makes safe. The safety comes from ownership discipline (Chapter 1), not from defensive stores.

**Habit:** a resource-owning destructor is one line per resource. If it is longer, ask what the extra lines think they are protecting against.

### Finding 10 — A clean sanitizer run is not a correctness proof

**Found in:** the Buffer sabotage experiments — removing the self-move guard produced "no issues," which was itself the bug.

**The theory.** Walk the guardless move assignment through `c = std::move(c)`:

```cpp
delete[] data_;                              // frees c's block
size_ = std::exchange(other.size_, 0);       // other IS c: size_ becomes 0
data_ = std::exchange(other.data_, nullptr); // reads the dangling ptr,
                                             // immediately overwrites with null
```

Net result: `c` ends up as an empty, *valid-looking* buffer. The destructor later does `delete[] nullptr` — a safe no-op. No invalid access ever occurred, so AddressSanitizer has nothing to say — and the data is silently gone. This is the quieter of the two possible outcomes (the loud one, heap-use-after-free, needs something to actually dereference the dangling pointer, e.g. a guardless self-*copy* doing `std::copy` from the freed block).

**The principle:** sanitizers catch *memory crimes* — invalid reads and writes, double-frees, leaks. They cannot catch *memory-clean but logically wrong* behavior. The two verification tools are complementary and neither substitutes for the other:

- **ASan answers:** "did this program touch memory it shouldn't?"
- **Predictions and assertions answer:** "did this program produce the values it should?"

```cpp
c = std::move(c);
assert(c.Size() == 5 && c.At(2) == 42);   // catches what ASan cannot
```

A related mechanical lesson from the same session: **ASan halts at the first error by default**, and leak detection runs at normal program exit — so a double-free report can mask a leak that would have been reported later. To see subsequent errors, either remove the first crime from the run, or use `ASAN_OPTIONS=halt_on_error=0` to report-and-continue. And note the report-shape asymmetry: a double-free carries three stacks (access, prior free, allocation); a leak carries exactly one — the allocation — because an orphaned block's birth is the only trace it ever leaves.

**Habit:** every experiment and every test states its expected *values*, not just "doesn't crash." The question "what should this print?" outranks "did ASan complain?" — and a run that surprises you by being clean deserves as much scrutiny as one that fails.

### Finding 11 — The Tracer as a permanent diagnostic tool

The exercise that surfaced Findings 1–5 is worth keeping as a reusable instrument: a class that prints from all six special member functions makes the invisible visible. Drop a Tracer into any container, function signature, or algorithm and the output tells you exactly what the compiler chose to do. Findings it demonstrated on first run:

| Observation in output | Concept confirmed |
|---|---|
| `Tracer d = MakeTracer()` printed ONE constructor, no copy, no move | RVO / copy elision — returning by value is free |
| `ByRef(b)` printed *nothing* | `const T&` binds directly: no construction — why it's the default parameter idiom |
| `ByValue(std::move(b))` printed a move, not a copy | std::move selects the move overload; the parameter steals |
| Second `push_back` printed activity for v1 too | reallocation transfers existing elements to the new block |
| That transfer was a COPY until the move ctor was noexcept | vector's exception-safety rule (Finding 3) |
| Every constructor line paired with exactly one destructor line | RAII bookkeeping balances — no leaks, no doubles |
| Destructors ran in reverse construction order per scope | stack unwinding order |

**Habit:** when container or call behavior is mysterious at work, reach for the Tracer before reaching for theory. Ten lines of printing beat an hour of guessing — and it requires no tools a locked-down work machine lacks.

---

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

## Appendix B — Core Principles (Cheat Sheet)

One line each. If you can say these fluently and back them with code, the concept is yours. Good for a quick re-read before code reviews, design discussions — or any morning.

**Ownership / RAII**

- "Every resource has exactly one clear owner — unique_ptr or stack allocation. I never write raw new/delete."
- "unique_ptr by default; shared_ptr only when I can explain why; weak_ptr breaks cycles — there's no GC to collect them."
- "Acquire in the constructor, release in the destructor, delete the copy operations — that's the RAII wrapper shape."

**Value semantics**

- "C++ is value-semantic by default; I opt into reference semantics explicitly."
- "const auto& in loops by reflex — auto alone copies."
- "Polymorphic objects go behind unique_ptr — storing them by value slices them."

**Virtual dispatch**

- "Non-virtual calls dispatch on the static type, virtual on the dynamic type — via the vtable."
- "Every polymorphic base gets virtual ~Base() = default — deleting through a base pointer otherwise is UB."
- "I mark every override 'override' so the compiler catches signature mismatches."

**Templates**

- "Templates are compile-time code generation — a separate instantiation per type, zero runtime cost."
- "Requirements are implicit pre-C++20; concepts made them explicit like C#'s where clauses."
- "Template definitions live in headers because each translation unit must see the source to instantiate."
- "I'd prefer adding a virtual method over dynamic_cast chains."

**Compilation model**

- "Each .cpp compiles independently as a translation unit; the linker resolves symbols across them."
- "Unresolved external = definition missing at link time; undeclared identifier = declaration missing at compile time."
- "I forward-declare in headers and include in .cpp files to keep build times sane."

**Modern C++**

- "Capture by copy when a lambda outlives its scope; by reference dangles."
- "Fail-able lookups return optional<T>, not null or sentinels."
- "string_view for read-only string params — zero copies. But never store one to a temporary."

**STL**

- "vector by default — contiguous memory beats theoretical complexity."
- "map is a tree; unordered_map is the Dictionary equivalent."
- "operator[] on a map inserts on read — I use find or contains."
- "Erasing during iteration: use erase's return value, or erase_if. And push_back can invalidate everything via reallocation."

**Rule of Five / move semantics**

- "Rule of Zero first: compose from self-managing members and write none of the five. Rule of Five only when holding a raw resource."
- "std::move is just a cast — it grants permission to steal; the move constructor does the stealing."
- "Move = steal the pointer and null out the source, or its destructor double-frees."
- "Move operations get noexcept — otherwise vector copies instead of moving on reallocation."
- "Copy assignment via copy-and-swap: strong exception guarantee, self-assignment safe for free."

**OOP mechanics**

- "Members are constructed in the initializer list, in declaration order — const and reference members can only be initialized there."
- "An interface is an abstract class: all pure virtual, virtual destructor, no data."
- "I keep multiple inheritance to interface-style bases — that sidesteps the diamond problem entirely."
- "Default inheritance for 'class' is private — I always write ': public Base' explicitly."
- "I mark every single-argument constructor explicit unless I deliberately want the implicit conversion."
- "No universal Object root — no free ToString/Equals; comparison and printing are opt-in."

**Errors, casts, strings, UB**

- "Throw by value, catch by const reference — catching by value slices. No finally: RAII is the finally."
- "No exception crosses the add-on boundary — I catch at entry points and translate to error codes. Destructors never throw."
- "static_cast for conversions I can prove, dynamic_cast to query at runtime; const_cast and reinterpret_cast are code-review question marks."
- "std::string is an encoding-unaware byte buffer — I keep it UTF-8 and convert to/from vendor strings explicitly, naming the encoding."
- "UB means the compiler assumes it never happens — Debug-works-Release-breaks is the signature. Sanitizers regularly."
- "Heap use is a deliberate choice in C++ — containers and smart pointers, never bare new."

**C-style SDK specifics**

- "The API is C-flavored: check every error code, zero-init API structs with = {}, pass addresses to be filled in."
- "I wrap every SDK-allocated payload and every opaque handle in an RAII guard so the dispose/close runs on every path."
- "Vendor containers and strings mirror the STL — same concepts and invalidation rules; convert at the boundary, encoding named."
- "A plug-in is a DLL: headers for the compiler, SDK .libs for the linker, the host exports the symbols at runtime."
- "No exception crosses the plug-in boundary, and callbacks registered with the SDK must outlive their registration."

---

---

## Appendix C — Working Without AI Assistants

If your workplace doesn't permit AI tools, the skill to build is **self-sufficiency**: answering your own questions with docs, a debugger, and memory. It is slower but not weaker — retention runs deeper precisely because every answer costs effort. The lookup-and-reason loop is a muscle; it comes back.

### Your offline lifelines

- **cppreference.com** — the canonical C++ reference; a downloadable offline archive exists. Practice navigating it: its style is terse and standards-flavored, and reading it fluently is itself a skill. Look up vector::erase (find the invalidation notes) and string::c_str (find the lifetime rules) as training.
- **The vendor SDK documentation** — usually ships with the SDK as local HTML; your most-opened window. Learn its structure: functions grouped by subsystem, each with requirements and error codes.
- **The SDK's example projects** — the best teacher for "how do I even...". The move when stuck: find the example doing something similar, read it, adapt.
- **Your own notes file** — every gotcha, every conversion snippet (vendor-string/UTF-8), every "how do I attach the debugger again". One searchable file. Months of accumulated snippets are what experienced add-on developers actually run on.
- **This book** — the chapters for re-learning, Appendix B for the morning re-read, Chapter 13 for toolchain commands.

### The escalation ladder when stuck

Practice this order deliberately; most "stuck" moments dissolve at steps 1–3 if you don't skip them:

1. **Read the error properly.** Compile vs link vs runtime (Chapter 12) tells you where to look before you look anywhere.
2. **cppreference / API docs.**
3. **SDK examples** — grep them for the function name you're fighting.
4. **Debugger and AddressSanitizer** — let the tools tell you the truth.
5. **Your notes file.**
6. **The vendor's developer forum/community** — asking is normal and accepted; answers take days, not seconds, so ask early and keep working meanwhile.
7. **A colleague** — with the error, what you tried, and what you ruled out. That framing earns respect and faster help.

### What must live in your head vs what may live in the docs

**In your head (Appendix B material):** the Rule of Five shape, const auto& reflex, catch-by-const-reference, the erase-during-iteration fix, member initializer lists, checking every SDK error code, virtual destructors on polymorphic bases. **In the docs, guilt-free:** exact signatures, container method names, algorithm spellings, API struct fields, format specifiers. Knowing which is which removes both cramming anxiety and lookup shame.

> **Habit:** Every surprise goes into the notes file the moment it happens — not "later". The file is only as good as its worst day.

---

---

## Appendix D — Resources, Further Reading, and First-Week Tips

### C++ references and learning

- **cppreference.com** — the daily reference. Offline archive available.
- **learncpp.com** — free, well-sequenced tutorials; excellent for re-deriving any single topic from scratch.
- **C++ Core Guidelines** (isocpp.github.io/CppCoreGuidelines) — Stroustrup & Sutter's "what good modern C++ looks like"; skim the sections on ownership (R.*) and classes (C.*) — they echo this book's rules with rationale.
- **Compiler Explorer** (godbolt.org) — paste code, see the assembly and try multiple compilers instantly; unbeatable for "does this copy or move?" questions. (Online tool — for home practice if work machines are restricted.)

### Books worth owning

- **A Tour of C++** (Stroustrup) — thin, modern, exactly right for an experienced developer returning; readable in days.
- **Effective Modern C++** (Scott Meyers) — 42 concrete items on C++11/14 (auto, moves, smart pointers, lambdas); the deep version of the ownership, modern-C++, and move chapters (1, 10, 6).
- **C++ Concurrency in Action** (Williams) — when/if threading enters your work.

### Working against a vendor SDK

- The SDK's own local documentation and example projects — installed with it; treat as primary sources, and grep the examples before searching the web.
- The vendor's developer forum or community — where the tribal knowledge lives; search before asking, ask early when needed (answers take days, not seconds).
- The vendor's GitHub organization, if one exists — example plug-ins and helper libraries often live there, more current than the shipped samples.
- For device work, the open ecosystems are excellent study material even if your device is proprietary: **libusb** and **HIDAPI** (USB/HID), **PortAudio** (audio I/O), **SQLite** and **zlib** (canonical C API design) — all small enough to read.

### First-week questions to ask the team

Asking these early is a strength signal, not a weakness:

- Which host-application or SDK versions do we support? (Multi-version support shapes the whole codebase — expect `#if` version guards.)
- Windows only, or Mac too? Which IDE/toolset versions are standard?
- Where is the build documentation, and is there a known-gotchas wiki or its equivalent?
- Who owns plug-in ID registration and code signing?
- What's the code review process, and is there a house style (naming, vendor vs std:: containers, error-handling conventions)?
- Which parts of the API does our product touch most — elements, attributes, listing, dialogs, I/O?

### A closing note

Seventeen years of C# is not baggage here — it is architecture sense, debugging instinct, and professional judgment that transfer completely. The C++-specific layer on top is finite and learnable; most of it is in these pages. The rest arrives the way it always has: one compile error, one code review, one notes-file entry at a time.

---

