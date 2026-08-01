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

> [!WARNING]
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


<!-- nav:begin -->
[← Chapter 3 — Stack, Heap, and Undefined Behavior](03-stack-heap-and-undefined-behavior.md) · [Contents](README.md) · [Chapter 5 — Virtual Dispatch and the Virtual Destructor →](05-virtual-dispatch-and-the-virtual-destructor.md)
<!-- nav:end -->
