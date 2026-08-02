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

(In C++, class and struct are identical except for two defaults: member *access* — private vs public — and base-class access, so `class D : B` inherits privately where `struct D : B` inherits publicly. Nothing to do with copy semantics.)

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

> [!WARNING]
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

> [!TIP]
> **Key principle:** "C++ is value-semantic by default; I opt into reference semantics explicitly." | "I iterate with const auto& to avoid accidental copies." | "Polymorphic objects go behind unique_ptr — storing them by value slices them."

---

---


<!-- nav:begin -->
[← Chapter 1 — Ownership and RAII](01-ownership-and-raii.md) · [Contents](README.md) · [Chapter 3 — Stack, Heap, and Undefined Behavior →](03-stack-heap-and-undefined-behavior.md)
<!-- nav:end -->
