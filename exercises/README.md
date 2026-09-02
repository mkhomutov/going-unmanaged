# Exercises — the index

Every exercise is stated in full in the book; each directory here holds a task
card (`TASK.md`) — and, for the SDK labs, the vendor code — so you can
attempt everything cold, without the book's reference solution on the next screen.

The rule that makes it work (Chapter 24): **do the exercises cold** — compiler,
debugger, sanitizer, and offline docs as your only feedback loops. Read the
chapter's reference solution and pitfalls only *after* your own attempt.

| Exercise | Chapter | Trains | Time | Reference solution |
|---|---|---|---|---|
| [The Lifetime Tracer](tracer/TASK.md) | 14 | every special member it defines, observed live | ~60 min | [tracer.cpp](../solutions/tracer.cpp) |
| [The Buffer](buffer/TASK.md) | 15 | the Rule of Five on a raw pointer | ~90 min | [Buffer.h](../solutions/Buffer.h) + [buffer.cpp](../solutions/buffer.cpp) |
| *(reading)* The SDK Bestiary | 16 | the five shapes vendor APIs take — read before the SDK labs | ~20 min | — |
| [The FakeSDK](fakesdk/TASK.md) | 17 | RAII + error codes (desktop-plugin style) | ~90 min | [fakesdk_solution.cpp](../solutions/fakesdk_solution.cpp) |
| [The Device SDK](fakedevice/TASK.md) | 18 | opaque handles + C callbacks (peripheral style) | ~2 h | [device_solution.cpp](../solutions/device_solution.cpp) |
| [The Word Counter](words/TASK.md) | 19 | STL fluency end to end | ~60 min | [words.cpp](../solutions/words.cpp) |
| [Slicing and Polymorphism](slicing/TASK.md) | 20 | slicing, virtual dispatch, virtual destructors | ~45 min | [shapes.cpp](../solutions/shapes.cpp) |
| [Iterator Invalidation](invalidation/TASK.md) | 21 | the invalidation trap, under ASan | ~45 min | [invalid.cpp](../solutions/invalid.cpp) |
| [Lambda Lifetimes](lambdas/TASK.md) | 22 | captures vs the missing GC | ~45 min | [lambdas.cpp](../solutions/lambdas.cpp) |
| [The Build-Model Lab](buildlab/TASK.md) | 23 | error-stage triage: preprocessor / compile / link | ~45 min | none — your notes are the artifact |
| [The Dependency Lab](deplab/TASK.md) | 27 | one library consumed three ways, and a version pin proved rather than demonstrated | ~90 min | the files themselves: [mathlib/CMakeLists.txt](deplab/mathlib/CMakeLists.txt) + the three `consume-*/CMakeLists.txt`, built at two tags |
| [The Test Lab](testlab/TASK.md) | 28 | a test framework from scratch, and testing ownership | ~60 min | the files themselves: [tiny_test.h](testlab/tiny_test.h), [buffer_test.cpp](testlab/buffer_test.cpp) |
| [The Threaded Callback](threadlab/TASK.md) | 29 | callback lifetime across a thread boundary, under TSan | ~2 h | [device_threaded_solution.cpp](../solutions/device_threaded_solution.cpp) |
| [The ABI Lab](abilab/TASK.md) | 30 | publishing a boundary instead of consuming one | ~2 h | the files themselves: [Widget.h](abilab/Widget.h), [IScorer.h](abilab/IScorer.h), [engine.h](abilab/engine.h) + their implementations and callers |
| [The Exit Crash](exitlab/TASK.md) | 32 | a ticket, not a task: static init/destruction order across TUs | ~60 min | the files themselves: the fixed [logger.cpp](exitlab/logger.cpp) + [audit.cpp](exitlab/audit.cpp), green in both link orders |
| [Here Is the Report](reportlab/TASK.md) | 33 | a ticket with the sanitizer report attached: locate the bug from the report alone | ~60 min | the files themselves: the fixed [main.cpp](reportlab/main.cpp) + [registry.h](reportlab/registry.h), green at 0 and 100 hot-plugs |
| [Parse This Capture](capturelab/TASK.md) | 34 | a ticket with the capture attached: padding, byte order, and the overlay that was never legal | ~90 min | the files themselves: the fixed [wire.cpp](capturelab/wire.cpp) + [main.cpp](capturelab/main.cpp), asserted against the hand decode |
| [Still Live at Unload](comlab/TASK.md) | 35 | a ticket against an upgraded SDK: manual refcounting, and the wrapper that ends it | ~90 min | the files themselves: [ref.h](comlab/ref.h) + the fixed [main.cpp](comlab/main.cpp), balanced under both judges |
| [The Host Stutters](perflab/TASK.md) | 36 | a ticket with the profile attached: cost evidence, read without being lied to by an average | ~60 min | the files themselves: the fixed [meter.cpp](perflab/meter.cpp) + the counting [main.cpp](perflab/main.cpp), zero allocations at two session lengths |
| [No Repro, Dump Attached](dumplab/TASK.md) | 37 | a ticket with the crash report attached: post-mortem from the paperwork alone | ~60 min | the files themselves: the fixed [session.cpp](dumplab/session.cpp) + [main.cpp](dumplab/main.cpp), green under both device configurations |
| [The Bridge Lab](bridgelab/TASK.md) | 38 | serving a foreign client: the main-thread queue, refusing work, the bounded wait | ~2 h | the files themselves: [main_thread_queue.h](bridgelab/main_thread_queue.h), [host.h](bridgelab/host.h), [bridge_core.h](bridgelab/bridge_core.h) + the judging [main.cpp](bridgelab/main.cpp), green under both sanitizer builds |
| [The Interop Lab](interoplab/TASK.md) | 39 | publishing a C surface a managed caller binds by hand | ~2 h | the files themselves: [plugin.h](interoplab/plugin.h) + [plugin.cpp](interoplab/plugin.cpp), judged by [main.cpp](interoplab/main.cpp) through the boundary header alone |
| [The Const Lab](constlab/TASK.md) | Appendix I | const as one subject, judged by five builds that must fail | ~45 min | the files themselves: [counter.h](constlab/counter.h) + [main.cpp](constlab/main.cpp), plus five builds that must be refused |

Chapter 24 (the practice plan) sequences everything above the dependency lab —
the nine Part V exercises plus the Bestiary reading — into a one-week schedule;
that row and the chapter rows below it belong to Part VI and come later, and the const lab sits last because its home is an appendix and nothing sequences it. The threaded
callback assumes you have already done the Device SDK lab cold — it is that lab
again with a driver thread in front of it — and the ABI lab assumes both SDK
labs, since it asks you to publish the shapes those two taught you to consume.

`cookbook/` and `choosing/` are the odd ones out: not exercises at all, but
appendix listings compiled and checked by `build_all.sh` so those pages
cannot rot — Appendix F's recipes and Appendix H's cost measurements
respectively (each README has the sync rule, and `choosing/`'s is enforced
in both directions). Nothing in either to attempt cold.

Twelve directories hold their reference in the open, rather than behind a
fold. `exitlab/`, `reportlab/`, `capturelab/`, `comlab/`, `perflab/` and
`dumplab/` are
the ticket-shaped ones: each TASK.md carries the broken code to work from
plus the ticket's attached evidence (reportlab's sanitizer report,
capturelab's bus capture and ICD table, comlab's migration notes,
perflab's profile and engine log, dumplab's crash report), and the files
beside
it are the fixed state — built in two link orders for `exitlab/`, run at
two hot-plug counts for `reportlab/`, asserted against a hand-decoded
capture with a deliberately unaligned frame for `capturelab/`, held to
two judges at once for `comlab/` (the vendor's live-object counter and the
sanitizers, one per direction of a refcount mistake), asserted
allocation-free at two session lengths for `perflab/` (a counter, because
the sanitizers are silent on an accidental copy and a timing would measure
the runner), and run under both device configurations for `dumplab/` (the
crash lived only in the one the bench never had) — because each fix's
claim is exactly what one build cannot prove.
`bridgelab/` holds Chapter 38's worked bridge core under the same card
discipline as the tickets — the three broken shapes are quoted in its
TASK.md and the chapter, identically — and its harness doubles as the
lab's judge: no wait in it is unbounded, because two of the breaks are
hangs and a hang cannot fail a script, only stop it.
`constlab/` is Appendix I's, and the only lab whose judge asserts a build
*fails*: `counter.h` and `main.cpp` build and run clean, and five const
violations behind `-D` guards must each be refused with a diagnostic that
names const — a const mistake never reaches a binary, so a harness that only
ever compiles things could not check the one thing that appendix is about.
`buildlab/` does double duty: it is Chapter 23's lab, and Chapter 26 builds that
same trio with CMake, so the reference
[`CMakeLists.txt`](buildlab/CMakeLists.txt) lives there. `interoplab/` is Chapter 39's, and the
only one whose caller is imaginary: `main.cpp` plays the marshaller, because
every mistake that chapter is about is observable from the native side and no
CLR is needed to see it. `deplab/` is Chapter
27's, and the only lab whose subject is entirely build description: one
`app/main.cpp` consumed three ways — vendored, fetched, and found as an
installed config package — so the three `consume-*/CMakeLists.txt` are the
whole lesson and the app cannot tell them apart. Its judges are worth knowing
before you read it: every consumer is grepped for an include path it must not
name, the three must share one `add_executable` and one `target_link_libraries`
between them, and the fetched one is built at two tags, each of which must
report its own version — building once proves the mechanism runs, only building
twice proves the *pin* chose the version, and asking merely that the two
outputs differ would let through a pin that chose the wrong commit. `testlab/`
holds Chapter 28's harness and suite, which the chapter prints in full anyway,
and `abilab/` holds Chapter 30's three worked boundaries for the same reason.
All five are kept green by `scripts/build_all.sh`. Write your own first — in a
directory of your own, without reading these.

## Building your attempt

`scripts/check.sh` builds (and runs) your code with the handbook's canonical
flags — strict warnings plus Address/UB sanitizers. Run it from wherever your
attempt lives (from inside an exercise directory that is
`../../scripts/check.sh`); your file and any run arguments resolve relative to
where you stand:

```bash
scripts/check.sh path/to/your.cpp                 # plain exercises
scripts/check.sh your.cpp fakesdk                 # links the FakeSDK vendor code
scripts/check.sh your.cpp fakedevice              # links the FakeDevice vendor code
scripts/check.sh your.cpp comlab                  # links the FakeSDK 2.0 vendor code (Ch 35)
scripts/check.sh registry.cpp main.cpp 100        # several TUs (ticket labs) + a run arg
STD=c++20 scripts/check.sh your.cpp file.txt      # C++20 + args passed to the run
SAN=thread scripts/check.sh your.cpp fakedevice   # ThreadSanitizer instead
SAN=none scripts/check.sh a.cpp b.cpp             # no sanitizer at all
SAN=none OPT=2 scripts/check.sh a.cpp b.cpp       # ...and optimised
```

`SAN=thread` is for the threaded lab, and it is a *second* run rather than a
replacement: ThreadSanitizer cannot be combined with AddressSanitizer, so
threaded code needs both builds to be checked at all (Chapter 29).

`SAN=none` is the one that looks like cheating and is not. A handful of
exercises are about what the tools *do not* catch — Chapter 27's ODR diamond
is the clearest — and their first step is to watch a wrong program run to
completion with nothing warning you at any point, which a build carrying the
sanitizers cannot show. `OPT` (default `0`) goes with it for the rebuilds that
ask you to watch a symptom change under optimisation. The script says so in
its own output: with `SAN=none` a clean exit is reported as proving nothing,
because it does not.

Several `.cpp` files build as one binary, in the order written — which is also
the link order, the thing Chapter 32's two-order test turns on. The first
argument that is not a `.cpp` file (after the optional vendor name) starts the
run arguments. `scripts\check.ps1` accepts the same shapes.

Vendor code (`fakesdk/Fake*`, `fakedevice/Fake*`, `comlab/FakeSDK2.*`) is
read-only: read it, compile it, link it — never edit it. `threadlab/` has
none of its own: it links `fakedevice/`'s from where it lives, which is what
a second lab against the same SDK should do. `comlab/`'s is deliberately a
*new* vendor drop rather than an edit to `fakesdk/` — the vendor shipped
2.0; version 1.x did not change, and neither do its files.
