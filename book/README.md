# Going Unmanaged

**A Hands-On C++ Handbook for C# Developers**

You have spent years in managed code — the runtime tracked your objects, the GC cleaned up after you, and "unmanaged" was the scary word in the P/Invoke docs. This book is the journey to the other side: refresh, learn, practice.

*Who this is for:* developers with solid C# (or Java) experience who once knew C++ or are learning it now, and need to become productive in a real C++ codebase — typically one built around a vendor SDK: a plug-in API for a desktop application, a peripheral-device SDK, a game or media engine, an embedded HAL. Each chapter therefore ends with an "In the wild" section connecting the concept to the C-flavored APIs you will actually meet, and Part V trains on two miniature SDKs written in those idioms.

*How to use it:* Parts I–IV are the syllabus — read once, then return by chapter when a topic resurfaces at work. Part V is where knowledge becomes skill: a worked example, a practice plan, and a growing log of real findings from real exercises. Part VI is the real codebase — what a project has that an exercise does not; read it when you land in one, or when the thing it covers lands on you. The appendices are the survival kit: the fundamentals refresher, the one-page cheat sheet for any morning, the offline-work playbook, and the cookbook indexed by the C# API you are reaching for.

## Contents

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

10. [Modern C++ Fluency](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency) — auto, lambdas and optional: the C++ that reads like C#
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
24. [Practice Plan](24-practice-plan.md#chapter-24--practice-plan)
25. [Findings from Practice — a Living Log](25-findings-from-practice.md#chapter-25--findings-from-practice-a-living-log)

**[Part VI — The Real Codebase](26-build-systems-and-cmake.md#part-vi--the-real-codebase)**

26. [Build Systems and CMake](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake) — what builds your code when one command no longer will
27. [Dependency Management](27-dependency-management.md#chapter-27--dependency-management) — there is no NuGet, and why that follows from the ABI
28. [Testing](28-testing.md#chapter-28--testing) — build a framework in forty lines, then learn why assertions aren't enough
29. [Concurrency](29-concurrency.md#chapter-29--concurrency) — no runtime, no await, and the thread that calls you back
30. [Authoring an ABI Boundary](30-authoring-an-abi-boundary.md#chapter-30--authoring-an-abi-boundary) — the other side of the Bestiary: shipping the thing someone else loads
31. [Reading What the Tools Tell You](31-reading-what-the-tools-tell-you.md#chapter-31--reading-what-the-tools-tell-you) — sanitizer reports line by line, and the debugger skills that differ
32. [It Crashes on Exit](32-it-crashes-on-exit.md#chapter-32--it-crashes-on-exit) — the first ticket: a crash after `main` returns, and the link-order bet behind it
33. [Here Is the Report](33-here-is-the-report.md#chapter-33--here-is-the-report) — the second ticket: a sanitizer report arrives attached, and the guilty line is in none of its stacks
34. [Parse This Capture](34-parse-this-capture.md#chapter-34--parse-this-capture) — the third ticket: an attached hex dump, a struct that padded itself, and a wire the host read backwards

**[Appendices](A-fundamentals-refresher.md#appendices)**

- A. [Fundamentals Refresher](A-fundamentals-refresher.md#appendix-a--fundamentals-refresher): pointers, references, explicit, = delete, const, .lib files, signed vs unsigned and size_t
- B. [Core Principles](B-core-principles.md#appendix-b--core-principles-cheat-sheet) — the one-page cheat sheet
- C. [Working Without AI Assistants](C-working-without-ai.md#appendix-c--working-without-ai-assistants)
- D. [Resources, Further Reading, and First-Week Tips](D-resources.md#appendix-d--resources-further-reading-and-first-week-tips)
- F. [The Rosetta Cookbook](F-rosetta-cookbook.md#appendix-f--the-rosetta-cookbook) — everyday tasks, indexed by the C# reflex (E is the glossary's letter, when it lands)

*License:* this text is © 2026 Maksim Khomutov and licensed under [CC-BY 4.0](https://creativecommons.org/licenses/by/4.0/) — share and adapt it with attribution to "Going Unmanaged — A Hands-On C++ Handbook for C# Developers" and a link to [the repository](https://github.com/mkhomutov/going-unmanaged). The code is MIT, including every code sample below: paste it into your own work freely.

---

