# Going Unmanaged

**A Hands-On C++ Handbook for C# Developers**

*Managed developer goes native: from RAII to ABI boundaries, with exercises you do cold and every solution green under the sanitizers in CI.*

[![CI](https://github.com/mkhomutov/going-unmanaged/actions/workflows/ci.yml/badge.svg)](https://github.com/mkhomutov/going-unmanaged/actions/workflows/ci.yml)

You have spent years in managed code — the runtime tracked your objects, the GC cleaned up after you, and "unmanaged" was the scary word in the P/Invoke docs. This handbook is the journey to the other side.

## What this is, honestly

This is not a book anyone sat down and wrote — it is a working handbook I *built*, with an AI assistant (Anthropic's Claude), during my own transition back to C++ after ~17 years in C#. The AI drafted the material; I drove the process, worked through the exercises, made the real mistakes that became the Findings log (Chapter 25), and every solution in this repository is verified under a compiler with `-Wall -Wextra` and the sanitizers — Address and UndefinedBehavior for everything, ThreadSanitizer for the threaded lab.

Think of it less as an authored book and more as a curated, battle-tested collection of hands-on material — the kind of thing scattered across the internet, gathered into one coherent path for one specific journey: **managed developer goes native**.

Where this text disagrees with [cppreference](https://cppreference.com) or the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/), trust them. Where you catch it being wrong — fix it (see [Contributing](#contributing)).

## What's inside

The book lives in [book/](book/README.md) — one file per chapter, with the
Contents as the entry point. (Prefer it in one piece? `scripts/build_book.sh`
concatenates the chapters into `build/going-unmanaged.md`, and every tagged
release carries that single file as a download.)

- **Parts I–IV (Chapters 1–13):** the syllabus — ownership and RAII, value semantics, the Rule of Five, virtual dispatch, templates vs generics, error handling, the STL, the compilation model, and the toolchain — every topic anchored in the C# knowledge you already have.
- **Part V (Chapters 14–23):** learning by doing — ten exercises with reference solutions and pitfall analyses, including two miniature vendor SDKs written in the real-world idioms: **FakeSDK** (error codes + owned payloads, the desktop-plugin style) and **FakeDevice** (opaque handles + C callbacks, the peripheral-device style).
- **Chapter 25:** the Findings log — real mistakes made during real practice, each with the theory behind it, broken and fixed code, and the habit to build.
- **Part VI (Chapter 26 onward):** the real codebase — what a project has that an exercise does not, starting with build systems, dependency management, testing, concurrency, authoring an ABI boundary, and reading what the tools tell you. This is where the roadmap's appended chapters land.
- **Appendices:** a fundamentals refresher, a one-page cheat sheet, a playbook for learning with (and without) AI assistants, curated resources, a glossary, and the Rosetta Cookbook indexed by the C# API you are reaching for.

## How to use it

1. Read Parts I–IV once; return by chapter when a topic resurfaces at work.
2. Do the exercises in Part V **cold** — compiler, debugger, sanitizer, and offline docs as your only feedback loops. Each exercise has a task card under [exercises/](exercises/README.md) so you can attempt it without the book's solution on the next screen; read each chapter's reference solution and pitfalls only *after* your own attempt.
3. Keep your own notes file. When an exercise teaches you something the book missed — that's a Finding, and Findings are the contribution this project most wants.

### Running the exercises

First, verify your toolchain — from the repo root:

```bash
./scripts/build_all.sh
```

`ALL GREEN` means your compiler, sanitizers, and this repo agree; the same script is the CI gate.

[exercises/README.md](exercises/README.md) is the index: every exercise's task card, chapter, and time estimate. Build your own attempts with the canonical flags via the helper:

```bash
scripts/check.sh path/to/your_attempt.cpp
```

(`scripts/check.sh your.cpp fakesdk` links the vendor code for the SDK labs.) Every solution builds with:

```bash
g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -g <files> -o out
# (clang++ works identically; MSVC: cl /std:c++17 /W4 /EHsc /fsanitize=address)
```

The SDK exercises are two translation units: `g++ ... FakeSDK.cpp your_solution.cpp`. Both fake SDKs have built-in leak detectors that must report zero when you finish — that check is part of the exercise.

## For team leads — give this to your C# hire

If you manage or mentor an experienced C# (or Java) developer moving onto a C++ codebase, this section is for you; everything above it is for them.

**What to assign.** [Chapter 24](book/24-practice-plan.md) is a one-week plan built for exactly this handover: Day 0 is toolchain setup, Days 1–3 are the core exercises, Days 4–6 run against *your* SDK if they have access to it (the fake SDKs stand in if not), Day 7 consolidates. Ask for the plan as written before assigning real tickets — it is one week, and it front-loads the mistakes you would otherwise meet in code review. After week one, the ticket chapters ([32](book/32-it-crashes-on-exit.md) onward) are shaped like the tickets you will actually assign: symptom first, diagnosis before the fold, an acceptance test that proves the fix's actual claim.

**What maps to your codebase in week one.** Ownership and RAII ([1](book/01-ownership-and-raii.md)), value semantics ([2](book/02-value-semantics.md)), the compilation model ([12](book/12-the-compilation-model.md)) and your build system ([26](book/26-build-systems-and-cmake.md)) cover most of what a first real change touches; if your product is a plug-in or device SDK, add the Bestiary ([16](book/16-the-sdk-bestiary.md)) — it teaches reading a vendor header the way your senior people already do.

**How to verify the labs actually happened.** The exercises grade themselves: `./scripts/build_all.sh` printing `ALL GREEN` proves their toolchain and sanitizers work, and `scripts/check.sh` (or `check.ps1` on Windows) is the one-line judge for their own attempts — a clean run under `-fsanitize=address,undefined` is not fakeable by copy-pasting harder. Ask to see a check.sh run of their cold Buffer rewrite ([Chapter 15](book/15-exercise-the-buffer.md)); it takes them ten minutes and tells you more than an hour of interview questions.

**How to pair with them.** Review, don't rescue. The handbook's own rule for AI assistants ([Appendix C](book/C-working-without-ai.md)) works as well for humans: critique their attempt against the chapter's pitfalls instead of writing the solution — the mistakes are the curriculum, and [Chapter 25](book/25-findings-from-practice.md) exists because they were allowed to happen. Their C# instincts are assets with two exceptions worth watching in early reviews: "hold a reference to it" ([Chapter 33](book/33-here-is-the-report.md)) and "the GC makes cleanup someone else's problem" ([Chapter 1](book/01-ownership-and-raii.md)).

## Contributing

This handbook is open in the fullest sense: **contribute, and become a co-author.**

Most-wanted contributions, in order:

1. **Findings** — you did an exercise, hit something instructive, and can write it up in the Chapter 25 shape (*found in / theory / broken vs fixed / habit*). This is the heart of the project.
2. **Corrections** — anywhere the text is wrong, outdated, or misleading.
3. **New exercises** — especially new SDK shapes (a COM-style refcounting lab and a threaded-callback lab are known gaps).
4. **Missing chapters** — subjects the book does not cover yet. [ROADMAP.md](ROADMAP.md) is the standing list, ranked by what they cost a reader who hits them unprepared: Tier 1 is closed (Chapters 26-29), and the ticket arc has grown to six (Chapters 32-37, with the performance and crash-dump tickets the latest — the format stays open to new tickets by PR). The next major chapter is P/Invoke (item 9, re-sequenced ahead of const-correctness); the glossary landed as Appendix E. Open an issue before starting a large one.
5. **Translations and tooling** — build scripts, per-platform notes, anything that lowers friction.

All contributed code must compile clean under the flags above; CI enforces it. The how — including the Finding template, the ground rules, and the versioning policy (chapter numbers are the public contract; append, don't insert) — is in [CONTRIBUTING.md](CONTRIBUTING.md).

## Contributors

- **Maksim Khomutov** — maintainer and curator; the mistakes in Chapter 25 were made personally.

Accepted contributors are added here — see [CONTRIBUTING.md](CONTRIBUTING.md#attribution).

## License

Dual-licensed ([NOTICE](NOTICE) has it in one place), and the split is
deliberate. The book text — everything under
[book/](book/README.md) and the single file built from it — is
[CC-BY 4.0](LICENSE-CC-BY-4.0): share it, translate it, adapt it, teach from
it, as long as you say where it came from. All the code is
[MIT](LICENSE): `exercises/`, `solutions/`, `scripts/`, `.github/`, **and every
code sample inside the chapters** — so a snippet you paste into your own work
carries no attribution obligation with it. Use it, fork it, teach from it.

Attributing the text takes one line:

> "Going Unmanaged — A Hands-On C++ Handbook for C# Developers" by Maksim
> Khomutov (https://github.com/mkhomutov/going-unmanaged), licensed under
> [CC-BY 4.0](https://creativecommons.org/licenses/by/4.0/).
