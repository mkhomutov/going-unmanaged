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
3. **New exercises** — especially new SDK shapes. Known gap: a COM-style
   refcounting lab (Bestiary Shape 3 has no lab yet).
4. **Missing chapters** — subjects the book does not cover yet.
   [ROADMAP.md](ROADMAP.md) is the standing list, ranked by what they cost a
   reader who hits them unprepared. Tier 1 is now closed — build systems,
   dependency management, testing and concurrency landed as Chapters 26-29,
   authoring an ABI boundary as Chapter 30 and the debugging chapter as
   Chapter 31 — so byte-level protocol work and const-correctness now lead.
   Open an issue before starting a large one, so nobody writes the same
   chapter twice.
5. **Translations and tooling** — build scripts, per-platform notes, anything
   that lowers friction.

## The Finding template

Findings live in Chapter 25 — `book/25-findings-from-practice.md` — and follow
one strict shape: **Found in / The theory / broken-vs-fixed code / Habit**. Community
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

## Where the book lives

The book is one file per chapter under [book/](book/README.md):
`01-ownership-and-raii.md` … `31-reading-what-the-tools-tell-you.md`, then
`A-`…`D-<slug>.md` for the appendices, with `book/README.md` carrying the
front matter and the Contents. Concretely, for a contributor:

- **Editing a chapter** — edit that one file. Nothing else moves.
- **A new chapter** — a new `NN-<slug>.md` file appended at the end of its
  part, plus its entry in the Contents in `book/README.md`. (New appendix:
  the same, with a letter prefix.)
- **Linking between chapters** — keep the GitHub anchor and put the file in
  front of it: `](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake)`.
  Links inside the same file stay plain `](#anchor)`. Plain prose references
  ("see Chapter 26") need no link at all.
- **The nav footer** at the bottom of each file, between `<!-- nav:begin -->`
  and `<!-- nav:end -->`, is generated — never hand-edit it. If you add or
  rename a file, run `scripts/build_book.sh --write-nav` and commit the
  result; CI fails on a stale footer.

The single-file book is a build artifact, not repository content:

```bash
./scripts/build_book.sh          # -> build/going-unmanaged.md
```

It concatenates the chapters in reading order and turns the cross-file links
back into in-page anchors. CI builds it on every push, and every tagged
release attaches it as a download.

## Where chapter code lives

Part VI chapters carry code that is not an exercise solution — build
descriptions, test harnesses, teardown skeletons, boundary headers. It has one
home, decided once so no PR has to argue it again:

- **Code a chapter builds and runs lives under `exercises/<lab-name>/`**, next
  to the task card of the lab it belongs to, exactly as `exercises/buildlab/`
  already holds the Chapter 23 Greeter trio and Chapter 26's reference
  `CMakeLists.txt`, `exercises/testlab/` holds Chapter 28's `tiny_test.h`
  and `buffer_test.cpp`, and `exercises/abilab/` holds Chapter 30's three
  worked boundaries with a caller for each. Reference *solutions* stay
  where they are: flat,
  standard-library-only `.cpp` files in `solutions/`. A lab that is a second
  pass over an SDK the repository already carries links that vendor code where
  it lives and copies nothing — `exercises/threadlab/` is Chapter 29's, and has
  no code of its own beyond its task card.
- **A header in `solutions/` is permitted exactly when a chapter requires the
  demo/test split.** Chapter 28 forced the first one: its suite tests the
  Chapter 15 Buffer, which cannot be tested while it shares a translation unit
  with `main()`, so the class moved to `solutions/Buffer.h` and
  `solutions/buffer.cpp` kept the demo. Duplicating it into the lab was the
  alternative and is not acceptable — two copies of a class the book teaches
  will drift. `solutions/` still stays flat and standard-library-only; this is
  the one thing it now also contains, and only for a reason a chapter states.
- **Everything verifiable is wired into `scripts/build_all.sh`.** That script
  remains the single repo invariant — it must print `ALL GREEN` — and adding
  chapter code without adding it to the script is half a contribution.
- **A step that needs a tool which may be absent locally** (cmake and
  ThreadSanitizer today) follows the `check_mermaid.sh` precedent: print a
  `SKIPPED` line and stay green on a machine that lacks it, and take a
  `--require-*` flag that refuses to skip. CI always passes that flag, so a
  step can never silently skip there. A local run should never look like a
  pass it did not earn. Name the flag for the tool — `--require-cmake` and
  `--require-tsan` in `build_all.sh`; `check_mermaid.sh` predates the pattern
  and spells its own `--required`, and checks one thing, so it stays. **Probe
  by doing the thing, not by looking for the tool.** The TSan step compiles
  *and runs* a trivial instrumented program, because ThreadSanitizer can be
  installed and still fail to start — a `command -v` style check would call
  that machine covered.
- **Deliberately broken programs stay in the book.** Chapter 31's sabotage
  runs and Chapter 30's break-it-first steps exist to fail, and are unverified
  on purpose — the reader's job is to reproduce them. ROADMAP item 5 records
  the reasoning. Do not try to make them green.

Chapters 26, 28, 29 and 30 are all done under this convention, which closes the
four-chapter Part VI debt it was written for. It stands for whatever Part VI
gains next.

One thing Chapter 30 added to it, small but easy to get wrong: **where the
separation between translation units is the lesson, keep it.** Its three demos
are three binaries of two TUs each — the boundary's implementation, and a
caller compiled against the header alone. Merged into one TU they would still
build, still run, still pass, and prove nothing, because the caller could see
everything the header exists to hide.

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
2. Run `./scripts/build_all.sh` — it must print `ALL GREEN`. If you touched
   `book/`, run `./scripts/build_book.sh` too (and `--write-nav` if you added
   or renamed a file).
3. Open a PR. Keep it focused: one Finding, one correction topic, or one
   exercise per PR.
4. In the PR description, say which exercise or chapter the change belongs to
   and (for Findings) confirm you hit the mistake yourself — that lived
   experience is what makes a Finding worth reading.

Contributors of accepted material are credited as co-authors of the handbook.

## License

By contributing you agree that your contribution is licensed under the same
[MIT License](LICENSE) that covers the project.
