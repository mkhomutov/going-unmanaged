# Going Unmanaged

**A Hands-On C++ Handbook for C# Developers**

You have spent years in managed code — the runtime tracked your objects, the GC cleaned up after you, and "unmanaged" was the scary word in the P/Invoke docs. This handbook is the other side: the concepts, the labs you do cold, and the reference you keep open once you are there.

*Where to read it:* [mkhomutov.github.io/going-unmanaged](https://mkhomutov.github.io/going-unmanaged/) — the site is built from these files, and it is where the code listings render; on GitHub they show as the include directives that pull them from the tested sources.

*Who this is for:* developers with solid C# (or Java) experience who once knew C++ or are learning it now, and need to become productive in a real C++ codebase — typically one built around a vendor SDK: a plug-in API for a desktop application, a peripheral-device SDK, a game or media engine, an embedded HAL. Each chapter therefore ends with an "In the wild" section connecting the concept to the C-flavored APIs you will actually meet, and Part V trains on two miniature SDKs written in those idioms.

*How to use it:* by the question in front of you, through the Reference list below — the symptom index, the cookbook, the choosing procedures and the catalogues. By topic, through the Concepts, when one surfaces at work. By doing, through the Labs, cold. Read in order, the Parts are the syllabus: I–IV the language and the toolchain, V the exercises and the mistakes they produced, VI the real codebase — what a project has that an exercise does not. The appendices are the survival kit: the fundamentals refresher, the one-page cheat sheet for any morning, the curated resources, the glossary, the cookbook indexed by the C# API you are reaching for, the bridge catalogue for the meeting where the plug-in must speak to everything else, the choosing procedures for the four decisions every signature makes, const-correctness as one subject rather than five fragments, the CMake catalogue for the verb already in your head, and the standards catalogue for the spelling a newer codebase uses and the way to ask a toolchain which standard it speaks.

*For the Java reader:* the comparisons are written in C#, but most of them rest on facts your runtime shares — a GC, objects behind references, single inheritance, an `Object` root, immutable strings, exceptions as the culture — so they translate on sight, and the code always carries the lesson on its own. Read with this pocket dictionary once and you will rarely need it again: `using`/`IDisposable` → try-with-resources/`AutoCloseable`; `base` → `super`; `sealed` → `final`; `internal` → package-private; delegates and `event` → functional interfaces and listener registration; LINQ → Streams; `Task.Run` + `await` → `ExecutorService` + `Future.get`; NuGet → Maven/Gradle; `ArgumentNullException`/`InvalidOperationException` → `NullPointerException`/`IllegalStateException`. Three places the languages genuinely diverge, flagged in place when you get there: **C# has value types** — when Chapter 2 says everything assigns like a C# struct, read *like a Java primitive: the whole object copied on assignment, fields and methods included*; Java has no such type, and this one anchor you must build rather than borrow. **C# generics are reified**, so Chapter 7's baseline is the opposite of your erasure instinct — though C++ templates' T-is-gone-at-runtime half will feel like home. **C# methods are non-virtual by default**, like C++ — so in Chapter 5 it is *your* everything-is-virtual reflex, not the C# reader's, that is the dangerous one.

## Contents

Three ways in. **Reference** is for the question in front of you; **Concepts** for the topic that has surfaced; **Labs** for the work you do cold. The [reading order](#reading-order) below them is the sequence the chapter numbers follow.

### Reference

- [Symptom Index](symptoms.md#symptom-index) — from what is on the screen to the page that owns it
- [Appendix F — The Rosetta Cookbook](F-rosetta-cookbook.md#appendix-f--the-rosetta-cookbook) — everyday tasks, by the C# API you were reaching for
- [Appendix H — Choosing: Signatures, Containers, and Storage](H-choosing.md#appendix-h--choosing-signatures-containers-and-storage) — which container, how to take a parameter, what to return, what goes inside the collection
- [Appendix J — The CMake Catalogue](J-cmake-catalogue.md#appendix-j--the-cmake-catalogue) — the CMake verb already in your head, and the page that owns it
- [Appendix K — The Standards Catalogue](K-the-standards-catalogue.md#appendix-k--the-standards-catalogue) — which standard a toolchain speaks, and the newer spelling beside the taught one
- [Appendix G — The Bridge Catalogue](G-the-bridge-catalogue.md#appendix-g--the-bridge-catalogue) — every way to connect a foreign client to a native host, priced
- [Appendix L — What Things Cost](L-what-things-cost.md#appendix-l--what-things-cost) — the price of the thing you are about to write, and the instrument that would tell you
- [Appendix I — Const-Correctness](I-const.md#appendix-i--const-correctness) — const as one subject
- [Chapter 25 — Gotchas: the Findings Log](25-findings-from-practice.md#chapter-25--gotchas-the-findings-log) — the mistakes practice produced: symptom, theory, broken and fixed code, habit
- [Chapter 31 — Reading What the Tools Tell You](31-reading-what-the-tools-tell-you.md#chapter-31--reading-what-the-tools-tell-you) — sanitizer reports line by line; the debugger and the profiler where they differ from C#
- [Chapter 13 — Toolchain Quick Reference](13-toolchain-quick-reference.md#chapter-13--toolchain-quick-reference) — the flags, and what MSVC calls the thing you know
- [Chapter 16 — The SDK Bestiary](16-the-sdk-bestiary.md#chapter-16--the-sdk-bestiary) — the shapes vendor APIs take, and how to read a header
- [Appendix E — Glossary](E-glossary.md#appendix-e--glossary) — the terms of art, each pointing at its owning chapter
- [Appendix B — Core Principles (Cheat Sheet)](B-core-principles.md#appendix-b--core-principles-cheat-sheet) — the one-page cheat sheet
- [Appendix A — Fundamentals Refresher](A-fundamentals-refresher.md#appendix-a--fundamentals-refresher) — pointers, references, explicit, = delete, const, .lib files, signed vs unsigned, naming
- [Components](components.md#components) — the pieces of the labs and the cookbook that are usable as they stand
- [Appendix D — Resources and Further Reading](D-resources.md#appendix-d--resources-and-further-reading) — references, books, and the vendor-SDK study material

### Concepts

- [Chapter 1 — Ownership and RAII](01-ownership-and-raii.md#chapter-1--ownership-and-raii)
- [Chapter 2 — Value Semantics](02-value-semantics.md#chapter-2--value-semantics)
- [Chapter 3 — Stack, Heap, and Undefined Behavior](03-stack-heap-and-undefined-behavior.md#chapter-3--stack-heap-and-undefined-behavior)
- [Chapter 4 — Classes, Inheritance, Interfaces](04-classes-inheritance-interfaces.md#chapter-4--classes-inheritance-interfaces)
- [Chapter 5 — Virtual Dispatch and the Virtual Destructor](05-virtual-dispatch-and-the-virtual-destructor.md#chapter-5--virtual-dispatch-and-the-virtual-destructor)
- [Chapter 6 — The Rule of Five and Move Semantics](06-the-rule-of-five-and-move-semantics.md#chapter-6--the-rule-of-five-and-move-semantics)
- [Chapter 7 — Templates vs C# Generics](07-templates-vs-csharp-generics.md#chapter-7--templates-vs-c-generics)
- [Chapter 8 — Error Handling: Exceptions and Error Codes](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes)
- [Chapter 9 — Casts, Conversions, and Strings](09-casts-conversions-and-strings.md#chapter-9--casts-conversions-and-strings)
- [Chapter 10 — Modern C++ Fluency](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency)
- [Chapter 11 — STL Containers, Algorithms, and Iterator Invalidation](11-stl-containers-and-algorithms.md#chapter-11--stl-containers-algorithms-and-iterator-invalidation)
- [Chapter 12 — The Compilation Model](12-the-compilation-model.md#chapter-12--the-compilation-model)
- [Chapter 26 — Build Systems and CMake](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake)
- [Chapter 27 — Dependency Management](27-dependency-management.md#chapter-27--dependency-management)
- [Chapter 28 — Testing](28-testing.md#chapter-28--testing)
- [Chapter 29 — Concurrency](29-concurrency.md#chapter-29--concurrency)
- [Chapter 30 — Authoring an ABI Boundary](30-authoring-an-abi-boundary.md#chapter-30--authoring-an-abi-boundary)
- [Chapter 41 — Templates You Will Write](41-templates-you-will-write.md#chapter-41--templates-you-will-write)

### Labs

Each has a task card under `exercises/` so it can be attempted without the solution on the next screen; the six from Chapter 32 on are tickets — a symptom and the code it happened to, the diagnosis behind a fold.

- [Chapter 14 — Exercise: The Lifetime Tracer](14-exercise-the-lifetime-tracer.md#chapter-14--exercise-the-lifetime-tracer)
- [Chapter 15 — Exercise: The Buffer](15-exercise-the-buffer.md#chapter-15--exercise-the-buffer)
- [Chapter 17 — Exercise: The FakeSDK](17-exercise-the-fakesdk.md#chapter-17--exercise-the-fakesdk)
- [Chapter 18 — Exercise: The Device SDK](18-exercise-the-device-sdk.md#chapter-18--exercise-the-device-sdk)
- [Chapter 19 — Exercise: The Word Counter](19-exercise-the-word-counter.md#chapter-19--exercise-the-word-counter)
- [Chapter 20 — Exercise: Slicing and Polymorphism](20-exercise-slicing-and-polymorphism.md#chapter-20--exercise-slicing-and-polymorphism)
- [Chapter 21 — Exercise: Iterator Invalidation](21-exercise-iterator-invalidation.md#chapter-21--exercise-iterator-invalidation)
- [Chapter 22 — Exercise: Lambda Lifetimes](22-exercise-lambda-lifetimes.md#chapter-22--exercise-lambda-lifetimes)
- [Chapter 23 — Exercise: The Build-Model Lab](23-exercise-the-build-model-lab.md#chapter-23--exercise-the-build-model-lab)
- [Chapter 32 — Crash on Exit](32-it-crashes-on-exit.md#chapter-32--crash-on-exit)
- [Chapter 33 — A Value Reads Zero After Hot-Plug](33-here-is-the-report.md#chapter-33--a-value-reads-zero-after-hot-plug)
- [Chapter 34 — Every Capture Rejected as Malformed](34-parse-this-capture.md#chapter-34--every-capture-rejected-as-malformed)
- [Chapter 35 — Objects Still Live at Unload](35-still-live-at-unload.md#chapter-35--objects-still-live-at-unload)
- [Chapter 36 — Dropouts With the Plug-in Loaded](36-the-host-stutters.md#chapter-36--dropouts-with-the-plug-in-loaded)
- [Chapter 37 — Crash at Session Close, Field Units Only](37-no-repro-dump-attached.md#chapter-37--crash-at-session-close-field-units-only)
- [Chapter 38 — The Bridge Out](38-the-bridge-out.md#chapter-38--the-bridge-out)
- [Chapter 39 — The Round Trip Home](39-the-round-trip-home.md#chapter-39--the-round-trip-home)
- [Chapter 40 — CMake for the Plug-in](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in)
- [Chapter 42 — The Formula Field](42-the-formula-field.md#chapter-42--the-formula-field)

### Reading order

**[Part I — The Mental Shift](01-ownership-and-raii.md#part-i--the-mental-shift)**

1. [Ownership and RAII](01-ownership-and-raii.md#chapter-1--ownership-and-raii) — who frees this, and the destructor that guarantees it
2. [Value Semantics](02-value-semantics.md#chapter-2--value-semantics) — assignment copies, and what that costs you
3. [Stack, Heap, and Undefined Behavior](03-stack-heap-and-undefined-behavior.md#chapter-3--stack-heap-and-undefined-behavior) — where a variable lives, and the bugs that don't announce themselves

**[Part II — The Language, Side by Side](04-classes-inheritance-interfaces.md#part-ii--the-language-side-by-side)**

4. [Classes, Inheritance, Interfaces](04-classes-inheritance-interfaces.md#chapter-4--classes-inheritance-interfaces) — the same words as C#, under different rules
5. [Virtual Dispatch and the Virtual Destructor](05-virtual-dispatch-and-the-virtual-destructor.md#chapter-5--virtual-dispatch-and-the-virtual-destructor) — one missing keyword, and every derived destructor skipped
6. [The Rule of Five and Move Semantics](06-the-rule-of-five-and-move-semantics.md#chapter-6--the-rule-of-five-and-move-semantics) — owning a raw resource without double-freeing it
7. [Templates vs C# Generics](07-templates-vs-csharp-generics.md#chapter-7--templates-vs-c-generics) — compile-time code generation, not one runtime type
8. [Error Handling: Exceptions and Error Codes](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes) — two dialects of one language, and how to choose
9. [Casts, Conversions, and Strings](09-casts-conversions-and-strings.md#chapter-9--casts-conversions-and-strings) — four casts by name, and a string that knows no encoding

**[Part III — The Standard Library](10-modern-cpp-fluency.md#part-iii--the-standard-library)**

10. [Modern C++ Fluency](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency) — auto, lambdas, optional and variant: the C++ that reads like C#
11. [STL Containers, Algorithms, and Iterator Invalidation](11-stl-containers-and-algorithms.md#chapter-11--stl-containers-algorithms-and-iterator-invalidation) — the Dictionary equivalents, and the loop that eats itself

**[Part IV — The Build and the Toolchain](12-the-compilation-model.md#part-iv--the-build-and-the-toolchain)**

12. [The Compilation Model](12-the-compilation-model.md#chapter-12--the-compilation-model) — why the error came from the linker
13. [Toolchain Quick Reference](13-toolchain-quick-reference.md#chapter-13--toolchain-quick-reference) — the flags, and what MSVC calls the thing you know

**[Part V — Learning by Doing](14-exercise-the-lifetime-tracer.md#part-v--learning-by-doing)**

14. [Exercise: The Lifetime Tracer](14-exercise-the-lifetime-tracer.md#chapter-14--exercise-the-lifetime-tracer) — seeing every copy, move, and death
15. [Exercise: The Buffer](15-exercise-the-buffer.md#chapter-15--exercise-the-buffer) — the Rule of Five, for real
16. [The SDK Bestiary](16-the-sdk-bestiary.md#chapter-16--the-sdk-bestiary) — the shapes vendor APIs take in the wild
17. [Exercise: The FakeSDK](17-exercise-the-fakesdk.md#chapter-17--exercise-the-fakesdk) — error codes and owned payloads (desktop-app style)
18. [Exercise: The Device SDK](18-exercise-the-device-sdk.md#chapter-18--exercise-the-device-sdk) — opaque handles and C callbacks (peripheral style)
19. [Exercise: The Word Counter](19-exercise-the-word-counter.md#chapter-19--exercise-the-word-counter) — STL fluency end to end
20. [Exercise: Slicing and Polymorphism](20-exercise-slicing-and-polymorphism.md#chapter-20--exercise-slicing-and-polymorphism) — the container that loses your data
21. [Exercise: Iterator Invalidation](21-exercise-iterator-invalidation.md#chapter-21--exercise-iterator-invalidation) — mutating while iterating, safely
22. [Exercise: Lambda Lifetimes](22-exercise-lambda-lifetimes.md#chapter-22--exercise-lambda-lifetimes) — captures that outlive their scope
23. [Exercise: The Build-Model Lab](23-exercise-the-build-model-lab.md#chapter-23--exercise-the-build-model-lab) — provoking and reading every error stage
24. *Retired. The number is not reused.*
25. [Gotchas: the Findings Log](25-findings-from-practice.md#chapter-25--gotchas-the-findings-log) — the mistakes practice produced, numbered and append-only: symptom, theory, broken and fixed code, habit

**[Part VI — The Real Codebase](26-build-systems-and-cmake.md#part-vi--the-real-codebase)**

26. [Build Systems and CMake](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake) — what builds your code when one command no longer will
27. [Dependency Management](27-dependency-management.md#chapter-27--dependency-management) — there is no NuGet, and why that follows from the ABI
28. [Testing](28-testing.md#chapter-28--testing) — build a framework in forty lines, then learn why assertions aren't enough
29. [Concurrency](29-concurrency.md#chapter-29--concurrency) — no runtime, no await, and the thread that calls you back
30. [Authoring an ABI Boundary](30-authoring-an-abi-boundary.md#chapter-30--authoring-an-abi-boundary) — the other side of the Bestiary: shipping the thing someone else loads
31. [Reading What the Tools Tell You](31-reading-what-the-tools-tell-you.md#chapter-31--reading-what-the-tools-tell-you) — sanitizer reports line by line, and the debugger and profiler skills that differ
32. [Crash on Exit](32-it-crashes-on-exit.md#chapter-32--crash-on-exit) — a crash after `main` returns that support cannot reproduce — work it cold
33. [A Value Reads Zero After Hot-Plug](33-here-is-the-report.md#chapter-33--a-value-reads-zero-after-hot-plug) — a sanitizer report arrives attached, and the guilty line is in none of its stacks
34. [Every Capture Rejected as Malformed](34-parse-this-capture.md#chapter-34--every-capture-rejected-as-malformed) — an attached hex dump the vendor's viewer decodes fine and your parser doesn't — while every sanitizer stays green
35. [Objects Still Live at Unload](35-still-live-at-unload.md#chapter-35--objects-still-live-at-unload) — the vendor's 2.0 is refcounted, and the host says objects are still live at unload
36. [Dropouts With the Plug-in Loaded](36-the-host-stutters.md#chapter-36--dropouts-with-the-plug-in-loaded) — a click every few minutes with your plug-in loaded, and an attached profile that says you are innocent
37. [Crash at Session Close, Field Units Only](37-no-repro-dump-attached.md#chapter-37--crash-at-session-close-field-units-only) — a crash report from a machine you will never see, and every bench rig is green
38. [The Bridge Out](38-the-bridge-out.md#chapter-38--the-bridge-out) — serving C#, Python, and the rest from inside the host: one queue, one seam, and a deadline on every wait
39. [The Round Trip Home](39-the-round-trip-home.md#chapter-39--the-round-trip-home) — P/Invoke from the native side: a signature written twice in two languages, and the four things nothing checks
40. [CMake for the Plug-in](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) — a MODULE with one exported symbol, an SDK that ships no config package, and the build decisions Chapter 26 never had to make
41. [Templates You Will Write](41-templates-you-will-write.md#chapter-41--templates-you-will-write) — the working subset: the seam as a template parameter, static_assert as the judge, the three utilities, and how to read the error
42. [The Formula Field](42-the-formula-field.md#chapter-42--the-formula-field) — user-typed text evaluated against the host's objects: tokens as a closed set, a tree behind unique_ptr, recursive descent with a value at every level, the provider seam, and a judge whose oracle is a table worked out by hand

**[Appendices](A-fundamentals-refresher.md#appendices)**

- A. [Fundamentals Refresher](A-fundamentals-refresher.md#appendix-a--fundamentals-refresher): pointers, references, explicit, = delete, const, .lib files, signed vs unsigned and size_t, and naming
- B. [Core Principles](B-core-principles.md#appendix-b--core-principles-cheat-sheet) — the one-page cheat sheet
- C. *Retired. The letter is not reused.*
- D. [Resources and Further Reading](D-resources.md#appendix-d--resources-and-further-reading)
- E. [Glossary](E-glossary.md#appendix-e--glossary) — the terms of art on one page, each pointing at its owning chapter, plus the ones colleagues use without introduction
- F. [The Rosetta Cookbook](F-rosetta-cookbook.md#appendix-f--the-rosetta-cookbook) — everyday tasks, indexed by the C# reflex and its Java neighbour
- G. [The Bridge Catalogue](G-the-bridge-catalogue.md#appendix-g--the-bridge-catalogue) — Chapter 38's lookup half: every way to connect a foreign client to a native host, priced, with the decision table
- H. [Choosing: Signatures, Containers, and Storage](H-choosing.md#appendix-h--choosing-signatures-containers-and-storage) — the four decisions in every signature: which container, how to take a parameter, what to return, and what goes inside the collection
- I. [Const-Correctness](I-const.md#appendix-i--const-correctness) — const as one subject: a path rather than an object, the interface it splits in two, and what retrofitting it costs
- J. [The CMake Catalogue](J-cmake-catalogue.md#appendix-j--the-cmake-catalogue) — Chapters 26, 27 and 40's lookup half: every verb they teach and the page that owns it, then the handful none of them needed, priced, with the decision table
- K. [The Standards Catalogue](K-the-standards-catalogue.md#appendix-k--the-standards-catalogue) — which standard a toolchain is speaking and how to ask it, every feature the book names by the standard it arrived in with the newer spelling beside the one taught, and the standards in one sitting
- L. [What Things Cost](L-what-things-cost.md#appendix-l--what-things-cost) — Chapter 36's lookup half: what a copy, a throw, a `shared_ptr` and a heap allocation cost, which instrument answers which complaint, and the two rows nobody had written down — an arena, and the cache line two counters share

*License:* this text is © 2026 Maksim Khomutov and licensed under [CC-BY 4.0](https://creativecommons.org/licenses/by/4.0/) — share and adapt it with attribution to "Going Unmanaged — A Hands-On C++ Handbook for C# Developers" and a link to [the repository](https://github.com/mkhomutov/going-unmanaged). The code is MIT, including every code sample below: paste it into your own work freely.

---

