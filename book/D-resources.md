## Appendix D — Resources and Further Reading

### C++ references and learning

- **cppreference.com** — the daily reference. Offline archive available.
- **learncpp.com** — free, well-sequenced tutorials; excellent for re-deriving any single topic from scratch.
- **C++ For C# Developers** (Jackson Dunstan, jacksondunstan.com) — a free article series mapping the entire language, C# construct by C# construct, for exactly this book's reader. The honest division of labour: Dunstan is the encyclopedic syntax-and-semantics mapping — go there when you want every corner of a feature laid against its C# counterpart; this handbook is the job — the labs, the SDK shapes, the sanitizer-verified habits. They compose; read both.
- **C++ Core Guidelines** (isocpp.github.io/CppCoreGuidelines) — Stroustrup & Sutter's "what good modern C++ looks like"; skim the sections on ownership (R.*) and classes (C.*) — they echo this book's rules with rationale.
- **Compiler Explorer** (godbolt.org) — paste code, see the assembly and try multiple compilers instantly; unbeatable for "does this copy or move?" questions. (Online tool — for home practice if work machines are restricted.)
- **C++ Insights** (cppinsights.io) — Compiler Explorer's sibling for a different question: not the assembly, but the C++ your C++ *desugars into* — lambdas as the classes they are ([Chapter 22](22-exercise-lambda-lifetimes.md#chapter-22--exercise-lambda-lifetimes)), range-for expanded, template instantiations made visible ([Chapter 7](07-templates-vs-csharp-generics.md#chapter-7--templates-vs-c-generics)'s per-type stamping, on screen).
- **CppCon and ACCU on YouTube** — free, current, searchable conference talks; the venue where the 2024–26 memory-safety discourse actually happened, if you want the context behind [Chapter 13](13-toolchain-quick-reference.md#chapter-13--toolchain-quick-reference)'s hardened-library switches. Search a topic plus "CppCon" before searching it plus "tutorial" — the median talk is a domain expert with production scars, which is not the median tutorial.

### Books worth owning

- **A Tour of C++, 3rd edition** (Stroustrup) — thin, modern, exactly right for an experienced developer returning; readable in days. The edition matters: the 3rd covers C++20, and the earlier ones are still on shelves.
- **Effective Modern C++** (Scott Meyers) — 42 concrete items; the deep version of the ownership, modern-C++, and move chapters (1, 10, 6). Know what you are buying: the items stop at C++14 and were never revised, so nothing on C++17/20 is in it — and it remains worth owning anyway, because the auto/move/forward/smart-pointer items it is built on are exactly the ground that has not moved.
- **C++ Crash Course** (Lospinoso) — the closest exercise-driven general C++ book, with no C# angle; a good second pass over the whole language once this handbook's job-shaped path is behind you.
- **C++ Templates: The Complete Guide, 2nd edition** (Vandevoorde, Josuttis, Gregor) — the eight hundred pages [Chapter 41](41-templates-you-will-write.md#chapter-41--templates-you-will-write) declines to be; open it the day you must *write* SFINAE, CRTP or a deduction guide rather than recognize one.
- **C++ Concurrency in Action** (Williams) — the full treatment, when threading enters your work. [Chapter 29](29-concurrency.md#chapter-29--concurrency) covers the vocabulary and the callback-thread problem; this is where you go next.

One habit for this whole page: **check the pulse before you adopt.** A book states its edition; a tool or library has a last-release date and an issue tracker, and both go stale without announcement — [Chapter 28](28-testing.md#chapter-28--testing) makes this point about test frameworks, and it generalizes. Thirty seconds on the repository before a download is the whole discipline.

### Working against a vendor SDK

- The SDK's own local documentation and example projects — installed with it; treat as primary sources, and grep the examples before searching the web.
- The vendor's developer forum or community — where the tribal knowledge lives; search before asking, ask early when needed (answers take days, not seconds).
- The vendor's GitHub organization, if one exists — example plug-ins and helper libraries often live there, more current than the shipped samples.
- For device work, the open ecosystems are excellent study material even if your device is proprietary: **libusb** and **HIDAPI** (USB/HID), **PortAudio** (audio I/O), **SQLite** and **zlib** (canonical C API design) — all small enough to read.
