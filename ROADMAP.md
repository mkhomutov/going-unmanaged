# Roadmap — what the handbook is still missing

This file is the standing answer to "what should I write?". It is a list of
**gaps found by reading the book against the job it claims to prepare you
for**, ranked by how much they cost a reader who hits them unprepared. Each
entry says what is missing, the evidence in the current text, and what a
contribution would concretely look like.

Nothing here is a promise or a schedule. Items get done when someone does the
work and writes it up; an item may also be closed as "deliberately out of
scope" — that decision belongs in an issue, and the entry moves to
[Deliberately out of scope](#deliberately-out-of-scope) below with the reason.

Delivered items stay where they are, marked **DONE** with a pointer to the
chapter that closed them. Item numbers get cited in issues and commit
messages, so they never shift.

**Everything on this list appends.** New chapters go at the end (Chapter 27
onward, in whatever order they land); new appendices continue from E. No item
here requires renumbering, so every one of them is a MINOR release. If you
think an item genuinely belongs *inside* an existing part, open an issue
first — see the numbering rules in [CONTRIBUTING.md](CONTRIBUTING.md).

Before starting a large item, open an issue saying so, so two people don't
write the same chapter twice.

---

## Tier 1 — the load-bearing gaps

These are places where the book's own scope, or its own promises, are not yet
met. They cost the reader in week one.

### 1. Build systems — DONE

**Delivered:** Chapter 26 — *Build Systems and CMake*, opening the new Part VI.
The gap was that Part IV is called *The Build and the Toolchain* and stopped at
raw `clang++` and `cl` invocations, leaving a reader who meets a
`CMakeLists.txt` on day one with nothing.

The chapter covers why a build system exists at all (header-dependency
tracking — the answer to Chapter 23's breakage 7), CMake as a *generator*
rather than a build tool with `.csproj`/MSBuild as the spine of the
comparison, a worked CMakeLists for the `exercises/buildlab/` Greeter trio,
targets and PRIVATE/PUBLIC/INTERFACE propagation, Debug vs Release, the
handbook's sanitizer flags behind a `-DGREETER_SANITIZE=ON` option,
`find_package` for SDK config packages, and IDE-native project files as the
alternative reality many SDK shops actually live in.

**Still open from this item:** the chapter's CMake snippets are the only code
in the book that `scripts/build_all.sh` does not verify. A reference
`CMakeLists.txt` for `exercises/buildlab/` plus a CI step that runs it when
cmake is present would put them under the same protection as everything else.
Small, self-contained, and a good first contribution.

### 2. Dependency management

**Missing:** how a third-party library gets into your build. No mention of
vcpkg, Conan, git submodules, or vendoring source into the repo.

**Evidence:** zero hits across the book. "There is no NuGet" is one of the
genuine shocks of the transition, and "how do I add a library" is a week-one
question the handbook currently cannot answer.

**A contribution looks like:** a section (natural companion to item 1, and
could share its chapter) covering the four real strategies — package manager,
submodule, vendored source, system-installed — with the honest note that
which one you use is usually decided by the team before you arrive, and that
vendored source is far more common in SDK work than newcomers expect.

Keep the book's own rule intact: **solutions use the standard library only**.
This chapter teaches the landscape; it does not add a dependency to the repo.

### 3. Testing

**Missing:** unit testing, entirely. No gtest, Catch2, or doctest.

**Evidence:** zero hits. For an exercise-driven handbook written for someone
arriving from a world where xUnit or NUnit is table stakes, the feedback loop
on offer is currently "compile, run, read stdout". The book's own practice
plan leans on prediction and observation, which is good pedagogy — but it
should be a deliberate choice the reader makes, not an absence they discover.

**A contribution looks like:** a chapter using one single-header framework
(doctest or Catch2 — pick for zero-install, since the reader may be on a
locked-down work machine), retrofitting tests onto an exercise the reader has
already written — the Buffer of Chapter 15 is the obvious candidate, because
its Rule of Five behaviour is exactly what assertions capture well. Wire the
test binary into `scripts/build_all.sh` in the same PR so the invariant keeps
holding.

This is the one item on the list that touches a ground rule, so be deliberate
about it: **solutions use the standard library only** (CONTRIBUTING.md), and a
vendored framework header is the only third-party code the repo would carry.
The resolution already exists in `build_all.sh` — `exercises/buildlab/` is
built and run there while being scaffolding rather than a solution. A test
framework is the same kind of thing: the header belongs under `exercises/`,
`solutions/` stays stdlib-only and independently buildable without it, and the
test binary is a third category the script builds to keep it green. Expect one
practical snag — a single-header framework will not survive `-Wall -Wextra`
silently, so include it with `-isystem` and keep the strict flags meaning what
they mean for our code. Open an issue before starting; this one changes a rule
rather than only adding a chapter.

### 4. Concurrency

**Missing:** threading, as a subject. `std::thread`, `std::mutex`,
`std::atomic`, `std::future`, `condition_variable`.

**Evidence:** the book promises this repeatedly and never delivers.
Chapter 16 tells you to ask *"what thread calls me back?"*; Chapter 18 answers
that a missing answer means "a thread that isn't yours", describes the
unregister-then-join problem in its pitfalls, and asks the reader in a stretch
goal to make a callback race-free with a mutex. Appendix D defers to Williams' book.
Meanwhile `condition_variable` appears zero times in the text and `atomic`
once. The reader is told the hazard matters, shown where it bites, and then
handed no vocabulary.

**A contribution looks like:** a chapter mapping the C# concurrency model onto
the C++ one — `Task`/`async`/`await` versus `std::thread`, `std::jthread`, and
`std::future`; `lock` versus `std::lock_guard` and `std::unique_lock` (RAII
again, which the reader already owns from Chapter 1); `Interlocked` versus
`std::atomic`; and the thing C# mostly hides — that a data race is undefined
behaviour, not a wrong answer. Pair it with the threaded-callback lab already
listed under [Known gaps carried over](#known-gaps-carried-over).

---

## Tier 2 — high value, smaller

### 5. A real debugging chapter

**Missing:** an actual debugging session on the page.

**Evidence:** Chapter 13 covers attach-to-process, which is the right
SDK-specific advice. But Day 2 of the practice plan instructs the reader to
"read its reports until they make sense" — and the book never shows an
AddressSanitizer report. The reader is asked to build fluency in a format they
have never seen printed.

**A contribution looks like:** one annotated ASan heap-use-after-free report,
walked line by line (the allocation stack, the free stack, the access stack,
and how to read which is which), plus the debugger basics that differ from
Visual Studio's C# experience: reading a native call stack, watchpoints, and
inspecting memory. Cheap to write, and it fulfils an instruction the book
already gives.

### 6. Authoring an ABI boundary

**Missing:** how to *ship* a stable interface, as opposed to consuming one.

**Evidence:** `ABI` appears exactly once — a Chapter 13 warning that a
mismatched Visual Studio toolset "produces baffling link and load errors" —
and the term is never unpacked. Chapter 12 describes the thing without naming
it ("binary compatibility across DLL boundaries is a real C++ concern C#
assemblies never have"), and PIMPL appears nowhere. Chapter 16's Bestiary
teaches the five shapes vendor APIs take and how to wrap them — all from the
consumer side. But the reader's actual job is to build a plug-in or a library
that someone else loads, which makes them the author of one of those
boundaries.

**A contribution looks like:** a chapter on the three ways to draw a stable
line — PIMPL, pure-virtual interface, `extern "C"` façade — and the rule that
explains all three: nothing whose layout your compiler chose may cross the
boundary, which is why `std::string` and `std::vector` must never appear in an
exported signature. Connects directly to Chapter 12's DLL material.

### 7. Byte-level protocol work

**Missing:** struct layout, padding, packing, endianness, alignment.

**Evidence:** zero hits for alignment, endianness, and `union`; one for
`sizeof`. Chapter 18 is a *device* lab. Parsing bytes off a wire or a bus is
what device work is, and struct padding plus byte order are the two canonical
bugs — both invisible from C#, where layout is the runtime's problem.

**A contribution looks like:** a short chapter, or a stretch goal on the
FakeDevice exercise, where a packed header struct is parsed from a byte
buffer: what `sizeof` reports and why, what a packing pragma does and costs,
host versus network byte order, and why `reinterpret_cast` over a buffer is a
strict-aliasing trap rather than a clever shortcut.

### 8. Consolidated const-correctness

**Missing:** const as one coherent subject.

**Evidence:** it is taught in fragments — const members in Chapter 4, trailing
const and `const char*` versus `char* const` in Appendix A and B, `const auto&`
as a reflex in Chapter 10, const overloads in the Buffer chapter. Every piece
is present and no page assembles them. C# has no equivalent concept at all
(`readonly` is not it), and const shows up in every C++ code review the reader
will ever sit in.

**A contribution looks like:** a chapter that gathers the existing fragments
and adds what is missing — const as an interface contract, const member
functions, `mutable`, and why const-correctness is retrofitted with pain but
free if it is there from the start. Cross-reference rather than duplicate;
the fragments stay where they are.

---

## Tier 3 — distinctive to this handbook

Material no general C++ book would carry, which is precisely why it belongs
here.

### 9. Going back the other way — C++/C# interop

**Missing:** P/Invoke, marshalling, and the round trip home.

**Evidence:** one passing mention of P/Invoke, nothing on marshalling. A
developer with 17 years of C# behind them, now working against a native SDK,
very plausibly ends up exposing that native code to C# — a test harness, an
internal tool, a UI. This handbook is the one place that reader would look.

**A contribution looks like:** a chapter on the `extern "C"` surface (which
connects to item 6), what marshals cleanly and what does not, string
conversion at the boundary, who owns memory that crosses it, and callback
lifetime when a delegate is handed to native code. The Chapter 22 lambda
lifetime lesson has a direct analogue here, and it bites harder.

### 10. A glossary

**Missing:** Appendix E.

**Evidence:** zero hits for "glossary". The book uses TU, ODR, RVO, ABI and
vtable in the ordinary course of explaining things, and the reader's future
colleagues will use ADL, POD, CRTP and SFINAE without introduction.

**A contribution looks like:** one page, alphabetical, one or two sentences
per term, each pointing at the chapter where the idea actually lives. Terms
the book already uses come first; terms the reader will merely *hear* come
second, marked as such.

---

## Known gaps carried over

Already recorded in [CONTRIBUTING.md](CONTRIBUTING.md) and still open:

- **A COM-style refcounting lab.** Bestiary Shape 3 is the only one of the
  five shapes without an exercise.
- **A threaded-callback lab.** Currently only a FakeDevice stretch goal;
  the natural companion to item 4.

## Deliberately out of scope

Nothing yet. Items closed as out of scope move here with the reasoning, so
the same suggestion does not arrive twice.

---

## Structural item (not content)

Splitting the book into per-chapter files under `book/` with a script that
concatenates them. Worth doing **only if contributor volume justifies it** —
one file is a real feature while the book has few editors, since it makes
grep, cross-reference checking, and the numbering invariant trivial to verify.
Revisit when merge conflicts in `going-unmanaged.md` become a regular event.
