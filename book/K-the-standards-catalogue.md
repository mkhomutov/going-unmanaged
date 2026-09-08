## Appendix K — The Standards Catalogue

The book pins `-std=c++17` and says why in [Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency): vendor-SDK work inherits its host's toolset, and C++17 is what that world lets you rely on. Then, forty times across the chapters, it flags a newer spelling — `jthread` for thread-plus-join, `format` for the stream dance, `expected` for the hand-rolled `Result` — each where the C++17 form is taught. That is the right place for the flag and the wrong place to look one up. This page is the lookup: which standard a feature arrived in, the spelling this book teaches, the spelling a newer codebase uses, and the page that owns it; then the history in one sitting, for the reader who wants to know what "modern C++" is modern *relative to*; and first, because it decides everything else, how to find out which standard a toolchain is actually speaking.

It is [Appendix G](G-the-bridge-catalogue.md#appendix-g--the-bridge-catalogue) and [Appendix J](J-cmake-catalogue.md#appendix-j--the-cmake-catalogue)'s shape — lookup material — with one difference: what a probe can ask, a probe asks. `exercises/cookbook/standard.cpp` is built by `build_all.sh` at three standards and refused at a fourth, and by the `buildlab-msvc` job with and without the switch the first section is about. What no probe asks — the default each compiler picks when nobody writes a switch, and which release of a library first shipped a header — is stated from the vendors' documentation, and re-checks itself only on the day you rely on it.

### Which standard am I on

In C# the language version is a line in the project file, `<LangVersion>`, and the compiler that reads it is the one that ships with the SDK — one artifact, one number. C++ has three things that version separately: the **standard** (an ISO document), the **compiler** (which implements some of it), and the **standard library** (a separate project, even when it ships in the same box, which implements the library half on its own schedule). So "which standard am I on" is three questions, and the probe asks all three:

```cpp
--8<-- "exercises/cookbook/standard.cpp:standard-spoken-report"
```

`__cplusplus` is the year and month of the standard the compiler was asked for — `201703` for C++17, `202002` for C++20, `202302` for C++23 — and the first trap is that MSVC reports `199711` for all of them unless you pass `/Zc:__cplusplus`, because too much old code tested the macro against that number for Microsoft to change the default. `_MSVC_LANG` carries the honest value on MSVC either way; a header that must know asks both. The `buildlab-msvc` job builds the probe twice and asserts both readings, because a vendor default is a fact that can change under a book without telling anyone.

> [!WARNING]
> **Trap:** `#if __cplusplus >= 201703L` around a C++17 path builds and runs on MSVC with `/Zc:__cplusplus` off — and takes the other branch, every time, with nothing to report. Guard on the feature's own macro, or on `_MSVC_LANG` as well; the probe shows the two disagreeing.

The second half of the probe asks the library, and it is the half that matters more:

```cpp
--8<-- "exercises/cookbook/standard.cpp:library-features"
```

Every feature the standard adds gets a **feature-test macro** — `__cpp_if_constexpr` from the compiler for a language feature, `__cpp_lib_optional` from the library for a library one — whose value is the year and month the feature reached its current shape, and whose absence means the toolchain does not have it *whatever `__cplusplus` says*. The `<version>` header (C++20, but shipped in C++17 mode by all three libraries) carries the library half in one place. The question to ask before reaching for `std::expected` is therefore not "am I on C++23" but "is `__cpp_lib_expected` defined" — on the machine this page was written on, `-std=c++26` still leaves `__cpp_lib_move_only_function` absent, while the Linux runner's libstdc++ reports it under `-std=c++23` — a fact about two libraries, and no contradiction at all. On this machine, clang with libc++, the probe at the book's floor and at C++23:

```text
$ clang++ -std=c++17 standard.cpp -o s17 && ./s17 | grep -E 'cplusplus|span|expected'
__cplusplus                      201703
__cpp_lib_span                   absent
__cpp_lib_expected               absent
$ clang++ -std=c++23 standard.cpp -o s23 && ./s23 | grep -E 'cplusplus|span|expected'
__cplusplus                      202302
__cpp_lib_span                   202002
__cpp_lib_expected               202211
```

Three spellings for the same switch, and one CMake pair that produces all of them: `-std=c++17` on clang and GCC, `/std:c++17` on MSVC, and `set(CMAKE_CXX_STANDARD 17)` with `CMAKE_CXX_EXTENSIONS OFF` ([Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake)), which writes `-std=c++17` rather than the `gnu++17` dialect. The defaults, when nobody writes a switch: GCC 11 through 15 default to `gnu++17` and GCC 16 to `gnu++20`; clang has defaulted to `gnu++17` since clang 16; MSVC defaults to `/std:c++14`. Which is the whole argument for writing the switch down: a codebase that relies on the default is on a standard chosen by whichever compiler the next person installs.

> [!TIP]
> **Key principle:** "The standard a codebase speaks is a line in its build description, not a fact about its compiler — and whether a feature is there is its feature-test macro's answer, not `__cplusplus`'s, because the library ships behind the language."

The probe's first act is not a print statement but a refusal, and `build_all.sh` asks `-std=c++14` to build it so the refusal is asserted rather than assumed:

```cpp
--8<-- "exercises/cookbook/standard.cpp:standard-spoken-macro"
```

Five lines at the top of a project's one common header do the same job for a codebase: the floor, stated once, enforced by the compiler in a sentence instead of by whichever `optional` first fails to be found three headers deep — [Chapter 41](41-templates-you-will-write.md#chapter-41--templates-you-will-write)'s judge, applied to the toolchain. The `#ifdef` is not decoration. The first draft of this probe asserted `__cplusplus` alone, and the `buildlab-msvc` job refused it at `/std:c++17` with the assertion's own sentence — the trap of the previous section, met before `main` ran, on the first build. And the Linux leg of the same run added a second lesson the section above only implied: GCC 13 reports `__cplusplus` as `202100` for `-std=c++23`, not `202302`, because the value is the compiler's opinion of a standard's date at the time the compiler shipped, and C++23 was not final until after GCC 13 was. Two compilers, one switch, two numbers — the year is a weak key, and the feature-test macro is the strong one.

### The features, by standard

The spelling this book teaches is the C++17 one, and the *newer spelling* column is what changes in a codebase past the floor — nothing else moves. Where a row's newer spelling has a feature-test macro, it is named, so the probe above answers whether a given toolchain has it.

**C++11 and C++14 — the ground the whole book stands on.** Used without comment from Chapter 1; listed so a reader who meets pre-2011 code knows what is missing there.

| Feature | Arrived | Owned by |
|---|---|---|
| `auto`, lambdas, range-`for` | C++11 | [Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency), [Chapter 2](02-value-semantics.md#chapter-2--value-semantics) |
| `unique_ptr`, `shared_ptr`, `weak_ptr` (`make_unique` is C++14) | C++11 | [Chapter 1](01-ownership-and-raii.md#chapter-1--ownership-and-raii) |
| rvalue references, `std::move`, the move operations | C++11 | [Chapter 6](06-the-rule-of-five-and-move-semantics.md#chapter-6--the-rule-of-five-and-move-semantics) |
| `nullptr`, `enum class`, `override`, `final`, `= default`, `= delete`, `constexpr`, `static_assert` | C++11 | [Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency), [Appendix A](A-fundamentals-refresher.md#appendix-a--fundamentals-refresher) |
| member initializers in the class body, delegating constructors | C++11 | [Chapter 4](04-classes-inheritance-interfaces.md#chapter-4--classes-inheritance-interfaces) |
| `<thread>`, `<mutex>`, `<atomic>`, `<condition_variable>`, `<future>` | C++11 | [Chapter 29](29-concurrency.md#chapter-29--concurrency), Recipe 13 |
| `<chrono>` | C++11 | Recipes 6, 28–30 |
| variadic templates | C++11 | [Chapter 41](41-templates-you-will-write.md#chapter-41--templates-you-will-write) |
| init-captures (`[v = std::move(big)]`), generic lambdas (`auto` parameters), `decltype(auto)` | C++14 | [Chapter 22](22-exercise-lambda-lifetimes.md#chapter-22--exercise-lambda-lifetimes), [Chapter 19](19-exercise-the-word-counter.md#chapter-19--exercise-the-word-counter) |

**C++17 — the floor.** Everything here is spelled as taught, everywhere in the book.

| Feature | Owned by |
|---|---|
| `std::optional`, `std::variant` and `std::visit`, `std::string_view`, `std::any` | [Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency), Recipes 19, 20 |
| `std::filesystem` | Recipes 10–12, 38–40 |
| structured bindings | [Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency), Recipe 35 |
| `if constexpr`, fold expressions, class template argument deduction, `std::void_t` | [Chapter 41](41-templates-you-will-write.md#chapter-41--templates-you-will-write) |
| `inline` variables and `inline static` members | [Chapter 12](12-the-compilation-model.md#chapter-12--the-compilation-model), [Chapter 14](14-exercise-the-lifetime-tracer.md#chapter-14--exercise-the-lifetime-tracer) |
| mandatory copy elision for a returned temporary | [Chapter 6](06-the-rule-of-five-and-move-semantics.md#chapter-6--the-rule-of-five-and-move-semantics), [Chapter 14](14-exercise-the-lifetime-tracer.md#chapter-14--exercise-the-lifetime-tracer) |
| `std::from_chars` / `std::to_chars` | Recipe 19; the `double` overloads came later to every library — libstdc++ 11, MSVC 2019 16.4, libc++ 20, and on Apple's libc++ only for a deployment target of macOS 26 or later — so [Chapter 42](42-the-formula-field.md#chapter-42--the-formula-field)'s number parse carries an `#if` |
| `std::invoke`, `std::invoke_result_t` | Recipe 28, [Chapter 38](38-the-bridge-out.md#chapter-38--the-bridge-out) |
| `std::scoped_lock`, `std::shared_mutex` | [Chapter 29](29-concurrency.md#chapter-29--concurrency) |
| `[[maybe_unused]]`, `[[nodiscard]]`, `[[fallthrough]]` | Recipe 24 |
| `enable_shared_from_this` throwing `bad_weak_ptr` on an unowned object | [Chapter 1](01-ownership-and-raii.md#chapter-1--ownership-and-raii) |
| **removed**: dynamic exception specifications (`throw(A, B)`), `std::auto_ptr`; **deprecated**: `<codecvt>` | [Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes), Recipe 17 |

**C++20 — the spellings a newer codebase changes.**

| The book teaches | The C++20 spelling | Macro | Owned by |
|---|---|---|---|
| a `std::thread` joined by hand, or in a destructor | `std::jthread`, `std::stop_token` | `__cpp_lib_jthread` | [Chapter 29](29-concurrency.md#chapter-29--concurrency), Recipe 16 |
| the erase-remove idiom | `std::erase_if` | `__cpp_lib_erase_if` | [Chapter 11](11-stl-containers-and-algorithms.md#chapter-11--stl-containers-algorithms-and-iterator-invalidation) |
| `m.find(k) != m.end()` | `m.contains(k)` | — (no macro; test the year, `>= 202002L`, through `_MSVC_LANG` on MSVC) | [Chapter 11](11-stl-containers-and-algorithms.md#chapter-11--stl-containers-algorithms-and-iterator-invalidation) |
| streams and `snprintf` | `std::format` | `__cpp_lib_format` | Recipes 5, 29 |
| a pointer plus a length | `std::span` | `__cpp_lib_span` | [Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency), [Appendix H](H-choosing.md#appendix-h--choosing-signatures-containers-and-storage) |
| `find_if` with a lambda, the map-to-vector dance | ranges and views | `__cpp_lib_ranges` | [Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency), [Chapter 19](19-exercise-the-word-counter.md#chapter-19--exercise-the-word-counter) |
| the detection idiom, `static_assert` on a trait | concepts and `requires` | `__cpp_concepts` | [Chapter 7](07-templates-vs-csharp-generics.md#chapter-7--templates-vs-c-generics), [Chapter 41](41-templates-you-will-write.md#chapter-41--templates-you-will-write) |
| `__FILE__` and `__LINE__` through a macro | `std::source_location` | `__cpp_lib_source_location` | [Chapter 28](28-testing.md#chapter-28--testing) |
| shifts at documented offsets | `std::endian`, `std::bit_cast` — the advice does not change | `__cpp_lib_endian`, `__cpp_lib_bit_cast` | [Chapter 34](34-parse-this-capture.md#chapter-34--every-capture-rejected-as-malformed) |
| `size_t` loops and `<` comparisons | `std::ssize`, `std::cmp_less` | `__cpp_lib_ssize`, `__cpp_lib_integer_comparison_functions` | [Appendix A](A-fundamentals-refresher.md#appendix-a--fundamentals-refresher) |
| `u8path` | `path(u8"...")` with `char8_t` | `__cpp_char8_t` | Recipe 10 |
| `localtime_r` and an offset you read yourself | `std::chrono::zoned_time`, `clock_cast` | `__cpp_lib_chrono` at `201907` or later — every C++17 library defines it at `201611`, so its presence says nothing | Recipes 6, 29, 39 |
| `#include` | modules (`import`) — headers everywhere in SDK work regardless | `__cpp_modules` (which clang does not define, and GCC only under `-fmodules`) | [Chapter 12](12-the-compilation-model.md#chapter-12--the-compilation-model) |
| a queue and a thread | coroutines — a library on top before they are usable | `__cpp_impl_coroutine` | [Chapter 29](29-concurrency.md#chapter-29--concurrency) |
| hand-written `==` and `<` | `operator<=>`, defaulted comparisons | `__cpp_impl_three_way_comparison` | — |
| `Config c; c.timeout = 30;` | designated initializers, `Config{.timeout = 30}` | `__cpp_designated_initializers` | — |

**C++23 — the ones the book names, each with a check-your-standard flag.**

| The book teaches | The C++23 spelling | Macro | Owned by |
|---|---|---|---|
| `Result<T, E>` over a variant | `std::expected`, `and_then`, `transform` | `__cpp_lib_expected` | [Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes), Recipe 22 |
| `if (!opt) return nullopt;` | `optional::and_then`, `transform`, `or_else` | `__cpp_lib_optional` at `202110` or later | [Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency), Recipe 19 |
| `auto` for a move-only lambda | `std::move_only_function` | `__cpp_lib_move_only_function` | [Chapter 22](22-exercise-lambda-lifetimes.md#chapter-22--exercise-lambda-lifetimes) |
| `text.find(word) != npos` | `text.contains(word)` | `__cpp_lib_string_contains` | Recipe 18 |
| a cast over a mapped region | `std::start_lifetime_as` | `__cpp_lib_start_lifetime_as` | Recipe 43 |
| `dependent_false_v<T>` | `static_assert(false)` in a discarded branch | — (language; clang 17, GCC 13) | [Chapter 41](41-templates-you-will-write.md#chapter-41--templates-you-will-write) |
| — | `std::print` and `std::println`, `std::stacktrace`, `std::flat_map`, `std::mdspan`, deducing `this` | `__cpp_lib_print`, `__cpp_lib_stacktrace`, `__cpp_lib_flat_map`, `__cpp_lib_mdspan`, `__cpp_explicit_this_parameter` | — |

**C++26 — in flight as this is written.** Two rows the book already cites: hardened library preconditions, the standardised form of [Chapter 13](13-toolchain-quick-reference.md#chapter-13--toolchain-quick-reference)'s `_LIBCPP_HARDENING_MODE` and `_GLIBCXX_ASSERTIONS`, and `std::optional<T&>` ([Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency); no macro of its own — `__cpp_lib_optional` at `202506` or later, the probe's `__cpp_lib_optional` line read for its value). The headline features — static reflection, contracts, `std::execution` — none of which this book leans on, arrive in compilers piecemeal over the next several releases, which is the paragraph below in miniature.

### The standards in one sitting

C# ships a language, a compiler and a runtime together, roughly yearly, and the version you are on is the SDK you installed. C++ is an ISO standard with a three-year cadence since 2011, implemented by three compilers and three standard libraries on their own schedules — so "C++20" names a document, and *whether you can use it* is a fact about the toolchain in front of you.

**C++98 and C++03.** The first standard, and a 2003 corrigendum that changed almost nothing a user would notice. This is the C++ a returning developer remembers: `new` and `delete` by hand, `auto_ptr` as the only smart pointer and a broken one, no lambdas, no `auto`, `NULL`, and iterators spelled out in full. Code written for it still exists in every vendor SDK older than a decade, and the Bestiary's C-flavoured shapes ([Chapter 16](16-the-sdk-bestiary.md#chapter-16--the-sdk-bestiary)) are partly a legacy of an era when the language offered nothing better across a binary boundary.

**C++11 — the watershed.** Delayed so long it was called C++0x through most of its development, and the release "modern C++" is modern relative to: move semantics, `unique_ptr` and `shared_ptr`, lambdas, `auto`, range-`for`, `nullptr`, `constexpr`, threads and atomics in the standard library, and a memory model that made [Chapter 29](29-concurrency.md#chapter-29--concurrency)'s data race a defined concept at all. Most of [Appendix B](B-core-principles.md#appendix-b--core-principles-cheat-sheet) is spelled in C++11. A codebase that predates it is a different language wearing the same syntax, and modernising one is a subject of its own rather than this page's.

**C++14.** The bug-fix release: `make_unique` (forgotten in C++11), generic lambdas, init-captures, relaxed `constexpr`. Small enough that "C++11/14" is one era in conversation.

**C++17 — this book's floor.** `optional`, `variant`, `string_view`, `filesystem`, structured bindings, `if constexpr`, guaranteed elision, inline variables: the release after which everyday code stopped needing Boost for the basics. The floor for the reason [Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency) gives — the oldest thing you must link against sets it, and a vendor SDK's pinned toolchain is that thing — and the floor most maintained SDKs now state themselves, which is why the pin is a policy rather than a compromise.

**C++20 — the big one after 11.** Concepts, ranges, coroutines, modules, `format`, `span`, `jthread`, `<=>`, and the feature-test macros standardised in `<version>`. Complete in all three compilers for years now, and the default dialect of GCC 16 — but coroutines need a library on top and modules need build-system support that is still arriving, so "on C++20" usually means the library half and concepts, not the whole document.

**C++23.** `expected`, `print`, monadic `optional`, `flat_map`, `mdspan`, deducing `this`, and `std::start_lifetime_as`. Published in 2024, and the library half is where a toolchain lags most visibly: a compiler accepting `-std=c++23` and a library missing `<expected>` is a normal state of affairs, which is why the probe above asks the library rather than the switch.

**C++26.** Being finalised as this book is written. Static reflection and contracts are the headline; the hardened library is the row a plug-in author meets first. Nothing here changes a lesson in this book; the spellings will move again, and a reader on a C++26 toolchain in 2029 reads the C++17 forms here the way a reader today reads `auto_ptr` — recognisable, superseded, and still in the vendor's sample code.

The cadence has a consequence for the reader's own code that C# never had: a toolchain upgrade is a *standard library* upgrade too, and the two can move separately. A new compiler with the old library, or the reverse, is how `__cplusplus` and `__cpp_lib_expected` come to disagree — and why the probe asks both.

### Moving a codebase past the floor

When a team raises `CMAKE_CXX_STANDARD` from 17 to 20, the rows above are the diff. Nothing in the C++17 spellings stops compiling — the standard removes almost nothing — so the change is a flag, a build, and a reading of the warnings. What to reach for, and what not to:

| You need | Reach for | Think twice about |
|---|---|---|
| To know what standard a codebase speaks | its build description — `CMAKE_CXX_STANDARD`, `-std=`, `/std:` — then the probe | the compiler's version, or its default |
| To know whether the *library* has a feature | its feature-test macro through `<version>` | `__cplusplus`, which the library never reads |
| `expected` on a C++17 codebase | Recipe 22's `Result<T, E>`, or a header-only backport as a [Chapter 27](27-dependency-management.md#chapter-27--dependency-management) dependency | raising the standard for one type |
| `format` before C++20 | fmt, the library `std::format` was standardised from, under Chapter 27's strategies | streams in a hot loop |
| `span` before C++20 | the pointer-plus-length pair, [Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency) | writing a span type of your own |
| `jthread` before C++20 | a thread whose owner joins it in a destructor, Recipe 16 | `detach` |
| Concepts before C++20 | the detection idiom and a `static_assert`, [Chapter 41](41-templates-you-will-write.md#chapter-41--templates-you-will-write) | `enable_if` gymnastics |
| To raise a codebase's floor | flip the switch, build clean, then adopt spellings where they read better | rewriting working code to look newer |
| A vendor sample that is C++98 | read it as the Bestiary shape it is, wrap it in this book's spellings at the boundary | modernising the vendor's code |
