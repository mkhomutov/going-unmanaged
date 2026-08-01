## Appendix D — Resources, Further Reading, and First-Week Tips

### C++ references and learning

- **cppreference.com** — the daily reference. Offline archive available.
- **learncpp.com** — free, well-sequenced tutorials; excellent for re-deriving any single topic from scratch.
- **C++ Core Guidelines** (isocpp.github.io/CppCoreGuidelines) — Stroustrup & Sutter's "what good modern C++ looks like"; skim the sections on ownership (R.*) and classes (C.*) — they echo this book's rules with rationale.
- **Compiler Explorer** (godbolt.org) — paste code, see the assembly and try multiple compilers instantly; unbeatable for "does this copy or move?" questions. (Online tool — for home practice if work machines are restricted.)

### Books worth owning

- **A Tour of C++** (Stroustrup) — thin, modern, exactly right for an experienced developer returning; readable in days.
- **Effective Modern C++** (Scott Meyers) — 42 concrete items on C++11/14 (auto, moves, smart pointers, lambdas); the deep version of the ownership, modern-C++, and move chapters (1, 10, 6).
- **C++ Concurrency in Action** (Williams) — the full treatment, when threading enters your work. [Chapter 29](29-concurrency.md#chapter-29--concurrency) covers the vocabulary and the callback-thread problem; this is where you go next.

### Working against a vendor SDK

- The SDK's own local documentation and example projects — installed with it; treat as primary sources, and grep the examples before searching the web.
- The vendor's developer forum or community — where the tribal knowledge lives; search before asking, ask early when needed (answers take days, not seconds).
- The vendor's GitHub organization, if one exists — example plug-ins and helper libraries often live there, more current than the shipped samples.
- For device work, the open ecosystems are excellent study material even if your device is proprietary: **libusb** and **HIDAPI** (USB/HID), **PortAudio** (audio I/O), **SQLite** and **zlib** (canonical C API design) — all small enough to read.

### First-week questions to ask the team

Asking these early is a strength signal, not a weakness:

- Which host-application or SDK versions do we support? (Multi-version support shapes the whole codebase — expect `#if` version guards.)
- Windows only, or Mac too? Which IDE/toolset versions are standard?
- Where is the build documentation, and is there a known-gotchas wiki or its equivalent?
- Who owns plug-in ID registration and code signing?
- What's the code review process, and is there a house style (naming, vendor vs std:: containers, error-handling conventions)?
- Which parts of the API does our product touch most — elements, attributes, listing, dialogs, I/O?

### A closing note

Seventeen years of C# is not baggage here — it is architecture sense, debugging instinct, and professional judgment that transfer completely. The C++-specific layer on top is finite and learnable; most of it is in these pages. The rest arrives the way it always has: one compile error, one code review, one notes-file entry at a time.

---

