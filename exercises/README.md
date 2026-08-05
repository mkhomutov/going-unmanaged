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
| [The Test Lab](testlab/TASK.md) | 28 | a test framework from scratch, and testing ownership | ~60 min | the files themselves: [tiny_test.h](testlab/tiny_test.h), [buffer_test.cpp](testlab/buffer_test.cpp) |
| [The Threaded Callback](threadlab/TASK.md) | 29 | callback lifetime across a thread boundary, under TSan | ~2 h | [device_threaded_solution.cpp](../solutions/device_threaded_solution.cpp) |
| [The ABI Lab](abilab/TASK.md) | 30 | publishing a boundary instead of consuming one | ~2 h | the files themselves: [Widget.h](abilab/Widget.h), [IScorer.h](abilab/IScorer.h), [engine.h](abilab/engine.h) + their implementations and callers |
| [The Exit Crash](exitlab/TASK.md) | 32 | a ticket, not a task: static init/destruction order across TUs | ~60 min | the files themselves: the fixed [logger.cpp](exitlab/logger.cpp) + [audit.cpp](exitlab/audit.cpp), green in both link orders |
| [Here Is the Report](reportlab/TASK.md) | 33 | a ticket with the sanitizer report attached: locate the bug from the report alone | ~60 min | the files themselves: the fixed [main.cpp](reportlab/main.cpp) + [registry.h](reportlab/registry.h), green at 0 and 100 hot-plugs |
| [Parse This Capture](capturelab/TASK.md) | 34 | a ticket with the capture attached: padding, byte order, and the overlay that was never legal | ~90 min | the files themselves: the fixed [wire.cpp](capturelab/wire.cpp) + [main.cpp](capturelab/main.cpp), asserted against the hand decode |
| [Still Live at Unload](comlab/TASK.md) | 35 | a ticket against an upgraded SDK: manual refcounting, and the wrapper that ends it | ~90 min | the files themselves: [ref.h](comlab/ref.h) + the fixed [main.cpp](comlab/main.cpp), balanced under both judges |
| [The Host Stutters](perflab/TASK.md) | 36 | a ticket with the profile attached: cost evidence, read without being lied to by an average | ~60 min | the files themselves: the fixed [meter.cpp](perflab/meter.cpp) + the counting [main.cpp](perflab/main.cpp), zero allocations at two session lengths |
| [No Repro, Dump Attached](dumplab/TASK.md) | 37 | a ticket with the crash report attached: post-mortem from the paperwork alone | ~60 min | the files themselves: the fixed [session.cpp](dumplab/session.cpp) + [main.cpp](dumplab/main.cpp), green under both device configurations |

Chapter 24 (the practice plan) sequences everything above the test lab — the
nine Part V exercises plus the Bestiary reading — into a one-week schedule; the
rows below it belong to Part VI and come later. The threaded callback assumes you
have already done the Device SDK lab cold — it is that lab again with a driver
thread in front of it — and the ABI lab assumes both SDK labs, since it asks you
to publish the shapes those two taught you to consume.

`cookbook/` is the odd one out: not an exercise at all, but Appendix F's
recipe listings, compiled and asserted by `build_all.sh` so the cookbook
cannot rot (its README has the sync rule). Nothing there to attempt cold.

Nine directories hold their reference in the open, rather than behind a
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
`buildlab/` does double duty: it is Chapter 23's lab, and Chapter 26 builds that
same trio with CMake, so the reference
[`CMakeLists.txt`](buildlab/CMakeLists.txt) lives there. `testlab/` holds
Chapter 28's harness and suite, which the chapter prints in full anyway, and
`abilab/` holds Chapter 30's three worked boundaries for the same reason. All
three are kept green by `scripts/build_all.sh`. Write your own first — in a
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
```

Several `.cpp` files build as one binary, in the order written — which is also
the link order, the thing Chapter 32's two-order test turns on. The first
argument that is not a `.cpp` file (after the optional vendor name) starts the
run arguments. `scripts\check.ps1` accepts the same shapes.

That last one is for the threaded lab, and it is a *second* run rather than a
replacement: ThreadSanitizer cannot be combined with AddressSanitizer, so
threaded code needs both builds to be checked at all (Chapter 29).

Vendor code (`fakesdk/Fake*`, `fakedevice/Fake*`, `comlab/FakeSDK2.*`) is
read-only: read it, compile it, link it — never edit it. `threadlab/` has
none of its own: it links `fakedevice/`'s from where it lives, which is what
a second lab against the same SDK should do. `comlab/`'s is deliberately a
*new* vendor drop rather than an edit to `fakesdk/` — the vendor shipped
2.0; version 1.x did not change, and neither do its files.
