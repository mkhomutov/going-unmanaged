## Chapter 5 — Virtual Dispatch and the Virtual Destructor

If a method is not marked **virtual**, the compiler decides which function to call based on the **variable's type, not the object's actual type**. C# would at least warn about hiding; C++ silently does the wrong thing.

**For Java readers:** your reflex is the more dangerous one here. In Java every instance method has always been virtual and overriding just works; in C++ (as in C#) forgetting the keyword means dispatch silently goes static, with no warning. `override` is `@Override` with teeth.

```cpp
class Shape  { public: void Draw() { std::cout << "Shape"; } };   // NOT virtual
class Circle : public Shape {
public: void Draw() { std::cout << "Circle"; } };  // hides, doesn't override

Circle c;
Shape* p = &c;
p->Draw();     // prints "Shape"!  - static dispatch, compile-time decision
```

**Try it (30 seconds).** Predict what the listing prints before running it. Then add `virtual` and `override` and run again — same variable, same object, different function.

The fix, and the modern habit:

```cpp
class Shape  { public: virtual void Draw() { std::cout << "Shape"; } };
class Circle : public Shape {
public: void Draw() override { std::cout << "Circle"; } };  // prints "Circle"
```

### Always write override

It makes the compiler verify you are actually overriding. Without it, this classic bug compiles — silently on GCC and MSVC; clang's `-Wall` does flag it (`-Woverloaded-virtual`), but a warning you may or may not get is not a contract:

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

Each class with virtual functions gets a **vtable** — a hidden array of function pointers. Each object carries one hidden pointer to its class's vtable. A virtual call is a lookup through that pointer. Cost: one pointer per object (one per polymorphic base, if you inherit several), one indirection per call, and no inlining *unless the compiler can prove the dynamic type* — which it often can, for a local, a `final` class or method, or under LTO, and then the call devirtualizes and inlines like any other. That is why C++ makes it opt-in — you do not pay unless you ask.

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

> [!IMPORTANT]
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

> [!NOTE]
> **Surprise for C# devs:** Virtual calls inside constructors/destructors do NOT dispatch to the derived class — during base construction the object still IS just the base. C# dispatches to the derived override (its own famous pitfall, in the opposite direction).

### In the wild: C-style SDKs

SDK surfaces are mostly C-style callbacks, but their C++ layers — and everything you build on top — use polymorphic hierarchies. Any class hierarchy you design for your own model needs virtual destructors the moment you store `unique_ptr<Base>` in a container. Which you will.

---


<!-- nav:begin -->
[← Chapter 4 — Classes, Inheritance, Interfaces](04-classes-inheritance-interfaces.md) · [Contents](README.md) · [Chapter 6 — The Rule of Five and Move Semantics →](06-the-rule-of-five-and-move-semantics.md)
<!-- nav:end -->
