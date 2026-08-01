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


<!-- nav:begin -->
[← Chapter 11 — STL Containers, Algorithms, and Iterator Invalidation](11-stl-containers-and-algorithms.md) · [Contents](README.md) · [Chapter 13 — Toolchain Quick Reference →](13-toolchain-quick-reference.md)
<!-- nav:end -->
