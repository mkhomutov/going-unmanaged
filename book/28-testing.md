## Chapter 28 — Testing

In C# testing is free and assumed. A test project, an attribute on a method, `dotnet test`, and the IDE grows a green tree you click. Nobody argues about which framework, because any of them work the same way and the tooling discovers your tests by reflection without being told.

C++ has no built-in testing story, for the reasons Chapters 26 and 27 laid out — no standard build system to hang a `test` command on, no standard package format to ship a framework in. What it has instead is a handful of good third-party frameworks and a decision you have to make.

And it needs testing more, not less. A C# test failure is an assertion message. A C++ bug may not fail *at all* on the machine where you wrote it: undefined behavior that works in Debug and breaks in Release (Chapter 3), a use-after-free that reads plausible stale data, an ODR violation that depends on link order (Chapter 27). The whole category of failure that C# eliminated by construction is the category your tests now have to hunt — and, as this chapter's central demonstration shows, assertions alone cannot do it.

### Why every C++ test framework is made of macros

Worth understanding before you use one, because it explains their strange shape.

In C# a test framework finds your tests by reflection: it loads the assembly, looks for `[Fact]`, and calls what it finds. If an assertion needs the file and line it came from, `[CallerLineNumber]` supplies it, and the failure message can quote your expression because the compiler kept it.

C++ has none of that. There is no reflection, no attribute the runtime can query, and a function cannot know its own call site. Everything a framework needs must be captured at compile time, and the only tool that sees source text is the preprocessor. Hence `#expr` to stringify an expression, `__FILE__` and `__LINE__` to stamp a location, and a macro to register a test function before `main` runs. The macros are not laziness; they are the only mechanism available.

Which means you can build one, and you should, once — the same way Chapter 18 says to write the trampoline once and recognize it forever.

### A test framework in forty lines

```cpp
// tiny_test.h - the three jobs of any test framework: register, check, report.
#pragma once
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace tiny {

struct Test { std::string name; std::function<void()> body; };

// Function-local static: the registry is created on first use, so a registrar
// running before main in ANY translation unit always finds it alive. A plain
// namespace-scope vector would be a bet on initialization order.
inline std::vector<Test>& Registry() {
    static std::vector<Test> tests;
    return tests;
}

inline int& FailureCount() { static int n = 0; return n; }

struct Registrar {
    Registrar(std::string name, std::function<void()> body) {
        Registry().push_back({std::move(name), std::move(body)});
    }
};

inline void Check(bool ok, const char* expr, const char* file, int line) {
    if (!ok) {
        ++FailureCount();
        std::cout << "    FAILED: " << expr << "\n"
                  << "    at " << file << ":" << line << "\n";
    }
}

inline int RunAll() {
    int failed_tests = 0;
    for (const auto& t : Registry()) {
        const int before = FailureCount();
        t.body();
        const bool ok = (FailureCount() == before);
        if (!ok) ++failed_tests;
        std::cout << (ok ? "  [ ok ] " : "  [FAIL] ") << t.name << "\n";
    }
    std::cout << Registry().size() << " tests, " << failed_tests << " failed\n";
    return failed_tests == 0 ? 0 : 1;   // the EXIT CODE is the result CI reads
}

}  // namespace tiny

// Macros, not functions: only the preprocessor can capture the source text of
// the expression (#expr) and the call site (__FILE__ / __LINE__).
#define CHECK(expr) ::tiny::Check((expr), #expr, __FILE__, __LINE__)

#define TEST(name)                                          \
    static void name();                                     \
    static ::tiny::Registrar registrar_##name(#name, name); \
    static void name()
```

The `TEST` macro is the interesting one. It declares a function, defines a namespace-scope object whose *constructor* pushes that function into the registry, and then opens the function body for you to fill in. Because namespace-scope objects are constructed before `main`, every test in every linked translation unit has registered itself by the time `RunAll` looks. That is the same static-initialization mechanism real frameworks use, minus about nine thousand lines of everything else.

### Testability is structural, not a matter of discipline

Now try to test the Buffer from Chapter 15 and you hit a wall immediately: it lives in `buffer.cpp` next to a `main()`. A test binary has its own `main`, and you cannot link two. **Code reachable only from a .cpp with an entry point is code that cannot be tested** — not because you lack discipline, but because of the compilation model in Chapter 12.

So the first act of testing anything is the Chapter 26 split: the class moves to `Buffer.h` (or a header plus a .cpp compiled into a library), and both the demo program and the test binary include it. In C# this problem does not exist — every public type in a project is visible to a test project by adding a reference. Here, testability is a shape you build the code into.

### Testing the Rule of Five

With `Buffer.h` extracted, the assertions write themselves — and notice what they are *about*. In C# you mostly assert on return values. Here the interesting properties are ownership and lifetime:

```cpp
#include "Buffer.h"
#include "tiny_test.h"
#include <vector>

TEST(ConstructorZeroInitializes) {
    Buffer a(4);
    CHECK(a.Size() == 4);
    CHECK(a.At(0) == 0);             // new int[n]{} zero-fills (Finding 7)
}

TEST(CopyIsDeepNotShallow) {
    Buffer a(3);
    a.At(1) = 42;
    Buffer copy = a;
    copy.At(1) = 99;                 // if this were a shallow copy...
    CHECK(a.At(1) == 42);            // ...the original would read 99
    CHECK(copy.At(1) == 99);
}

TEST(CopyAssignAcrossSizes) {
    Buffer a(2);
    Buffer b(5);
    b.At(4) = 7;
    a = b;                           // the old, smaller block must be freed
    CHECK(a.Size() == 5);
    CHECK(a.At(4) == 7);
}

TEST(SelfAssignmentIsHarmless) {
    Buffer a(2);
    a.At(0) = 5;
    const Buffer& alias = a;         // launder it past the compiler's warning
    a = alias;
    CHECK(a.Size() == 2);
    CHECK(a.At(0) == 5);             // copy-and-swap makes this free
}

TEST(MoveLeavesSourceEmptyButValid) {
    Buffer a(3);
    a.At(2) = 11;
    Buffer moved = std::move(a);
    CHECK(moved.Size() == 3);
    CHECK(moved.At(2) == 11);
    CHECK(a.Size() == 0);            // the husk: valid, unspecified, destructible
}

TEST(MoveAssignFreesTheOldBlock) {
    Buffer a(2);
    Buffer b(5);
    b.At(4) = 7;
    a = std::move(b);                // a's own block must be freed, not leaked
    CHECK(a.Size() == 5);
    CHECK(a.At(4) == 7);
    CHECK(b.Size() == 0);            // the source is a husk here too
}

TEST(SelfMoveIsHarmless) {
    Buffer a(2);
    a.At(0) = 5;
    Buffer& alias = a;               // launder it past the compiler's warning
    a = std::move(alias);            // without the guard: delete, then read it
    CHECK(a.Size() == 2);
    CHECK(a.At(0) == 5);
}

TEST(VectorReallocationPreservesContents) {
    std::vector<Buffer> v;
    for (int i = 0; i < 8; ++i) {    // force at least one reallocation
        Buffer b(2);
        b.At(0) = i;
        v.push_back(std::move(b));
    }
    for (int i = 0; i < 8; ++i) CHECK(v[static_cast<size_t>(i)].At(0) == i);
}

int main() { return tiny::RunAll(); }
```

Green, that reads:

```text
  [ ok ] ConstructorZeroInitializes
  [ ok ] CopyIsDeepNotShallow
  [ ok ] CopyAssignAcrossSizes
  [ ok ] SelfAssignmentIsHarmless
  [ ok ] MoveLeavesSourceEmptyButValid
  [ ok ] MoveAssignFreesTheOldBlock
  [ ok ] SelfMoveIsHarmless
  [ ok ] VectorReallocationPreservesContents
8 tests, 0 failed
```

and red, with the expression and location the macro captured:

```text
    FAILED: a.At(1) == 41
    at buffer_test.cpp:16
  [FAIL] CopyIsDeepNotShallow
8 tests, 1 failed
```

Exit code 1. That exit code is the entire interface between your tests and CI.

### The demonstration: every assertion passes and the code is still wrong

Now break the Buffer the subtle way. Not the copy constructor — that would fail an assertion honestly. Break only the *null-out* in the move constructor, leaving the size exchange intact:

```cpp
Buffer(Buffer&& other) noexcept
    : size_(std::exchange(other.size_, 0)),
      data_(other.data_)              // BROKEN: source keeps the pointer too
{}
```

Every assertion in the suite still passes, and that is not a prediction — it is arithmetic. `MoveLeavesSourceEmptyButValid` checks `a.Size() == 0`, and the size *was* zeroed. The only thing wrong is that two objects now own one block, and there is no value anywhere in the file that differs because of it. There is nothing to compare. The program is simply going to free the same memory twice.

Run it under `-fsanitize=address,undefined` and the truth arrives:

```text
==68609==ERROR: AddressSanitizer: attempting double-free on 0x602000000190 in thread T0:
    #0 ... operator delete[]
    #1 ... Buffer::~Buffer() Buffer.h:20
    #2 ... Buffer::~Buffer() Buffer.h:20
    #3 ... MoveLeavesSourceEmptyButValid() buffer_test.cpp:51

freed by thread T0 here:
    #0 ... operator delete[]
    #1 ... Buffer::~Buffer() Buffer.h:20
    #2 ... Buffer::~Buffer() Buffer.h:20
    #3 ... MoveLeavesSourceEmptyButValid() buffer_test.cpp:51

previously allocated by thread T0 here:
    #0 ... operator new[]
    #1 ... Buffer::Buffer(unsigned long) Buffer.h:17
    #2 ... Buffer::Buffer(unsigned long) Buffer.h:17
    #3 ... MoveLeavesSourceEmptyButValid() buffer_test.cpp:45
```

Read that carefully, because the obvious reading is wrong. A double-free report has **three** stacks — the free it stopped, the free that came first, and the allocation — and each one is a single call chain, so no stack here contains two deletes. The pair of `Buffer::~Buffer()` frames is not two destructions; it is *one*. The ABI gives every destructor two entry points — a complete-object one and a base-object one, emitted even for a class like this with no base at all — and unoptimized, the first simply calls the second. The tell is in the third stack, where the same doubling happens to `Buffer::Buffer` — and a constructor plainly ran only once. Rebuild at `-O1` and both pairs collapse to a single frame while the double free stays exactly where it was.

So: two deletes, both reported at `buffer_test.cpp:51`, the closing brace where the two objects go out of scope in reverse order of declaration — `moved` frees the block, then `a` frees it again — and the allocation stack names line 45, `Buffer a(3)`, the block they both think they own. Exit code 134: the process aborted rather than finishing.

Now compare that with what you get *without* the sanitizer. Build the identical broken code with plain `-Wall -Wextra` and run it:

```text
$ ./buffer_test ; echo "exit=$?"
exit=133
```

No output at all. Not one `[ ok ]` line, no summary, no message — the four tests that had already passed had printed into `std::cout`'s buffer, and that buffer was never flushed, because the allocator detected the double free and killed the process on the spot. All you are left with is a number that names no file, no line, and no test. (The exact number is your allocator's business: 133 here, where macOS traps; on glibc you typically get 134 and a terse `free(): double free detected` on stderr. Neither tells you which test.)

The failure has two halves, and it is worth separating them. **The suite found nothing** — every `CHECK` in the file passed. Your assertions are not merely quiet about this bug; they are structurally incapable of seeing it. **And the runtime told you almost nothing** — an unexplained death during teardown, long after the line that caused it. The sanitizer cannot fix the first half; nothing can, short of writing a different kind of check. What it fixes is the second: same workload, same bug, but now a report that names the two destructor frames and the test that ran them.

This is the difference in one page. In C# a passing suite is decent evidence the code is right. In C++ a passing suite that has never run under sanitizers is weak evidence, because the entire class of ownership bugs this book is about produces *no wrong values* — it produces undefined behavior. The tests supply the workload; the sanitizer supplies the verdict.

> [!TIP]
> **Key principle:** "My test binary runs under Address and UB sanitizers, because the bugs that matter in C++ produce no wrong values — assertions supply the workload, the sanitizer supplies the verdict."

### The real frameworks

Write the harness once to understand it, then use something maintained. What you actually get from a real framework: readable assertion output that prints both operands, exception and crash isolation per test, filtering and tagging, fixtures, parameterized tests, and machine-readable output for CI.

- **doctest** and **Catch2** are single-header (or single-header-ish) and need no build integration: drop the header in, `#include` it, done. That matters more than it sounds on a locked-down work machine where installing a package manager is a ticket and downloading one file is not.
- **GoogleTest** is heavier and must be built, which makes it a Chapter 27 dependency decision rather than a file copy. It is extremely common in large codebases, and its `EXPECT_*` / `ASSERT_*` vocabulary is worth recognizing.

The shape barely changes — which is the point of having written your own:

```cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

TEST_CASE("copy is deep") {
    Buffer a(3);
    a.At(1) = 42;
    Buffer copy = a;
    copy.At(1) = 99;
    CHECK(a.At(1) == 42);      // on failure, prints: 99 == 42
}
```

On the build side, Chapter 26's project grows a second executable, and CTest runs it:

```cmake
enable_testing()

add_executable(buffer_test buffer_test.cpp)
target_link_libraries(buffer_test PRIVATE sanitizers)  # Ch 26's INTERFACE target
add_test(NAME buffer_test COMMAND buffer_test)
```

Note what `buffer_test` links and what it does not. `Buffer.h` is header-only, so there is no library of Buffer code to link — but there *is* the `sanitizers` INTERFACE target from Chapter 26, and the test binary is the one target in the project that must never be built without it. If you took the other extraction route and put the implementation in a `.cpp` compiled into a library, link that library here too: the test binary links the code under test, which is the whole reason the split had to happen first.

`ctest --test-dir build` then runs every registered test binary and reports pass/fail from exit codes — the same contract your forty-line harness already honours. (`--test-dir` needs CMake 3.20 or newer; on older tooling, `cd build && ctest`.)

### What is genuinely harder than in C#

**Mocking.** C# mocking libraries fabricate an implementation of an interface at runtime, using reflection and code generation. C++ cannot do that; there is nothing to reflect on. So a fake is a class you write, and the seam has to exist in the design *before* you need it — either a pure-virtual interface (Chapter 4) that both the real and fake implementations satisfy, or a template parameter (Chapter 7) that swaps the dependency at compile time with no virtual call.

You have already seen the answer twice. **FakeSDK and FakeDevice are exactly this pattern**: a stand-in for a vendor dependency, with the same shape as the real thing, that you can build and run on your own machine. That is what testing against an SDK looks like in practice — not a mocking library, but a fake you own.

### In the wild: testing code that needs a host

Plug-in code cannot run standalone: it needs the host application to load it, and half of what it does is call an SDK that expects live state. The professional answer is the one Chapter 10 already gave for a different reason — keep a thin, disciplined layer where you touch the raw C API, and put your actual logic behind it.

That layer split *is* your testability boundary. Geometry, parsing, unit conversion, business rules, and error translation are ordinary C++ that runs anywhere, and those are the parts worth testing and the parts most likely to be wrong. What remains — the calls that genuinely need the host — gets verified by running inside the host with a debugger attached (Chapter 13). Do not fight for 100% coverage of a plug-in; fight for a code base where the interesting logic does not require a host to execute.

### Pitfalls

- **Tests that never see a sanitizer.** The whole point of the demonstration above. Run the test binary under `-fsanitize=address,undefined` in CI, not just locally, not just sometimes.
- **Asserting only on return values.** In C++ the properties worth pinning are frequently invariants: no double free, no leak, moved-from objects still destructible, containers surviving reallocation. Some of those are asserted; some are observed by the sanitizer; the Chapter 14 Tracer can assert on the *counts* of copies and moves when that is the property you care about.
- **A test suite that cannot fail.** Break the code on purpose once and confirm the suite goes red. An untested test is not evidence of anything — this is the "predict-then-run" drill of Chapter 24 applied to your own harness.
- **Testing through the SDK.** A test that needs a device plugged in, or a host running, is an integration test: valuable, slow, and unfit for the fast loop. Keep it, label it, and do not let it be the only thing you have.
- **Forgetting that the exit code is the contract.** A test runner that prints "FAILED" and returns 0 is a green CI build with red tests. Return non-zero.

> [!TIP]
> **Key principle:** "Code reachable only from a .cpp with main() cannot be tested — testability is a structural property, so the logic lives in a library and the entry point stays thin."

> [!TIP]
> **Key principle:** "There is no reflection, so there are no runtime mocks — I design the seam first, as an interface or a template parameter, and write the fake myself."

### Try it

The Buffer of Chapter 15 is the subject, and everything here uses the standard library only, so it works on any machine.

1. **Write `tiny_test.h` from the description, not the listing.** Three jobs: a registry that survives static initialization, a `CHECK` macro that captures expression text and location, a runner that returns a meaningful exit code. Compare with the version above afterwards.
2. **Extract the Buffer** into `Buffer.h` so both a demo program and a test binary can include it — the structural point of this chapter, felt rather than read. This repository has since had the same extraction applied to its own Chapter 15 solution, which is now `solutions/Buffer.h` plus a `buffer.cpp` holding only the demo; do yours first, then compare.
3. **Write the suite.** Deep copy, copy assignment across different sizes, self-assignment, move leaving an empty husk, move assignment over an existing buffer, self-move, and vector reallocation. Predict each result before running, per the Chapter 24 drill. Move assignment earns two of those on its own: it is the one member that frees a block by hand before stealing, so it is the one that can free the wrong thing — or, on a self-move, free the block it is about to read.
4. **Prove the suite can fail.** Change one expected value, confirm red and exit code 1, change it back.
5. **Run the demonstration.** Break only the move constructor's null-out. Confirm every assertion still passes, then rebuild with `-fsanitize=address,undefined` and read the double-free report. This is the single most important run in the chapter: it is what "the sanitizer is the oracle" means, in your own terminal.
6. **Stretch:** put a Chapter 14 Tracer in a `std::vector`, and assert on the number of copies and moves a `push_back` causes. You are now testing a property that has no return value at all — and you will need `noexcept` on the move constructor for the test to pass (Finding 3).

---


<!-- nav:begin -->
[← Chapter 27 — Dependency Management](27-dependency-management.md) · [Contents](README.md) · [Chapter 29 — Concurrency →](29-concurrency.md)
<!-- nav:end -->
