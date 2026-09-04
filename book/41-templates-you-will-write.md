## Chapter 41 — Templates You Will Write

[Chapter 7](07-templates-vs-csharp-generics.md#chapter-7--templates-vs-c-generics) set templates beside generics and stopped where the comparison stopped: instantiation, duck typing, headers, no reflection. [Appendix E](E-glossary.md#appendix-e--glossary) then filed CRTP and SFINAE under *terms you will hear*, "none of these is taught in this book". Both were the right call, and both left a gap the job walks into in month two. Not the template machinery of a library author — that is a different book — but the five or six templates a plug-in author writes because nothing else does the job, and the skill of reading the error when one goes wrong. This chapter is that working subset. The lab is `exercises/templatelab/`, one `Session` compiled against two policies, and a build that must fail.

### The seam as a template parameter

[Chapter 28](28-testing.md#chapter-28--testing) said there are no runtime mocks in C++ and named two seams: a pure-virtual interface, and a template parameter that swaps the dependency at compile time. It showed the first — FakeSDK and FakeDevice are interfaces the vendor happened to write in C — and promised the second. Here it is. The dependency is a *policy*: a type with a handful of static functions and a typedef, named by the template parameter and never virtual.

Two spellings in the listing you have not met. `decltype(expr)` asks the compiler for the type of an expression without running it; `std::declval<X>()` is a pretend value of type `X` that may only appear inside such a `decltype`. Together they let a template ask *would this call compile?* — the whole of `HasSdkShape`, explained in the next section.

```cpp
#pragma once
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>

// The one metaprogramming trick worth owning before concepts: the detection
// idiom. `HasSdkShape<T>` is true when T offers the shape a Session needs,
// and the static_assert below refuses a half-shaped policy at the class -
// where, without it, a policy missing Poll compiles clean until the first
// Pump(), because a member of a class template is only compiled when used.
template <class T, class = void>
struct HasSdkShape : std::false_type {};

template <class T>
struct HasSdkShape<T, std::void_t<
    decltype(std::declval<typename T::Handle>() != typename T::Handle{}),
    std::enable_if_t<std::is_convertible_v<
        decltype(T::Open(std::declval<const char*>())), typename T::Handle>>,
    decltype(T::Close(std::declval<typename T::Handle>())),
    decltype(T::Poll(std::declval<typename T::Handle>(),
                     std::declval<std::function<void(int)>&>()))>> : std::true_type {};

template <class Sdk>
class Session {
    static_assert(HasSdkShape<Sdk>::value,
                  "Sdk policy needs: Handle (default-constructible, !=), "
                  "Open(const char*) -> Handle, Close(Handle), "
                  "Poll(Handle, std::function<void(int)>&)");
public:
    using Handle = typename Sdk::Handle;

    explicit Session(const char* name) : handle_(Sdk::Open(name)) {}
    ~Session() { Sdk::Close(handle_); }
    Session(const Session&) = delete;              // owns a handle: Chapter 6's rule
    Session& operator=(const Session&) = delete;
    Session(Session&& other) noexcept
        : handle_(std::exchange(other.handle_, Handle{})),
          samples_(std::move(other.samples_)) {}
    Session& operator=(Session&&) = delete;

    bool IsOpen() const { return handle_ != Handle{}; }

    // Drain the device: every sample the policy delivers lands in samples_.
    // A moved-from Session owns nothing and pumps nothing - the same answer
    // from both policies, rather than one policy's error and the other's throw.
    std::size_t Pump() {
        if (!IsOpen()) return samples_.size();
        std::function<void(int)> sink = [this](int s) { samples_.push_back(s); };
        Sdk::Poll(handle_, sink);
        return samples_.size();
    }
    const std::vector<int>& Samples() const { return samples_; }
    Handle Raw() const { return handle_; }        // for the SDK's own test hooks only

private:
    Handle handle_{};
    std::vector<int> samples_;
};
```

The class knows its policy's name, `Sdk`, and nothing else; every call is `Sdk::Something`, resolved when the class is instantiated. Two policies make it two classes:

```cpp
#pragma once
#include "FakeDevice.h"

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

// The real thing, in policy clothing: static functions over FakeDevice's C
// API (Chapter 18), with the trampoline inside Poll where the void* is.
struct FakeDeviceSdk {
    using Handle = DeviceHandle;

    static Handle Open(const char* name) {
        Handle h = nullptr;
        const DevErr err = Device_Open(name, &h);
        if (err != DevOk) {
            throw std::runtime_error("Device_Open failed: code " + std::to_string(err));
        }
        return h;
    }
    static void Close(Handle h) { Device_Close(h); }   // null-safe by the SDK's contract
    static void Poll(Handle h, std::function<void(int)>& sink) {
        Device_SetCallback(h, &Trampoline, &sink);
        Device_Poll(h);
        Device_SetCallback(h, nullptr, nullptr);       // sink dies with this frame
    }

private:
    // noexcept: the sink is called from inside the vendor's C frame, and
    // nothing may escape through it (Chapter 30's rule). A throwing sink
    // terminates here, loudly, rather than unwinding through C.
    static void Trampoline(int sample, void* ctx) noexcept {
        (*static_cast<std::function<void(int)>*>(ctx))(sample);
    }
};

// The double: Chapter 28's "a fake you own", swapped in at compile time.
// Every session is an index into a table of scripted samples.
struct RecordingSdk {
    using Handle = std::size_t;                         // 0 = closed; slots start at 1

    static inline std::vector<std::vector<int>> scripts;   // what each open() will deliver
    static inline std::size_t open_count = 0;
    static inline std::size_t close_count = 0;

    static Handle Open(const char*) { ++open_count; return open_count; }
    static void Close(Handle h) { if (h != 0) ++close_count; }
    static void Poll(Handle h, std::function<void(int)>& sink) {
        if (h == 0) return;                             // a closed handle delivers nothing
        for (int s : scripts.at(h - 1)) sink(s);
    }
};
```

`Session<FakeDeviceSdk>` is Chapter 18's session, trampoline and all, with the `void*` inside the policy's `Poll`. `Session<RecordingSdk>` is the same source with no device anywhere: a scripted table, and two counters that prove the handle was opened and closed once each — Chapter 28's "a fake you own", with the fake chosen by the compiler rather than by a factory. There is no vtable pointer in either class and no virtual call across the seam — the compiler sees straight through `Sdk::Poll` to the policy's body and inlines it at `-O2`; the one indirect call left in `Pump` is the `std::function` sink, [Chapter 38](38-the-bridge-out.md#chapter-38--the-bridge-out)'s type erasure, which is a choice about the callback and not about the seam. `main.cpp` asserts the first of those with `std::is_polymorphic_v`, which is the kind of claim this chapter is about.

When to pick this seam over the interface, in one question: **is the dependency known when the program is compiled?** A test double, a platform, a vendor version you build against once — yes, and the template costs nothing. A dependency chosen at run time — a plug-in loaded by name, a device the user picks from a list — no, and that stays a virtual base behind `unique_ptr`, because a template parameter cannot be a run-time value. C# had one seam, the interface, and a container to fill it — C# 11's `static abstract` interface members are this seam spelled as a constraint, `TSdk.Open(name)` under `where TSdk : ISdk`, but they still need the interface written, where the template needs only the calls to compile. C++ has two seams, and choosing is the new skill.

> [!TIP]
> **Key principle:** "A dependency known at compile time is a template parameter, not an interface — the seam Chapter 28 asked for, with no virtual call and a fake I own; one chosen at run time stays a virtual base."

### static_assert and the traits: the compile-time judge

Three chapters in Part VI made a claim about a *type* and checked it at run time or not at all: Chapter 30 printed `sizeof(Widget)` to show PIMPL held, Chapter 34 printed `sizeof(Header)` to discover it was 12 and not 8, Chapter 39 said a struct must be blittable and offered no check. A run-time print was the right teaching move in 30 and 34; the fixed code afterwards deserves a judge that fails the build, and that judge is `static_assert`, with `<type_traits>` as the vocabulary. From the lab's `main.cpp`:

```cpp
static_assert(std::is_nothrow_move_constructible_v<Session<RecordingSdk>>);
static_assert(std::is_nothrow_move_constructible_v<Session<FakeDeviceSdk>>);
static_assert(!std::is_polymorphic_v<Session<RecordingSdk>>);
static_assert(HasSdkShape<RecordingSdk>::value && HasSdkShape<FakeDeviceSdk>::value);
static_assert(!HasSdkShape<int>::value);
```

A failed `static_assert` is a compile error with your sentence in it, at the line you wrote it, and it costs nothing at run time. That makes it the right home for every claim the book has so far put in a comment: "this move is noexcept", "this struct is eight bytes", "this type has no vtable". The nearest C# is a unit test over `Marshal.SizeOf<T>()` or `typeof(T).IsValueType` — a check that runs after the build; `static_assert` is that test moved into the compiler, with no test project. The traits read as questions about a type — `is_trivially_copyable_v` is the necessary half of Chapter 39's blittable; `is_same_v` compares two; `is_arithmetic_v` and `is_convertible_v` sort them — and the `_v` suffix is the C++17 spelling that saves a `::value`.

The one piece of *metaprogramming* worth owning before C++20 is in `session.h` above, and it exists to move an error. `HasSdkShape<T>` is the **detection idiom**: a primary template that says `false`, and a partial specialization that says `true` only when the expressions inside its `std::void_t<...>` — a `Handle` that compares, `T::Open(...)` returning one, `T::Close(...)`, `T::Poll(...)` — are all well-formed. If one is not, the specialization is discarded rather than diagnosed (that discarding is what SFINAE means, and it is the only place this chapter uses it), the primary answers `false`, and the `static_assert` in `Session` refuses the policy at the class, in one sentence naming the shape it wanted. Without it, a policy missing `Poll` is not refused at all until something calls `Pump` — the next section — and then the diagnosis is the compiler's, from inside the template. The idiom checks that the calls are well-formed and, for `Open`, what it returns; it does not read minds — a `Poll` with the right signature and the wrong behavior sails through, as it would through any interface. C++20's concepts are this idiom with syntax: `template <SdkShape Sdk> class Session`, and a concept's failure names the exact expression that failed, where the `static_assert` can only list what it asked for. Until your toolchain is there, this is the spelling.

> [!TIP]
> **Key principle:** "A claim about a type is a static_assert, not a comment — its size, its noexcept move, the shape a policy must have — and a template I write checks its parameter up front, so the error is a sentence rather than a novel."

### The three utilities every codebase writes

```cpp
#pragma once
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

// The dependent-false idiom: a static_assert in a discarded if constexpr
// branch must depend on T, or an older compiler (clang before 17, GCC
// before 13) refuses the whole template at definition.
template <class>
inline constexpr bool dependent_false_v = false;

// if constexpr: one function, one body per kind of T, and the branch not
// taken is discarded - not instantiated, so std::to_string is never resolved
// against a string_view for the string case (it is still parsed).
template <class T>
std::string Describe(const T& value) {
    static_assert(!std::is_null_pointer_v<T>, "Describe: nullptr has no text");
    if constexpr (std::is_same_v<T, char>) {
        return std::string(1, value);                // char is arithmetic: 'A' is not "65"
    } else if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(value);
    } else if constexpr (std::is_convertible_v<T, std::string_view>) {
        return std::string(std::string_view(value));
    } else {
        static_assert(dependent_false_v<T>, "Describe: no spelling for this type");
    }
}

// The variadic every codebase has: C#'s params object[], resolved at compile
// time. The fold expression `((expr), ...)` expands to expr once per argument.
template <class... Parts>
std::string Join(std::string_view separator, const Parts&... parts) {
    std::string out;
    std::size_t n = 0;
    ((out += (n++ ? std::string(separator) : std::string()) + Describe(parts)), ...);
    return out;
}

// A class template with a non-type parameter: the size is part of the type,
// so a Ring<float, 8> and a Ring<float, 16> cannot be confused, and the
// storage is inline (Chapter 11's std::array, not a heap block).
template <class T, std::size_t N>
class Ring {
    static_assert(N > 0, "a Ring needs room for at least one element");
public:
    void Push(T value) {
        data_[head_] = std::move(value);             // a by-value sink is moved from (Appendix H)
        head_ = (head_ + 1) % N;
        if (size_ < N) ++size_;
    }
    std::size_t Size() const { return size_; }
    // Precondition: Size() > 0. An empty ring has no oldest element.
    const T& Oldest() const { return data_[(head_ + N - size_) % N]; }

private:
    std::array<T, N> data_{};
    std::size_t head_ = 0;
    std::size_t size_ = 0;
};
```

**`if constexpr`** is `if` decided at compile time on a condition about the type, and the branch not taken is discarded — not instantiated, so the call `std::to_string(value)` is never resolved against a `string_view` when `T` is a string. It is still parsed, and anything in it that does not depend on `T` is still checked; and outside a template `if constexpr` discards nothing. C#'s nearest thing is a generic method with a `typeof(T)` switch: the compiler emits every branch, and the JIT drops the dead ones only in the per-value-type instantiations. The `static_assert` in the last branch is the idiom for "no spelling for this type", and `dependent_false_v<T>` is there because its condition must mention `T` on any compiler older than clang 17 or GCC 13 — the pitfall below. The `char` branch is the trap the C# reflex walks into: `char` is arithmetic in C++, so without it `Describe('A')` is `"65"`, where `'A'.ToString()` was `"A"`.

**The fold expression** in `Join` is the variadic every codebase ends up with — logging, a formatted message, a path from pieces — and the C# it replaces is `params object[]` with the boxing removed. `Parts...` is a *pack* of types, `parts...` the matching pack of arguments, and `((expr), ...)` is expanded at compile time into one `expr` per argument, which then run left to right. Read the comma fold as the loop it is, unrolled. Recipe 20's `overloaded` — `using Fs::operator()...` — is the same pack expansion applied to base classes.

**The non-type parameter** makes `Ring<float, 8>` and `Ring<float, 16>` different types — Chapter 7 mentioned the feature; this is it doing work. The size is baked in, the storage is a `std::array` inline in the object ([Chapter 11](11-stl-containers-and-algorithms.md#chapter-11--stl-containers-algorithms-and-iterator-invalidation)'s no-heap-block promise), and the `static_assert(N > 0)` refuses a zero-length ring at the declaration rather than at the first push. Where a C# generic would take the capacity in a constructor and check it at run time, this checks it before the program exists.

One thing you will read and rarely write: **class template argument deduction**. `std::pair p{1, 2.0};` and `std::optional o{5};` deduce their arguments from the constructor since C++17, which is why you will meet `std::lock_guard guard(m);` in newer code — this book's listings spell the `<std::mutex>` out, and both name the same type. Your own class templates get it for free when their constructors take the parameter types directly; a *deduction guide* — the `overloaded(Fs...) -> overloaded<Fs...>` line in Recipe 20 — is how you supply it when they do not.

### Reading the novel

The error you have been promised since Chapter 7 — with a correction first. In C# a generic that uses a member `T` did not promise fails at the generic's own line, once, because `where` is checked at the definition. C++ checks at the *use*, and a member of a class template is compiled only when something uses it. So hand `Session` a policy that forgot `Poll`, without the detection idiom, and nothing happens: `Session<HalfSdk> s("x");` builds green, because `Open` and `Close` exist and `Pump` was never asked for. That is the wrong-that-looks-like-working of this chapter — a half-shaped policy that compiles until the first caller, months later — and the reason the lab's `static_assert` sits in the class body rather than in `Pump`.

Call `Pump`, and clang says two things: `no member named 'Poll' in 'HalfSdk'`, then `in instantiation of member function 'Session<HalfSdk>::Pump' requested here`, pointing at your call. Short, because the failure is one level deep. The novel arrives when it is deep: hand a standard template a type that does not do what it needs — `std::sort` on a type with no `operator<`, a `std::map` key without one — and the first line names `__tree`, `_Hashtable` or `__invoke`, followed by one `requested here` per frame out to the line you wrote. Read it the way [Chapter 31](31-reading-what-the-tools-tell-you.md#chapter-31--reading-what-the-tools-tell-you) reads a sanitizer report: find the innermost failure and the frame in *your* file, and ignore the frames between, which are the library's business. Where those two lines sit depends on the compiler — clang and MSVC print the failure first and the trail outward (the last `requested here`, the last `see reference to ... being compiled`, is yours); GCC prints the trail first (`required from here`) and the failure last. The *other* novel, `no matching function for call` followed by pages of `candidate template ignored: substitution failure`, is the same shape from overload resolution: every candidate that dropped out, with its reason, then your line.

With the detection idiom in place the diagnosis is a sentence, and it arrives at the class whether or not anything calls `Pump`. The lab's must-fail build proves it: compiled with `-DTEMPLATELAB_BROKEN_POLICY`, `main.cpp` instantiates `Session` with a policy missing `Poll` *and calls `Pump`* — so that without the `static_assert` the build would fail on the compiler's line instead — and `build_all.sh` asserts the build is refused *and* that the first error line contains `Sdk policy needs`, the `static_assert`'s own text, not the compiler's. That is the constlab discipline from Appendix I applied to templates: a judge that only ever compiles things could not check the one claim this section makes.

### What is not here, on purpose

SFINAE beyond the detection idiom; CRTP; expression templates; `std::enable_if` gymnastics; the metaprogramming that made Boost famous. All real, all in the standard library you use every day, and none of it is a plug-in author's to write in the first year — which is why this chapter is a working subset rather than a tutorial. When one of those names arrives in a review comment, Appendix E says what kind of thing it is; when you need to *write* one, the book to open is Vandevoorde, Josuttis and Gregor's *C++ Templates: The Complete Guide* ([Appendix D](D-resources.md#appendix-d--resources-further-reading-and-first-week-tips)), and the feature to learn first is C++20 concepts, which retire most of the tricks.

### Pitfalls

- **A template parameter for a dependency chosen at run time.** It cannot be; the compiler needs the type. The tell is a `switch` on a string that instantiates one of three templates — that is a virtual interface wearing a template's clothes, and the interface is simpler.
- **A policy with state and no way to reach it.** Static functions keep the seam simple; the moment a policy needs per-session state, either pass a policy *object* (a member of that type, calls through it) or accept that the interface seam was the right one.
- **`static_assert(false)` in an `if constexpr` branch.** Before 2023 that was ill-formed at definition, before any instantiation, and the idiom was a condition that mentions `T` — `util.h`'s `dependent_false_v<T>`. C++23 legalised it and recent compilers (clang 17, GCC 13) accept it under `-std=c++17` too, so a template that builds on your machine can be refused by the CI's older compiler. Keep the dependent spelling until the oldest compiler you ship on is past it.
- **A fold over an empty pack.** `(... + parts)` with zero arguments is ill-formed for most operators; the comma fold in `Join` is fine, and `Join("-")` returning an empty string is the lab's check that it is.
- **Reading a template error from the wrong end.** Find the two lines that matter — the innermost failure (`no member named`, `no matching function`) and the frame in *your* file (clang's last `requested here`, GCC's first `required from here`, MSVC's last `see reference to`) — and ignore the frames between.
- **`char` is arithmetic.** `Describe('A')` through the number branch is `"65"`; the C# reflex expects `"A"`. Any `if constexpr` that sorts types by `is_arithmetic_v` needs to decide what a `char` is first.
- **Putting the template body in a `.cpp`.** Chapter 7's oldest trap, and still the first thing that goes wrong when a class becomes a template: the body moves to the header, or the linker complains for every instantiation the `.cpp` did not know about.

### Try it

The lab is `exercises/templatelab/` — the task card walks the same road as this chapter, cold. In outline:

1. **Write the seam.** A `Session<Sdk>` over a policy with `Handle`, `Open`, `Close` and `Poll`, and two policies: one over `FakeDevice.h` (the trampoline lives in the policy now — where does the `void*` go?), one recording double with counters.
2. **Judge it twice.** Open, pump, move and close under the canonical flags against both policies; assert the double's open and close counts, and `FakeDevice_OpenHandles() == 0` for the real one. Then add the `static_assert`s: no vtable, `noexcept` move, both policies pass the shape check and `int` does not.
3. **Break a policy and read the error, twice.** Delete `Poll` from a policy and build: green, if nothing calls `Pump`. Call it, build again, and find the two lines that matter. Then add the detection idiom and build once more: one sentence, at the class, with or without the call. That last build, refused by name, is the lab's judge.
4. **Write the three utilities** from their descriptions: `Describe` with `if constexpr`, `Join` with a fold, `Ring<T, N>`. Assert each, and put a `static_assert` on the ring's size.
5. **Stretch: the concept.** On a C++20 toolchain, replace `HasSdkShape` with a `concept SdkShape` and `template <SdkShape Sdk>`, and compare the error text for the broken policy with the `static_assert`'s: the concept's names the exact expression that failed; the `static_assert`'s can only list what it asked for.

---


<!-- nav:begin -->
[← Chapter 40 — CMake for the Plug-in](40-cmake-for-the-plug-in.md) · [Contents](README.md) · [Appendix A — Fundamentals Refresher →](A-fundamentals-refresher.md)
<!-- nav:end -->
