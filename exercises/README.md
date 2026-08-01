# Exercises — the index

Every exercise is stated in full in the book; each directory here holds a task
card (`TASK.md`) — and, for the two SDK labs, the vendor code — so you can
attempt everything cold, without the book's reference solution on the next screen.

The rule that makes it work (Chapter 24): **do the exercises cold** — compiler,
debugger, sanitizer, and offline docs as your only feedback loops. Read the
chapter's reference solution and pitfalls only *after* your own attempt.

| Exercise | Chapter | Trains | Time | Reference solution |
|---|---|---|---|---|
| [The Lifetime Tracer](tracer/TASK.md) | 14 | all six special members, observed live | — | [tracer.cpp](../solutions/tracer.cpp) |
| [The Buffer](buffer/TASK.md) | 15 | the Rule of Five on a raw pointer | — | [buffer.cpp](../solutions/buffer.cpp) |
| *(reading)* The SDK Bestiary | 16 | the five shapes vendor APIs take — read before the SDK labs | — | — |
| [The FakeSDK](fakesdk/TASK.md) | 17 | RAII + error codes (desktop-plugin style) | ~90 min | [fakesdk_solution.cpp](../solutions/fakesdk_solution.cpp) |
| [The Device SDK](fakedevice/TASK.md) | 18 | opaque handles + C callbacks (peripheral style) | ~2 h | [device_solution.cpp](../solutions/device_solution.cpp) |
| [The Word Counter](words/TASK.md) | 19 | STL fluency end to end | ~60 min | [words.cpp](../solutions/words.cpp) |
| [Slicing and Polymorphism](slicing/TASK.md) | 20 | slicing, virtual dispatch, virtual destructors | ~45 min | [shapes.cpp](../solutions/shapes.cpp) |
| [Iterator Invalidation](invalidation/TASK.md) | 21 | the invalidation trap, under ASan | ~45 min | [invalid.cpp](../solutions/invalid.cpp) |
| [Lambda Lifetimes](lambdas/TASK.md) | 22 | captures vs the missing GC | ~45 min | [lambdas.cpp](../solutions/lambdas.cpp) |
| [The Build-Model Lab](buildlab/TASK.md) | 23 | error-stage triage: preprocessor / compile / link | ~45 min | none — your notes are the artifact |

Chapter 24 (the practice plan) sequences these into a one-week schedule.

One directory does double duty: `buildlab/` is Chapter 23's lab, and Chapter 26
builds that same trio with CMake. Its reference
[`CMakeLists.txt`](buildlab/CMakeLists.txt) lives there, kept green by
`scripts/build_all.sh` — so it is the one reference solution in this index you
can reach without opening a fold. Write your own first.

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
STD=c++20 scripts/check.sh your.cpp file.txt      # C++20 + args passed to the run
```

Vendor code (`fakesdk/Fake*`, `fakedevice/Fake*`) is read-only: read it, compile
it, link it — never edit it.
