## Chapter 31 — Reading What the Tools Tell You

Chapter 24's practice plan tells you, on Day 2, to break the Buffer three ways and "read its reports until they make sense". The book has never shown you one. This chapter is that omission repaired, plus the debugger skills that differ most from the C# experience.

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

No stack. A column number, which ASan never gives you and which points precisely at the operator. And — the part that catches people — **the program then carried on and exited 0**. UBSan's default is report-and-continue, so a script that only checks the exit code will call this run a success.

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

---


<!-- nav:begin -->
[← Chapter 30 — Authoring an ABI Boundary](30-authoring-an-abi-boundary.md) · [Contents](README.md) · [Appendix A — Fundamentals Refresher →](A-fundamentals-refresher.md)
<!-- nav:end -->
