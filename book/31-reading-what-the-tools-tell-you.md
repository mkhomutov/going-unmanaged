## Chapter 31 — Reading What the Tools Tell You

Chapter 24's practice plan tells you, on Day 2, to break the Buffer three ways and "read its reports until they make sense". The book has never shown you one. This chapter is that omission repaired, plus the debugger and profiler skills that differ most from the C# experience.

The difference in posture is worth naming first. In C#, a failure comes to *you*: an exception with a type, a message, and a stack trace, thrown at the moment of the mistake. In C++, a failure is usually silent — the program keeps running with corrupted memory and dies somewhere unrelated, or doesn't die at all. The tools that turn silence into a report have to be invited in, at build time, before the run. That is why the sanitizer flags are in every command in this book.

### Four report shapes

The most useful thing to learn first is not how to read a report in detail — it is that each kind of report has a **characteristic shape**, and the shape names the bug class before you read a word.

| Shape | What it is |
|---|---|
| **Three stacks** (access, free, allocation) | use-after-free or double-free |
| **Two stacks** (access, allocation) | heap buffer overflow — the block is alive, you went outside it |
| **One stack** (allocation only) | a leak: the block's birth is the only trace it ever left |
| **No stack, one line with a column number** | UndefinedBehaviorSanitizer |

That table is worth more than any single walkthrough. Glance at a report, count the stacks, and you already know what you are dealing with.

One qualification, because it is the case where counting misleads: only *heap* overflows carry an allocation stack, since only heap blocks have a birth site worth recording. A `stack-buffer-overflow` or `global-buffer-overflow` shows the access stack alone and then names the offending variable outright — `[32, 48) 'local_buf' (line 3) <== Memory access at offset 52 overflows this variable` for a local, or the declaration site for a global. That is more information than the heap case gives you, not less. Do not count that single stack and conclude "leak": the error name on the first line is always the tiebreaker, and the shape is the index, not the verdict.

### An ASan report, line by line

Here is the canonical one. A `Reading` is allocated, released, and then read:

```text
==74725==ERROR: AddressSanitizer: heap-use-after-free on address 0x6020000000b8
                                  at pc 0x0001028b4994 bp 0x00016d54a9d0 sp 0x00016d54a9c8
READ of size 8 at 0x6020000000b8 thread T0
    #0 0x0001028b4990 in main uaf.cpp:15

0x6020000000b8 is located 8 bytes inside of 16-byte region [0x6020000000b0,0x6020000000c0)
freed by thread T0 here:
    #0 ... in _ZdlPv (libclang_rt.asan_osx_dynamic.dylib)
    #1 0x0001028b4920 in Release(Reading*) uaf.cpp:9
    #2 0x0001028b4964 in main uaf.cpp:14

previously allocated by thread T0 here:
    #0 ... in _Znwm (libclang_rt.asan_osx_dynamic.dylib)
    #1 0x0001028b483c in MakeReading(int) uaf.cpp:5
    #2 0x0001028b4958 in main uaf.cpp:13

SUMMARY: AddressSanitizer: heap-use-after-free uaf.cpp:15 in main
```

Read it in this order:

1. **The error name.** `heap-use-after-free`. Everything else is evidence for it.
2. **The first stack answers "where did I touch it?"** `READ of size 8` at `uaf.cpp:15`. Size 8 is a `double` — already a hint about which member.
3. **The region line answers "what did I touch?"** `8 bytes inside of 16-byte region`. A 16-byte object, and you were 8 bytes in: given `struct Reading { int id; double value; }`, that is `value`, at offset 8 after padding. The report just told you the field name without knowing it. Read the preposition carefully — `inside of` means you were within a block that had been freed; a buffer overflow says `12 bytes after 16-byte region` instead, and that one word is the difference between a lifetime bug and an indexing bug.
4. **The second stack answers "who killed it?"** — `Release(Reading*) uaf.cpp:9`, called from `main` at line 14.
5. **The third stack answers "where did it come from?"** — `MakeReading(int) uaf.cpp:5`, called from line 13.
6. **The SUMMARY line** repeats the site. It is what to paste into a notes file, and what to search for.

Frame `#0` in the free and allocation stacks is always inside the sanitizer runtime (`_ZdlPv` is the mangled `operator delete`, `_Znwm` is `operator new` — Chapter 12's name mangling in the wild). **Start reading at `#1`**, the first frame in your own code. Everything below the `SUMMARY` line is the shadow-byte map, which you can ignore; it is a picture of ASan's own bookkeeping, and in years of use you will want it approximately never.

Three stacks, three questions, and between them the whole life of the object. No C# tool tells you this, because in C# the object could not have been in this state.

### The leak report, and a platform caveat

A leak report carries exactly one stack — the allocation — because a block nobody freed and nobody can reach has left no other trace. That asymmetry is worth internalizing: with a use-after-free you get the whole story, and with a leak you get only the birth certificate.

There is a practical catch that costs an afternoon if you meet it unprepared: **LeakSanitizer is not available on macOS/arm64.** Asking for it says so plainly:

```text
==74907==AddressSanitizer: detect_leaks is not supported on this platform.
```

On Linux — including this repository's CI — leak detection runs automatically at normal program exit. On an Apple-silicon Mac it does not run at all, so a leaking program under ASan looks clean. If you are practising on a Mac, treat "no leak report" as "no information", and get your leak coverage from CI, from a Linux container, or from the platform's own tooling.

### The UBSan report is a different animal

```text
ub.cpp:2:29: runtime error: signed integer overflow: 2000000000 * 2 cannot be represented in type 'int'
SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior ub.cpp:2:29
```

No stack. A column number, pointing precisely at the operator — and one the ASan transcripts above never showed you, though that is a fact about this machine rather than about ASan: macOS symbolizes with `atos`, which stops at the line, while Linux uses `llvm-symbolizer`, which supplies columns to every sanitizer alike. So on Linux the missing *stack* is the tell, not the column. And — the part that catches people — **the program then carried on and exited 0**. UBSan's default is report-and-continue, so a script that only checks the exit code will call this run a success.

Two options fix it, and it is worth knowing both:

```bash
UBSAN_OPTIONS=halt_on_error=1 ./app          # runtime: stop at the first finding
c++ ... -fno-sanitize-recover=undefined      # build time: same effect, baked in
```

Both make the run exit nonzero — 134 on macOS, where the sanitizer runtime calls `abort()` after printing, and 1 on Linux, where it exits with the runtime's default `exitcode`. What matters is that it is no longer 0. This repository's `scripts/check.sh` uses the first, which is why a learner's attempt containing undefined behavior fails the check rather than passing it quietly.

And when the one-line report is not enough to locate the cause, UBSan will produce a stack on request:

```bash
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./app
```

```text
ub.cpp:2:29: runtime error: signed integer overflow: ...
    #0 0x00010469c68c in Scale(int) ub.cpp:2
    #1 0x00010469c6c0 in main ub.cpp:5
```

> [!TIP]
> **Key principle:** "The shape of a sanitizer report names the bug class before I read it — three stacks is a use-after-free, one is a leak, and no stack at all with a column number is UBSan."

### Optimization erodes your evidence

Build the same use-after-free at `-O2` and the report gets worse — not wrong, worse:

```text
freed by thread T0 here:
    #1 0x0001027fc8d0 in main uaf.cpp:14

previously allocated by thread T0 here:
    #1 0x0001027fc8c8 in main uaf.cpp:13
```

`Release` and `MakeReading` have vanished. They were inlined, so the frames that named *who* freed the block and *who* allocated it no longer exist. In a two-function toy that costs nothing; in a real call chain it is the difference between a diagnosis and a shrug.

This is Chapter 13's Debug-versus-Release advice arriving from the other direction. Test both configurations, because UB hides in one and detonates in the other — but when you sit down to *diagnose*, reproduce at `-O0 -g` first, so the tools can still tell you who did what.

> [!TIP]
> **Key principle:** "A stack from an optimized build is missing frames — I reproduce at -O0 -g before I believe what a trace does or doesn't say."

### The debugger, where it differs from C#

Two habits transfer unchanged: breakpoints and stepping. Three things are different enough to be worth practising deliberately.

**There is no exception to catch.** In C# you break on a thrown exception and read its message. Here the process stops on a signal — a segmentation fault, an abort — or the sanitizer traps first. `lldb` and Visual Studio both stop with the sanitizer's own description; the ASan run above, under `lldb`, reports `stop reason = Use of deallocated memory` and puts you on the offending frame. Get the backtrace with `bt`, move with `frame select N`, and inspect with `frame variable`.

**Watchpoints answer a question C# cannot ask.** "Something is setting this value to -1 and I do not know what" is a common and miserable bug. In C++ you can stop the program the moment a *memory location* changes:

```text
(lldb) b watch.cpp:7
(lldb) run
(lldb) watchpoint set variable c.timeout
Watchpoint created: Watchpoint 1: addr = 0x16fdfe7ec size = 4

(lldb) continue
Watchpoint 1 hit:
old value: 30
new value: 45
    frame #0: Configure(c=0x000000016fdfe7e8) at watch.cpp:3:45

(lldb) continue
Watchpoint 1 hit:
old value: 45
new value: -1
```

Old value, new value, and the exact frame that did it. In Visual Studio this is a **data breakpoint** (Debug ▸ New Breakpoint ▸ Data Breakpoint). It has always been a natural fit in native code, where an object sits at a fixed address for its whole life; on the managed side a moving garbage collector makes "watch this address" a harder proposition, and tooling support arrived later and with more restrictions. Whatever you were used to, build the reflex here: *stop adding print statements to find out who wrote a value; ask the hardware.*

Watchpoints are a scarce hardware resource — a handful at a time, each covering a small number of bytes — so set them on the specific member that is going wrong, not on a whole object.

**Container inspection needs the visualizers.** Chapter 13 said this; it bears repeating because the raw view of a `std::vector` is three pointers and looks like nothing. Visual Studio does it out of the box; in `lldb`, `frame variable` applies the standard library's formatters and prints elements.

> [!TIP]
> **Key principle:** "When I need to know who changed a value, I set a watchpoint — old value, new value, and the frame that did it — instead of adding print statements."

### The profiler, where it differs from C#

[Chapter 36](36-the-host-stutters.md#chapter-36--the-host-stutters) taught you to read a profile and never said how the profile was taken. In C# that was never a question worth a section: the Visual Studio profiler or PerfView attaches to a managed process and the runtime hands it every frame's name, and allocations are a runtime event the same tools subscribe to, because the allocator is the runtime's — nobody replaces `new` to count them. Here the things the runtime supplied are things you arrange before the first sample lands, and the tools differ per platform the way Chapter 13's debuggers did. Nothing in this section is checked by the repository's CI — a profile is a fact about one machine and one run — so read it as the checklist it is; the two compiler facts in it were checked on the way in, and the command lines were run.

**The tools, by platform.** All three are *sampling* profilers, the instrument Chapter 36's attachment came from: a timer interrupts the thread, the stack is written down, and a symbol's count is how often it was on it.

- **Linux: `perf`.** `perf record -g ./app` then `perf report`, or `perf record -g -p <pid>` to sample a process already running, which is how you profile a host. Frame-pointer unwinding by default; `--call-graph dwarf` when the binary was built without them, at a cost in file size.
- **macOS: `sample`, then Instruments.** `sample <pid> 10` is on every Mac, needs no Xcode, and attaches to a running host for ten seconds; Chapter 36's listing is in its shape, the `(in meter.plugin)` module column included (`[self]` is that chapter's own marker). Instruments' *Time Profiler* template is the same sampler with a UI — pick the running host in its target chooser, or `xctrace record --template 'Time Profiler' --attach <pid>` from the command line, which needs the full Xcode.
- **Windows: Visual Studio's Performance Profiler** (Debug ▸ Performance Profiler, the *CPU Usage* tool), which attaches to a running process the same way the debugger does; Windows Performance Recorder and Analyzer when the question is system-wide rather than one process.

**Three things to arrange before the first sample lands.**

1. **Build the thing you ship, with the names kept.** Profile at `-O2` (`/O2` on MSVC): an unoptimized profile is a profile of a different program, with copies and calls the optimizer would have removed. Do not strip, and keep `-g` (`/Zi`, and `/DEBUG` to the linker, which the IDE's Release configuration already passes) so the inline records survive; `-g` changes no instruction at `-O2`. The symbol file is the artifact [Chapter 37](37-no-repro-dump-attached.md#chapter-37--no-repro-dump-attached) archives for crash reports, and a profile of a stripped binary is a column of addresses for the same reason a stripped crash report is — for [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in)'s plug-in, hidden by preset with one export, one name and offsets from it.
2. **Keep the frame pointers.** Add `-fno-omit-frame-pointer` on clang and GCC for x86-64, where `-O2` drops the register. Without it a sampler walking the stack by frame pointer truncates or garbles the callers, and a profile in which every function is called from nowhere, or from somewhere absurd, is the tell — and no tool names a profile taken from the wrong build; the shape is the only warning you get. (macOS on arm64 keeps the frame pointer by ABI and MSVC's x64 code keeps unwind tables the profiler walks regardless, so a Mac reader will not meet this tell at home.)
3. **Expect the inlining.** The `-O2` section above showed frames vanish from a sanitizer stack; they vanish from a profile the same way, and an inlined function's time is charged to its host — Chapter 37's `FoldedMean` inlined into `Report` appears as `Report` here — unless you ask, as that chapter did: `perf report --inline`, and Instruments expands them when the dSYM sits beside the binary. When a hot function seems to be doing more than its own body, ask what was inlined into it before asking what it does.

**Attaching to the host.** A plug-in profiles the way it debugs (Chapter 13): the host is the process, and you attach after it has loaded your module. Your frames are the ones carrying your module's name, and the reflex to build is Chapter 36's vendor engineer's: filter to your module, then look under your own frames for symbols that have no business there. A `malloc`, a `memmove`, a lock in a peak meter's subtree is a finding at any percentage.

**Which instrument answers which question.** A sampler measures the mean: where the time goes, on average, which is the right question for throughput work. It cannot show you a rare event — the one allocation in four million that took milliseconds — because it records weight, not duration: forty-one three-millisecond stalls are a sliver indistinguishable from a hundred thousand microsecond calls, and if the stall is a wait on a lock the thread is off the CPU and a CPU sampler never sees it at all. For the tail, the instrument is a *counter* or a *trace*: Chapter 36's replaced `operator new` counting allocations, RealtimeSanitizer aborting on the first one, or a tracer that records every event with a timestamp (`perf sched` on Linux, Instruments' *System Trace*, Event Tracing for Windows) and shows you the one long gap an average smoothed away. Read the complaint first: *slow* is a sampler's question; *stutter*, *spike* and *dropout* are a counter's.

**Timing one function.** Recipe 28 in [Appendix F](F-rosetta-cookbook.md#appendix-f--the-rosetta-cookbook) is the stopwatch, and its trap already says to consume the result and measure at `-O2` without the sanitizers. What the recipe leaves to you: warm the code up before measuring, repeat and report a minimum or a percentile rather than a mean, and compare two versions on the same machine in the same session, because a number from last week on a different laptop is a rumour. When this grows past a loop and a stopwatch, Google Benchmark mechanizes all of it — a [Chapter 27](27-dependency-management.md#chapter-27--dependency-management) decision — and its `DoNotOptimize` is the consume-the-result rule as a function.

> [!TIP]
> **Key principle:** "I profile the optimized build with its symbols and frame pointers kept, and I read a sampler for the mean and a counter or a trace for the tail — the complaint decides the instrument."

### Method: the tools are for testing hypotheses, not for browsing

The single biggest difference between fast and slow debugging is not tool knowledge. It is arriving with a hypothesis.

Chapter 24's predict-then-run drill is the training for exactly this. Before you step, say what you expect the value to be. A debugger session that begins "let me look around" takes an hour; one that begins "I believe `size_` is stale by the time `At` runs, and I will know within two steps" takes two minutes — and if the prediction was wrong, *that* is the finding, and it goes in the notes file.

Three questions that resolve most stuck moments before the debugger is even needed, in order — the ladder from Appendix C, sharpened:

1. **Which stage failed?** Preprocessor, compile, link, or run (Chapter 12). This decides which file you open.
2. **What changed?** If it worked an hour ago, the diff is the suspect list. `git bisect` mechanizes this and is criminally underused for "when did this break".
3. **Does the tool see it?** Rebuild with sanitizers before theorising about memory. Ten seconds of ASan beats an hour of reasoning, and it is right more often.

### In the wild: a crash inside a host application

- **Most of the stack is not yours.** A plug-in crash shows host frames above and below your code. Scroll to your frames — those are the ones with your source file names — and read outward from there. Chapter 13's advice about keeping a Debug build of your plug-in even against a symbol-less host is what makes this possible.
- **The first suspect is the SDK contract, not the language.** A crash on a handle you closed twice, a payload you freed with the wrong function, a callback firing into a dead object (Chapter 29) — the Chapter 16 four questions are a debugging checklist as much as a design one.
- **Release-only crashes are UB until proven otherwise.** The pattern "works in Debug, crashes in Release" is Chapter 3's signature. Do not start by suspecting the optimizer; start by suspecting uninitialized reads and lifetime bugs, and run the sanitizers.
- **Keep the crash.** A core file or minidump plus the exact binary and its symbols is a bug you can debug next week. Without the matching symbols it is a bug you get to reproduce from scratch.

### Pitfalls

- **Reading a sanitizer report from the top and stopping.** The first stack tells you where the program noticed, which is rarely where the mistake is. The mistake is usually in the second or third stack.
- **Believing an exit code over a report.** UBSan reports and continues by default; a green script can be hiding findings. Make it fatal.
- **Debugging a build that isn't the one running.** Chapter 13 names this as the top wasted-afternoon cause for plug-ins: breakpoints that never bind mean the loaded binary is not the one you just built. Check paths before you check logic.
- **Adding print statements to find a writer.** Use a watchpoint. Prints change timing, which is fatal for the threading bugs of Chapter 29 — the bug moves when you look at it.
- **Assuming a leak report will appear.** On macOS/arm64 it will not, whatever your code does.

### Try it

Everything here uses code you already have.

1. **Make the three shapes.** Break the Chapter 15 Buffer three ways — remove the null-out in the move constructor (double-free), read past the end via `At` with the assert disabled (overflow), and drop the `delete[]` in the destructor (leak). Build each under ASan and file the reports in your notes with a one-line translation. Count the stacks each time before reading them.
2. **Read one properly.** For the double-free, name the three stacks out loud: where it was touched, where it was freed, where it was born. Then find the offset line and work out which member the address corresponds to.
3. **Watch the evidence degrade.** Rebuild the same bug at `-O2` and diff the report against the `-O0` one. Note exactly which frames disappeared.
4. **Meet the UBSan trap.** Write a signed overflow, run it, and confirm the process exits 0 with a report on screen. Then run it again with `halt_on_error=1`, and again with `print_stacktrace=1`. Three runs, three different amounts of information from one bug.
5. **Use a watchpoint in anger.** Take any program with a value that changes more than once, break after it is constructed, set a watchpoint on one member, and let it stop at every write. Do this once and you will reach for it forever.
6. **Practise on a leak you cannot see** (Mac only): confirm that a deliberately leaking program under ASan reports nothing at all on your machine, and then decide how *you* will catch leaks — CI, a container, or platform tooling. Write the answer in your notes; it is a permanent property of your setup.
7. **Take Chapter 36's profile yourself.** Recreate perflab's broken `Tick` from its task card, build it with the harness at `-O2 -g -fno-omit-frame-pointer` and no sanitizers, sample it with your platform's profiler, and find `vector(const vector&)` under your own frame — the sibling the chapter's attachment showed. Then rebuild without `-fno-omit-frame-pointer` (on x86-64) and watch its parents vanish: that is the wrong-build profile the section above warned no tool would name.

### The symptom index

Real problems do not arrive labelled with the chapter that owns them; they arrive as something on a screen. The ticket chapters are already titled as their symptoms — this table routes the rest of the book the same way, from what you see to where it is taught.

| What you see | Where it is taught |
|---|---|
| A crash after `main` returns — `__cxa_finalize` or `atexit` in the stack | [Chapter 32](32-it-crashes-on-exit.md#chapter-32--it-crashes-on-exit) |
| Works in Debug, breaks in Release | [Chapter 3](03-stack-heap-and-undefined-behavior.md#chapter-3--stack-heap-and-undefined-behavior), [Chapter 13](13-toolchain-quick-reference.md#chapter-13--toolchain-quick-reference), and this chapter's `-O2` section |
| `undefined reference` / `Undefined symbols` / `LNK2019` | [Chapter 12](12-the-compilation-model.md#chapter-12--the-compilation-model), [Chapter 23](23-exercise-the-build-model-lab.md#chapter-23--exercise-the-build-model-lab); the sanitizer-runtime variant, [Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake) |
| `multiple definition` / `duplicate symbol` / `LNK2005` | [Chapter 12](12-the-compilation-model.md#chapter-12--the-compilation-model), [Chapter 23](23-exercise-the-build-model-lab.md#chapter-23--exercise-the-build-model-lab); two versions of one library, [Chapter 27](27-dependency-management.md#chapter-27--dependency-management) |
| A watched value goes wrong after a container grows | [Chapter 11](11-stl-containers-and-algorithms.md#chapter-11--stl-containers-algorithms-and-iterator-invalidation), [Chapter 21](21-exercise-iterator-invalidation.md#chapter-21--exercise-iterator-invalidation), [Chapter 33](33-here-is-the-report.md#chapter-33--here-is-the-report) |
| The host says objects are still live at shutdown | [Chapter 17](17-exercise-the-fakesdk.md#chapter-17--exercise-the-fakesdk), [Chapter 35](35-still-live-at-unload.md#chapter-35--still-live-at-unload) |
| A crash inside a callback, or after the callback's owner died | [Chapter 18](18-exercise-the-device-sdk.md#chapter-18--exercise-the-device-sdk), [Chapter 22](22-exercise-lambda-lifetimes.md#chapter-22--exercise-lambda-lifetimes), [Chapter 29](29-concurrency.md#chapter-29--concurrency) |
| Garbage — or mirrored — values decoded from a wire or a file | [Chapter 34](34-parse-this-capture.md#chapter-34--parse-this-capture) |
| Sanitizers green, values wrong | Finding 10 in [Chapter 25](25-findings-from-practice.md#chapter-25--findings-from-practice-a-living-log), [Chapter 34](34-parse-this-capture.md#chapter-34--parse-this-capture) |
| No leak report on a Mac that should have one | This chapter's leak section, and Finding 10 in [Chapter 25](25-findings-from-practice.md#chapter-25--findings-from-practice-a-living-log) |
| Non-ASCII text corrupts, or a string's length looks wrong | [Chapter 9](09-casts-conversions-and-strings.md#chapter-9--casts-conversions-and-strings), Recipe 17 in [Appendix F](F-rosetta-cookbook.md#appendix-f--the-rosetta-cookbook) |
| `-858993460` or `0xcccccccc` in a variable | [Chapter 3](03-stack-heap-and-undefined-behavior.md#chapter-3--stack-heap-and-undefined-behavior) |
| It broke when the library added a private member | [Chapter 27](27-dependency-management.md#chapter-27--dependency-management), [Chapter 30](30-authoring-an-abi-boundary.md#chapter-30--authoring-an-abi-boundary) |
| The process died in CI and the log is empty | [Chapter 28](28-testing.md#chapter-28--testing), Recipe 15 in [Appendix F](F-rosetta-cookbook.md#appendix-f--the-rosetta-cookbook) |
| Stutter, dropouts, or frame spikes with your code loaded — but the profile says you are cheap | [Chapter 36](36-the-host-stutters.md#chapter-36--the-host-stutters) |
| Text is fine for most customers and mojibake for the German and Russian ones | [Chapter 39](39-the-round-trip-home.md#chapter-39--the-round-trip-home), [Chapter 9](09-casts-conversions-and-strings.md#chapter-9--casts-conversions-and-strings) |
| A managed caller crashes inside your library, and the managed stack looks fine | [Chapter 39](39-the-round-trip-home.md#chapter-39--the-round-trip-home) |
| A crash report full of raw addresses, or a fault address just past null | [Chapter 37](37-no-repro-dump-attached.md#chapter-37--no-repro-dump-attached) |
| A client of your plug-in shows a spinner forever — or the host freezes with one thread parked in a wait | [Chapter 38](38-the-bridge-out.md#chapter-38--the-bridge-out) |
| A profile in which every function is called from nowhere, or that is a column of addresses | This chapter's profiler section, [Chapter 37](37-no-repro-dump-attached.md#chapter-37--no-repro-dump-attached) |
| Runs from the build tree, dies before `main` installed or on the customer's machine — `dyld: Library not loaded`, `error while loading shared libraries`, *X.dll was not found* | [Appendix J](J-cmake-catalogue.md#appendix-j--the-cmake-catalogue)'s runtime-delivery entry; the sanitizer-runtime variant in [Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake) |
| A crash on *entry* to a function, before its first line — `stack-overflow`, or a bare `SEGV`/`BUS` "on unknown address": one stack, and no allocation site | [Chapter 3](03-stack-heap-and-undefined-behavior.md#chapter-3--stack-heap-and-undefined-behavior), Recipe 34 in [Appendix F](F-rosetta-cookbook.md#appendix-f--the-rosetta-cookbook) |
| A saved file is empty or truncated after a crash — the customer's preferences are gone (after a power cut, the same symptom needs the `fsync` that recipe names) | Recipe 38 in [Appendix F](F-rosetta-cookbook.md#appendix-f--the-rosetta-cookbook) |
| A file watcher fires once and then never again — or misses a second save made within the same second | Recipe 40 in [Appendix F](F-rosetta-cookbook.md#appendix-f--the-rosetta-cookbook): an editor saved by rename (Recipe 38), or a timestamp at the filesystem's resolution |
| `curl_easy_perform` returned `CURLE_OK` and the body is an HTML error page — the JSON parse fails and "the server is flaky" | Recipe 41 in [Appendix F](F-rosetta-cookbook.md#appendix-f--the-rosetta-cookbook): two verdicts, the transport's and the server's, and only one was checked |
| `sqlite3_close` returns `SQLITE_BUSY` at shutdown, or the database file stays locked after the plug-in unloads | Recipe 42 in [Appendix F](F-rosetta-cookbook.md#appendix-f--the-rosetta-cookbook): a statement nobody finalized — [Chapter 35](35-still-live-at-unload.md#chapter-35--still-live-at-unload)'s still-live-at-unload, one library over |

**When more than one row fits, and you have no stack yet.** *It crashes at shutdown or unload* is the common case: it matches the first row, the still-live-at-shutdown row and the callback row equally well, and the ticket that brought it rarely says which. Two questions settle the order, and both are answerable from your own source without reproducing anything. **Does a global or function-local static own something on the failing path?** If so start at [Chapter 32](32-it-crashes-on-exit.md#chapter-32--it-crashes-on-exit) — your source names the statics, but not the order they are destroyed in, which is that chapter's whole subject. **Does anything the host owns still hold a pointer to you — a registered callback, an observer, a refcount?** Then [Chapter 29](29-concurrency.md#chapter-29--concurrency) for the threaded case, [Chapter 35](35-still-live-at-unload.md#chapter-35--still-live-at-unload) for the refcounted one. If both are true, rule out the static first: it is the cheaper of the two, because it needs no repro to investigate.

---


<!-- nav:begin -->
[← Chapter 30 — Authoring an ABI Boundary](30-authoring-an-abi-boundary.md) · [Contents](README.md) · [Chapter 32 — It Crashes on Exit →](32-it-crashes-on-exit.md)
<!-- nav:end -->
