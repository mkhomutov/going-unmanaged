# Contributing to Going Unmanaged

This handbook is open in the fullest sense: **contribute, and become a co-author.**
It was built as a curated, battle-tested collection of hands-on material for one
specific journey — managed developer goes native — and it grows the same way it
was made: by people doing the exercises, hitting something instructive, and
writing it down properly.

## What to contribute (most wanted first)

1. **Findings** — you did an exercise, hit something instructive, and can write
   it up in the Chapter 25 shape (see the template below). This is the heart of
   the project.
2. **Corrections** — anywhere the text is wrong, outdated, or misleading. Where
   the book disagrees with [cppreference](https://cppreference.com) or the
   [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/), they win;
   a PR aligning the book with them is always welcome.
3. **New exercises** — especially new SDK shapes. Known gaps: a COM-style
   refcounting lab (Bestiary Shape 3 has no lab yet) and a threaded-callback
   lab (currently only a FakeDevice stretch goal).
4. **Missing chapters** — subjects the book does not cover yet.
   [ROADMAP.md](ROADMAP.md) is the standing list, ranked by what they cost a
   reader who hits them unprepared: build systems, dependency management,
   testing, and concurrency are the big four. Open an issue before starting a
   large one, so nobody writes the same chapter twice.
5. **Translations and tooling** — build scripts, per-platform notes, anything
   that lowers friction.

## The Finding template

Findings live in Chapter 25 of `book/going-unmanaged.md` and follow one strict
shape: **Found in / The theory / broken-vs-fixed code / Habit**. Community
findings submitted via PR keep this shape. Concretely:

```markdown
## Finding N — <one-line title: the mistake, stated plainly>

**Found in:** <which exercise or real-world situation surfaced it>.

**The theory.** <Why the language behaves this way — the mental model a C#
developer is missing. Cite chapters by number where relevant.>

**The broken version:**

​```cpp
// the code as actually (mis)written, with a comment on the wrong line
​```

<Dissect what the broken version really does.>

**The fix:**

​```cpp
// the corrected code
​```

<What changed and why the fixed version is right.>

**Habit:** <one or two sentences, imperative, the reflex to build — something
you can actually check for in your own code from now on>.
```

Read a couple of existing Findings before writing yours — match their voice:
honest, practical, first person, with C# comparisons where they illuminate.
Number your Finding as the next free number at the end of the log; do not
renumber existing ones.

## Ground rules (CI enforces the first one)

- **Everything compiles clean and runs clean.** All contributed code must build
  and run under
  `g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined`
  (clang++ works identically). Run `./scripts/build_all.sh` before opening a
  PR — it must print `ALL GREEN`. If you add a solution, add it to that script
  in the same PR.
- **Vendor code is frozen.** `exercises/fakesdk/FakeSDK.h|.cpp` and
  `exercises/fakedevice/FakeDevice.h|.cpp` are "vendor code": their public
  contracts are quoted verbatim in Chapters 17 and 18. Changing them requires
  updating those chapters in the same commit — and is almost never the right
  move. Exercise solutions never edit vendor files.
- **Chapter numbering is load-bearing.** The book cross-references chapters by
  number ("Chapter 6", "Finding 3 of Chapter 25"), including inside code
  comments. Prefer appending new material over inserting. Renumbering is a
  MAJOR version event (see below) — open an issue first rather than doing it
  in a PR. If a renumbering is ever agreed, every later chapter and every
  in-text reference moves with it; verify with
  `grep -n "Chapter [0-9]" book/*.md` before and after.
- **No real vendor or product names in the book's SDK material.** The point of
  FakeSDK and FakeDevice is generality. Open-source ecosystems named as study
  material are fine (libusb, PortAudio, SQLite, Qt, Unreal, STM32 HAL, and COM
  as a technology).
- **Solutions use the standard library only.** No third-party dependencies.

## Content conventions

- **Voice:** honest, practical, first-person curator. C# comparisons are the
  pedagogical spine ("in C# this would…"). British-neutral English.
- **Exercise chapters** follow one structure: *what it trains / vendor code
  (if any) / the task / reference solution / pitfalls / stretch goals*.
- **Key-principle quotes** are written in speakable first person ("I check
  every error code…") — they double as a cheat sheet. Appendix B mirrors
  them; if you add one, add it in both places in the same PR.
- **Code style in the book:** 4 spaces, trailing-underscore members
  (`name_`), comments explain *why*, not what.

## Versioning and numbering

This is a book, not a library — but it has something that behaves exactly like
a public API: **chapter and Finding numbers**. People cite "Chapter 6" and
"Finding 3 of Chapter 25" in issues, notes, and links; renumbering breaks
those citations the way a signature change breaks callers. So the handbook
versions accordingly:

- **MAJOR** — renumbering or restructuring: inserting a chapter, reordering
  parts, renumbering Findings. Anything that changes what an existing number
  refers to.
- **MINOR** — appended content: a new chapter at the end of a part, a new
  Finding, a new exercise with its solution, a new appendix section.
- **PATCH** — corrections, wording, and code fixes that move no numbers.

`main` is always green (CI enforces the build invariant); releases are
annotated git tags with a [CHANGELOG.md](CHANGELOG.md) entry. Before v1.0 the
numbering is not yet frozen; from v1.0 on, treat renumbering as the breaking
change it is.

For contributors the practical rule is one line: **append, don't insert** —
then your change is MINOR or PATCH and needs no special discussion.

## Attribution

Contribute, and become a co-author — concretely:

- Accepted contributors are listed in the README's **Contributors** section
  (and permanently in the git history).
- A contributed Finding may credit its finder in the **Found in** line, in
  the book's own voice — e.g. *"**Found in:** the Buffer exercise (found by
  @handle)"*. Optional; say in your PR if you'd rather not be named.
- The book text itself stays free of meta-commentary beyond that — no
  changelogs or credits inside chapters.

## Submitting

1. Fork, branch, make your change.
2. Run `./scripts/build_all.sh` — it must print `ALL GREEN`.
3. Open a PR. Keep it focused: one Finding, one correction topic, or one
   exercise per PR.
4. In the PR description, say which exercise or chapter the change belongs to
   and (for Findings) confirm you hit the mistake yourself — that lived
   experience is what makes a Finding worth reading.

Contributors of accepted material are credited as co-authors of the handbook.

## License

By contributing you agree that your contribution is licensed under the same
[MIT License](LICENSE) that covers the project.
