## Chapter 30 — Authoring an ABI Boundary

Chapter 16 catalogued the five shapes vendor APIs take and taught you to wrap them. Every one of those shapes was somebody's design decision, made for reasons this chapter is about — because sooner or later you are on the other side of the table. A plug-in *is* this: the host loads your binary and calls into it, and every constraint you have been obeying as a consumer becomes a constraint you must impose as an author.

The good news is that you have already met the answers. FakeDevice's opaque handle, FakeSDK's error codes, the `Release` function on a payload you did not allocate — those were not arbitrary vendor eccentricities. They are what a stable binary boundary looks like, and by the end of this chapter you will be able to derive every one of them.

### API is not ABI

Two contracts, and C# only ever made you think about the first.

An **API** is a source-level contract: does my code compile against your header. An **ABI** — application binary interface — is the contract between already-compiled artifacts: does my object file *link* against your library, and does the resulting program behave. It covers name mangling, calling conventions, the exact byte offset of every member, vtable layout and ordering, how exceptions propagate across a call, which allocator owns a block, and even the size of `bool`.

C# has no equivalent exposure. An assembly ships metadata; the runtime lays out types at load time; adding a private field to a library and shipping only that DLL simply works, because nothing was ever baked into the caller. That is precisely the property C++ does not have, and it is the same root cause as Chapter 27's missing package manager: the compiler bakes layout decisions into every caller, so the caller and the library must agree exactly.

### The one rule

> [!IMPORTANT]
> **Nothing whose layout your compiler chose may cross the boundary.**

Every technique in this chapter is a way of obeying it, and every corollary follows:

- **No standard-library types in exported signatures.** `std::string` and `std::vector` have different layouts between standard library implementations, sometimes between versions, and on some toolchains between Debug and Release. A parameter of type `const std::string&` is a promise that both sides were built by the same compiler in the same configuration.
- **No exceptions across the boundary.** Chapter 8 said this from the consumer side; now you see why it was your vendor's rule too. Exception propagation is ABI, and it is not portable. Catch everything at your entry points and translate to error codes.
- **Whoever allocates must free.** Two modules may hold two different heaps. A block `new`ed in your library and `delete`d by the caller is undefined behavior even when both are C++, which is why every SDK in this book hands you a `Release` or `Close` function instead of letting you free its memory.
- **No inline function that touches private state.** An inline accessor in your header bakes a member offset into the caller's code, permanently.

### What breaking it looks like

Chapter 27 showed this as an *accident* — two versions of a library colliding in one binary. As a library author it is not an accident, it is your users' standing risk, and it arrives from the most innocent change imaginable: adding a private member.

```cpp
class Naive {
public:
    explicit Naive(std::string n);
    int Score() const;
private:
    std::string name_;
    int score_ = 7;
    double weight_ = 1.5;    // v2 adds this - private, harmless-looking
};
```

`sizeof(Naive)` goes from 32 to 40. Recompile the library, ship it, and do not recompile the caller — which is the entire point of shipping a library. The caller still reserves 32 bytes and still believes `score_` sits where it used to. The v2 constructor initializes `weight_` at offset 32, writing eight bytes past the object the caller allocated:

```text
ERROR: AddressSanitizer: stack-buffer-overflow
WRITE of size 8 at 0x00016ef2e8a0 thread T0
    #0 Naive::Naive(std::string) Naive.h:10
    #2 main naive_use.cpp:3
```

Nothing complained at compile time or link time. And note *when* the sanitizer helped: only because the caller's translation unit was itself instrumented. Rebuild only the library with sanitizers, as you would when debugging your own code, and the report vanishes — the corruption is happening in a frame the sanitizer cannot see. Across a real DLL boundary, where the caller is a host application you did not build, nobody instruments anything.

That is the failure mode you are designing against: silent, remote, and triggered by a change that looks private.

### Technique 1 — PIMPL

Move every data member into a hidden implementation type, and leave the public class holding exactly one pointer.

```cpp
// Widget.h - the only thing your users compile against
#pragma once
#include <memory>
#include <string>

class Widget {
public:
    explicit Widget(std::string name);
    ~Widget();                          // declared here, DEFINED in the .cpp
    Widget(Widget&&) noexcept;          // the same rule applies to move ops
    Widget& operator=(Widget&&) noexcept;
    int Score() const;
private:
    struct Impl;                        // declared, never defined here
    std::unique_ptr<Impl> impl_;
};
```

```cpp
// Widget.cpp - everything real, invisible to callers
#include "Widget.h"
struct Widget::Impl { std::string name; int score = 7; };

Widget::Widget(std::string n) : impl_(std::make_unique<Impl>()) { impl_->name = std::move(n); }
Widget::~Widget() = default;                              // HERE Impl is complete
Widget::Widget(Widget&&) noexcept = default;
Widget& Widget::operator=(Widget&&) noexcept = default;
int Widget::Score() const { return impl_->score; }
```

Look at what is still in that header, though: `std::string` in an exported signature — the thing the corollary above forbids. PIMPL does not fix that and was never meant to. What it fixes is *your class's* layout drift, and what it buys you is the freedom to add private members forever. Both sides must still have been built by the same compiler with the same standard library in the same configuration, which is exactly what the first row of the table below says. If you need more reach than that, you need Technique 3.

Now the proof. Compile a caller **once**, then relink it against two implementations — the second with a `std::vector`, a `double`, and reordered members added to `Impl`, the header untouched:

```text
impl v1 -> sizeof(Widget) as the CALLER sees it = 8, Score()=7
impl v2 -> sizeof(Widget) as the CALLER sees it = 8, Score()=7
```

Eight bytes, both times: one pointer. The implementation can grow without limit and the caller never notices, because there is nothing in the header left to change.

> [!WARNING]
> **Trap:** omit that `~Widget();` declaration and the code fails to compile at the *call site*, with `error: invalid application of 'sizeof' to an incomplete type 'Widget::Impl'` pointing into `unique_ptr`'s internals — the compiler-generated destructor is emitted where the caller is, and there `Impl` is still incomplete. The same applies to the move operations. Declare all of them in the header, define them in the .cpp with `= default`. This trap catches everyone exactly once.

PIMPL costs a heap allocation per object, an indirection per access, and the loss of inlining — real costs, worth paying at a boundary and not worth paying inside your own code. It also cuts compile-time coupling, which is Chapter 12's forward-declaration argument arriving as a bonus.

### Technique 2 — a pure-virtual interface and a factory

Ship an abstract class with no data at all, plus one function to make instances. The caller sees only a vtable shape.

```cpp
// IScorer.h
class IScorer {
public:
    virtual int  Score() const = 0;
    virtual void Destroy() = 0;      // the LIBRARY frees it, with its own allocator
protected:
    ~IScorer() = default;            // non-virtual AND protected: no delete through this
};
extern "C" IScorer* CreateScorer(int seed);   // one unmangled symbol to find
```

The implementation lives entirely in your .cpp, in an anonymous namespace, and never appears in a header. `Destroy` exists because of the whoever-allocates-frees rule — and the destructor is deliberately `protected` and non-virtual so a caller *cannot* write `delete scorer` and get it wrong. Chapter 5 taught that deleting through a base pointer without a virtual destructor is undefined behavior; here you remove the temptation at the type level.

This is Bestiary Shape 3, and now you can read its constraint from the inside: **a published vtable is append-only.** The caller's compiled code reaches methods by index, so inserting a method in the middle, reordering two, or changing a signature silently rebinds every existing caller to the wrong slot — no link error, just the wrong function. Adding at the end is safe. That is the entire reason interface-based ecosystems accumulate names like `IThing2` and `IThingEx`: the second version is a *new* interface because the first one could never change.

One direction only, though, and it is worth knowing which one you are in. Appending is safe when **you** are the sole implementer and the caller only consumes — the arrangement above, where your factory hands out the objects. Turn it around, as a plug-in architecture does when the *host* publishes the interface and your binary implements it, and even appending breaks: an already-compiled plug-in's vtable is short by an entry, and the host reaching for the new slot reads past the end of it. So the rule has a sharper form on that side — a published interface someone else implements can never change at all, which is why COM freezes one the day it ships and adds `IThing2` alongside it for callers to ask for by name. As the plug-in author you do not get to extend the host's interface; you implement whichever versions of it you support.

### Technique 3 — an `extern "C"` façade

The most robust option, because C's ABI is the one thing every toolchain on a platform agrees about. Opaque handle, free functions, error codes:

```cpp
// engine.h - consumable by C, C++, and anything with an FFI
#ifdef __cplusplus
extern "C" {
#endif
typedef struct EngineImpl* EngineHandle;          // opaque: no layout to disagree about
int Engine_Create(int seed, EngineHandle* out);   // 0 = ok
int Engine_Score(EngineHandle h, int* outScore);
int Engine_Destroy(EngineHandle h);
#ifdef __cplusplus
}
#endif
```

```cpp
// engine.cpp - modern C++ inside, C at the seam
struct EngineImpl { int seed; std::string note; };   // std:: is fine IN HERE

extern "C" int Engine_Create(int seed, EngineHandle* out) {
    if (!out) return 1;
    try { *out = new EngineImpl{seed, "internal"}; }
    catch (...) { return 2; }                        // no exception escapes. ever.
    return 0;
}
```

Look at what that header is. Opaque handle you cannot see inside, output parameters filled in through pointers, an integer error code on every function, an explicit destroy. **It is FakeDevice.** You have been consuming this exact shape since Chapter 18, and it looks that way because it is the only shape that survives crossing a binary boundary between two compilers.

The internals stay modern. `std::string` inside `EngineImpl` is perfectly fine — it never crosses. The discipline is only at the seam, which is Chapter 10's advice about the thin disciplined layer, now inverted: there you built it to consume a C API, here you build it to *present* one.

### Choosing

| | Callers must match | Can evolve by | Best for |
|---|---|---|---|
| **PIMPL** | same compiler + stdlib + config | adding anything private | a C++ library shipped with its ecosystem |
| **Pure-virtual interface** | same C++ ABI family | appending methods, or a new interface | plug-ins, COM-style component models |
| **`extern "C"` façade** | almost nothing | adding functions; struct-size versioning | SDKs, cross-language, maximum reach |

Reach and convenience trade against each other in the obvious direction. If your consumers might be built by a different compiler than yours — which is the normal situation for a shipped SDK — the C façade is the only answer that is actually safe, and it is the one every SDK in this book chose.

### Versioning what you published

The boundary is a promise, so plan for the version-two conversation before you have it.

- **Put a size field first in every struct you export.** `struct Options { uint32_t size; ... };` with the caller setting `opts.size = sizeof(Options)` lets version two append fields and detect, at runtime, which version it was handed. You have seen this field in real SDK headers and now you know what it is for.
- **Append, never insert.** In structs, in vtables, in enum values. An offset that has been published is a number someone else's compiled code is already using.
- **Add functions rather than changing them.** `Engine_Create2` is ugly and it is also the thing that lets a five-year-old binary keep working.
- **Never change the meaning of an existing error code.** Add new ones.

### In the wild: shipping a plug-in

- **Nothing escapes your entry points.** Every exported function is a try/catch(...) boundary that translates to an error code. This is exactly the trampoline guard of Chapter 18's stretch goal — you were writing it as a consumer, and it is the same guard you owe your host as an author.
- **Match the host's configuration.** Chapter 26's Debug/Release runtime pitfall is now yours to document for *your* users, because they will hit it and blame you.
- **Static initialization across modules is not ordered.** A global in your library and a global in the host have no defined construction order relative to each other. Prefer the function-local static of Chapter 28's registry, which constructs on first use.
- **Answer Chapter 16's four questions in your own documentation.** Who allocates, who releases and with which function, what the failure contract is, and what thread may call what. You know how much it costs when a vendor leaves one unanswered.

### Pitfalls

- **Exporting a class with inline methods.** The inline body is compiled into the caller; changing it later changes nothing for anyone who already built. It is a permanent commitment disguised as an implementation detail.
- **`std::string` in an exported signature.** The single most common ABI break in practice, because it compiles, links, and works right up until a consumer uses a different standard library or configuration.
- **Letting the caller `delete` your object.** Even with a virtual destructor, it uses the caller's allocator. Hand out `Destroy`.
- **Assuming the same compiler forever.** "We control both sides" is true until a team upgrades a toolset, and the resulting bug reports do not mention that they did.
- **Versioning by changing a struct in place.** Without a size field there is no way for either side to detect the mismatch, and no diagnostic will appear.

> [!TIP]
> **Key principle:** "Nothing whose layout my compiler chose may cross a binary boundary — no std::string, no std::vector, no exceptions, in any exported signature."

> [!TIP]
> **Key principle:** "Whoever allocates, frees — so my boundary hands out a Destroy function rather than letting the caller delete."

> [!TIP]
> **Key principle:** "A published vtable is append-only — I add methods at the end or publish a new interface, because inserting one silently rebinds every existing caller to the wrong slot."

### Try it

Take something you have already written — the Chapter 15 Buffer is ideal, since it owns a resource and has real state — and publish it three ways. The three boundaries printed above are checked in as `exercises/abilab/`, each with a caller of its own, so do the Buffer versions cold first and compare afterwards.

1. **Break it on purpose first.** Put the Buffer's members in the header, compile a caller against it, then add a private member and rebuild *only* the library. Confirm `sizeof` changed, run it, and then rebuild both sides with `-fsanitize=address` to see the overflow. Note that instrumenting only the library hides it.
2. **PIMPL it.** Move the state behind `struct Impl`. Deliberately omit the destructor declaration first so you meet the incomplete-type error on purpose and recognize it forever. Then prove stability: compile the caller once, change `Impl` substantially, relink without recompiling the caller, and watch `sizeof` stay at one pointer.
3. **Interface it.** Publish an abstract `IBuffer` with a factory and a `Destroy`. Then break the vtable rule on purpose — insert a new pure-virtual method at the *top* of the interface, rebuild only the library, and observe the caller now calling the wrong function with no diagnostic anywhere. That five-minute experiment is why `IThing2` exists.
4. **Wrap it in C.** An opaque `BufferHandle`, create/at/size/destroy functions, error codes, and a `catch (...)` in every one. Then compare your header side by side with `exercises/fakedevice/FakeDevice.h` and see how close you landed to it without trying.
5. **Version it.** Add an options struct with a leading size field, then add a field in "version two" and make the library handle both callers correctly at runtime.

---


<!-- nav:begin -->
[← Chapter 29 — Concurrency](29-concurrency.md) · [Contents](README.md) · [Chapter 31 — Reading What the Tools Tell You →](31-reading-what-the-tools-tell-you.md)
<!-- nav:end -->
