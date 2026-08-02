# Exercise: The ABI Lab (publish a boundary, three ways)

Full statement and the reasoning behind every constraint: **Chapter 30** of the
book, *Try it*. Do it COLD on your own Buffer first — the files here are the
chapter's worked boundaries, and they are the comparison afterwards, not the
starting point. ~2 h.

*Trains: Chapter 30 (API vs ABI, the one rule, PIMPL, a pure-virtual interface
with a factory, an `extern "C"` façade); Chapter 16's Bestiary from the other
side of the table — you have consumed these shapes since Chapter 17, and this is
where you derive them; Chapter 5's virtual-destructor rule, met again as a
design decision rather than a warning.*

## The task

Take the Chapter 15 Buffer — it owns a resource and has real state, which is
what makes it worth publishing — and publish it three ways.

1. **Break it on purpose first.** Members in the header, a caller compiled
   against it, then add a *private* member and rebuild only the library.
   Confirm `sizeof` changed; run it; rebuild both sides under
   `-fsanitize=address` to see the overflow, and note that instrumenting only
   the library hides it.
2. **PIMPL it.** State behind `struct Impl`. Omit the destructor declaration
   first, on purpose, so you meet the incomplete-type error and recognize it
   forever. Then prove stability: compile the caller once, change `Impl`
   substantially, relink without recompiling, watch `sizeof` stay at one
   pointer.
3. **Interface it.** An abstract `IBuffer`, a factory, a `Destroy`. Then break
   the vtable rule deliberately — insert a pure-virtual method at the *top*,
   rebuild only the library, and watch the caller call the wrong function with
   no diagnostic anywhere.
4. **Wrap it in C.** An opaque `BufferHandle`, create/at/size/destroy, error
   codes, a `catch (...)` in every entry point. Then put your header beside
   [`exercises/fakedevice/FakeDevice.h`](../fakedevice/FakeDevice.h) and see how
   close you landed without trying.
5. **Version it.** An options struct with a leading size field, then a field
   added in "version two", handled correctly at runtime for both callers.

Steps 1, the second half of 3, and the relink in 2 exist to *fail* or to need a
stale binary, so they stay out of any script you wire up — as they stay out of
this repository's.

## The files here

The chapter's three worked boundaries, checked in so you have something to
compare against — each a boundary header, an implementation, and a caller that
sees only the header:

| Technique | Boundary | Implementation | Caller |
|---|---|---|---|
| PIMPL | `Widget.h` | `Widget.cpp` | `widget_demo.cpp` |
| Interface + factory | `IScorer.h` | `scorer.cpp` | `scorer_demo.cpp` |
| `extern "C"` façade | `engine.h` | `engine.cpp` | `engine_demo.cpp` |

Each demo is a *separate binary of two translation units*, and that separation
is the subject matter rather than a build detail: the caller is compiled against
the header alone, so what the header does not say is genuinely unavailable to
it. `Widget.h`, `Widget.cpp`, `IScorer.h` and `engine.h` are Chapter 30's
listings; `scorer.cpp` and the rest of `engine.cpp` complete what the chapter
excerpts. Editing any of them means editing Chapter 30 in the same commit.

The demos assert what the chapter claims — `sizeof(Widget) == sizeof(void*)`,
that a protected destructor makes `delete scorer` a compile error, that every
`Engine_*` return code is what the header documents — and
`scripts/build_all.sh` builds and runs all three under the canonical flags on
every push.

## Build

Two translation units per binary, so this is a compiler line rather than
`check.sh` (which builds one file):

```
g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -g \
    Widget.cpp widget_demo.cpp -o widget
```

...and the same shape for `scorer.cpp scorer_demo.cpp` and
`engine.cpp engine_demo.cpp`. `scripts/check.sh` still fits the parts of your
own attempt that are one file — a single-TU sketch of a façade, say — and
`scripts/build_all.sh` builds all three of these.

## Done means

- **The caller compiles against the header alone.** If your demo needs the
  implementation's header to build, the boundary is not a boundary yet.
- **Nothing whose layout your compiler chose is in an exported signature** in
  step 4 — no `std::string`, no `std::vector`, no exceptions crossing.
- **Whoever allocates, frees.** A `Destroy`/`Close` function, never a `delete`
  the caller writes.
- **You met the incomplete-type error on purpose** (step 2) and can now say,
  from memory, why the destructor has to be defined in the .cpp.
- **You saw the wrong function get called** (step 3) with no error from the
  compiler, the linker, or the runtime. That five-minute experiment is why
  `IThing2` exists.
