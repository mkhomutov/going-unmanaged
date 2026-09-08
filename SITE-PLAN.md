# Site plan — from handbook to a reference you use daily

Decided 2026-09-08. This file is the standing answer to "what is the book
turning into, and what is the next step?" — the structural counterpart to
[ROADMAP.md](ROADMAP.md), which lists missing *content*. Steps are marked
DONE as they land, the way roadmap items are.

## The decision

The book stops being a book you read front to back and becomes a **web
reference you consult** — by the task in your head, by the symptom on your
screen, by the C# API you were about to reach for. Three consequences,
all settled:

- **No print, no PDF, no single file.** The per-chapter files under `book/`
  stay canonical and keep rendering on GitHub; a static site is the
  delivered form. The concatenated single-file build was a stand-in for a
  print artifact and retires with it (step 2 below).
- **Pure technical.** Nothing in the resource narrates a reader's week,
  month or mood. The material that does — a one-week practice schedule,
  first-week questions to ask the team, a closing note, the spaced-repetition
  tails on the ticket chapters, the episode recaps that open them — goes, or
  moves out of the resource into the repo's own contributor documents (step 3).
  The C# comparison stays: it is the product, and it is the reason a reader
  who does not yet know the C++ term can find the page.
- **Rust enters as a column, not a syllabus** (step 5). A third tab on a
  recipe beside C# and C++; a third client at the ABI boundary beside C# and
  Python; never a parallel Part I. And the repo's one rule extends to it
  unchanged: a Rust listing CI does not compile and run is worse than none.

## What the book is today, by shape

Word counts are approximate, from `wc -w` on 2026-09-08 (146k in all).

| Shape | Pages | Words |
|---|---|---|
| Lookup catalogues | F, G, H, J, K, E, B, I, A; Chapters 13, 16, 25, 31 | 50k |
| Concept explanations | Chapters 1–12, 26–30, 41 | 40k |
| Labs with a judge | Chapters 14–23, 32–40, 42 | 50k |
| Timeline and meta | Chapter 24, Appendix C, half of Appendix D | 3k |

A third of the text is already lookup material, and the five "lookup half"
appendices (G, H, J, K, and F itself) were each a step toward this shape. The
Contents page hides that: it presents six Parts in reading order, and a
reader arriving from a search engine does not want a syllabus.

## Step 1 — prove a generator on the unchanged book — DONE

MkDocs Material over `book/` as-is, nothing copied or rewritten:
[`mkdocs.yml`](mkdocs.yml), [`scripts/site_hooks.py`](scripts/site_hooks.py),
[`scripts/build_site.sh`](scripts/build_site.sh), output in `build/site/`.
What it cost, measured on the built site:

| Feature | In `book/` | On the site |
|---|---|---|
| Cross-file links with a `#chapter-n--title` anchor | 609 | 609 resolve; `pymdownx.slugs` reproduces GitHub's slugs byte for byte, double dash included |
| `> [!TYPE]` alerts | 151 | 151 admonitions, through a thirty-line hook, no plugin |
| mermaid fences | 12 | 12 render (client-side; see the open decision below) |
| `<details>` folds | 14 | 14, once the hook marks them for `md_in_html` — the first build kept all fourteen tags and rendered their markdown as literal text, and no checker noticed |
| Strict build (`mkdocs build --strict`) | — | green, 3 seconds, 7.6 MB |

Include-from-file was proven on Recipe 1: a page whose C++ tab is
`--8<-- "exercises/cookbook/files.cpp:52:60"` renders the function straight
out of the cookbook source, beside a C# tab and a Rust tab, with a copy
button — the whole of step 4's mechanism, working. Two frictions found and
recorded rather than fixed: the generated nav footers at the bottom of every
chapter are redundant on a site with its own navigation (they go in step 2),
and `IMPORTANT` is not a Material admonition type, so the hook maps it to
`danger`, Material's strongest, to keep GitHub's visual order (a stylesheet
would have to live under `book/`). Versions are pinned in
`scripts/site-requirements.txt` because Material's own team warns that
MkDocs 2.0 removes the hook system with no migration path; mdBook is the
fallback if that forces a move, and its include-by-anchor is the same
mechanism.

The strict build is now a CI job (`site`), because it is also a second,
independent link-and-anchor checker beside lychee, and it fails on an
include whose file or line range does not exist.

## Step 2 — retire the single file — DONE

Removed `scripts/build_book.sh`, the `<!-- nav:begin -->` footers it
generated (and the trailing `---` that sat above them in 41 of the 53 files —
it was a chapter separator for the concatenation, and on its own it drew a
stray divider under every last paragraph), the `--check-nav` CI step,
`build/going-unmanaged.md`, and the release workflow's attachment of it (the
workflow still creates the release page for a tag). Links between files keep
the GitHub anchor suffix — that is what makes the same file render on GitHub
and on the site — and CLAUDE.md now gives that as the reason. Hard invariant
6 reads "the site stays buildable from `book/`". `check_markup.sh` lost its
"and the built single file" pass.

**Acceptance, met:** CI green with the book job three steps shorter,
`grep -rn "nav:begin" book/` empty, and the release workflow publishing
nothing it does not build.

## Step 3 — the structural pass — DONE

The reading order becomes three entry points. Files do not move and numbers
do not change (see *Numbering* below); the nav in `site_hooks.py` and a new
`book/README.md` express the grouping.

- **Reference** — for daily use: the cookbook (F), choosing (H), CMake (J),
  standards (K), the bridge catalogue (G), const (I), the glossary (E), the
  principles (B), the fundamentals (A), the toolchain (13), the bestiary
  (16), the gotchas (25, see below), reading tool output (31). Plus one new
  page the CONTRIBUTING questions already demand — a **symptom index**: from
  "crash after `main` returns", "objects still live at unload", "LNK2019",
  "dropouts with the plug-in loaded", "a sanitizer report I cannot read" to
  the page that owns it. Every ticket chapter and every Finding gets a row.
- **Concepts** — read when a topic surfaces: Parts I–IV and the Part VI
  explanations (26–30, 41). Each page must stand alone; the edit is a
  *prerequisites* line at the top of each page in place of the "as Chapter 6
  said" glue, not a rewrite.
- **Labs** — done cold: 14–23, the tickets 32–37, 38–40, 42. Each with its
  task card, its judge, and the solution in a fold. The lab directories are
  the downloadable unit; a **Components** page lists the pieces that are
  reusable as they stand and how each is judged: the RAII handle wrapper
  (Recipe 7), the forty-line test framework (testlab), the main-thread queue
  (bridgelab), the ring and join utilities (templatelab), the expression
  evaluator (exprlab), the skeleton project.

The cuts, and where each goes:

| Today | Becomes |
|---|---|
| Chapter 24, Practice Plan | Deleted. The one rule it carried — do the exercise cold, with the compiler, debugger and sanitizer as the only feedback — moves to `exercises/README.md`, which is where the reader who is about to attempt one is standing. |
| Appendix C, Learning With (and Without) AI Assistants | Moves to CONTRIBUTING.md and the root README: it is the method by which this material is checked, which is provenance, not reference. |
| Appendix D, "First-week questions to ask the team" and "A closing note" | Deleted. The resource-list halves of D stay. |
| The "Reproduce it cold" tail on each ticket chapter | Deleted. |
| Ticket chapters 32–37: episode recaps, ticket numbers, the studio, support closing the ticket | Deleted. The **fixtures stay** — the sampling profile, the crash report, the hex capture, the sanitizer report are technical artifacts, FakeSDK in another form. Each chapter is retitled by symptom (the title today becomes the first line of the page, if anything). |
| Chapter 25, "Findings from Practice — a Living Log"; each entry's **Found in:** | Retitled *Gotchas*; **Found in:** becomes **Symptom:**. The Finding numbers stay as anchors, and the page stays a catalogue that grows by PR. |
| The root README's origin story | Stays in the root README. It is the repo's provenance and belongs there; it never enters the site. |

What does **not** change: the C# comparisons, the "why it looks like this"
paragraphs, the "In the wild" sections, the twelve questions in
CONTRIBUTING.md (all technical criteria), the callout and diagram rules,
and every hard invariant in CLAUDE.md.

**Status.** PR 1 of 3, *the cuts*, is DONE: Chapter 24 and Appendix C
retired (the file gone; the Contents keeps a one-line *Retired* entry for
each so the ordered list still renders true, and the number and letter are
never reused), Appendix D retitled *Resources and Further Reading* and
trimmed, the six "Reproduce it cold" tails gone, Chapter 25 retitled
*Gotchas: the Findings Log* with **Symptom:** in place of **Found in:** and
CONTRIBUTING's template changed to match. Appendix C's rule lives on as
CONTRIBUTING's "Assistants, and the exercises"; its offline-lifelines list
was already Appendix D's. PR 2 of 3, *the tickets*, is DONE: the six chapters are titled by symptom (Crash on Exit; A Value Reads Zero After Hot-Plug; Every Capture Rejected as Malformed; Objects Still Live at Unload; Dropouts With the Plug-in Loaded; Crash at Session Close, Field Units Only — file names unchanged, anchors updated everywhere), the openings state the evidence and the rule without narrating the series, the ticket numbers are gone from chapters and cards alike, and the fixtures — profile, crash report, capture, report, migration notes — are untouched. PR 3 of 3, *the entry points*, is DONE: `book/README.md` leads with the three groups (Reference / Concepts / Labs, every page in exactly one) and keeps the six-Part reading order below them; the site nav is built from those groups and a page in none of them fails the strict build; `book/symptoms.md` is the Symptom Index (Chapter 31's table promoted to its own page, its three "this chapter" rows now links, plus the Gotchas by symptom) and `book/components.md` the Components page; the two "month N" openers (Chapters 38 and 42) are gone. The prerequisites line was not added page by page: a grep for reading-order glue found four sentences in eighteen concept pages, all already carrying links to the chapter they lean on, so the inline links do that job.

**Acceptance:** `grep -rn -i "day [0-9]\|week [0-9]\|first week" book/`
returns nothing outside code and quoted output; the six ticket pages are
titled by symptom and open on the evidence; the symptom index has a row for
every ticket and every gotcha; `check_verbatim.sh` and `check_markup.sh`
green, because the cuts touch prose the pairings do not cover.

## Step 4 — invert the verbatim direction — DONE

Today a chapter *contains* the code and `check_verbatim.sh` proves the file
agrees, in one direction or both. After this step the file is the only
source and the page is a view of it: every listing that is a pairing today
becomes `--8<--` of a **named section** in the source file, marked in
comments —

```cpp
// --8<-- [start:recipe-1]
std::string read_all_text(const std::filesystem::path& path) { ...
// --8<-- [end:recipe-1]
```

— never a line range, because line ranges rot the first time a comment
above the function grows. Drift becomes impossible by construction, copy
and download come free, and `check_verbatim.sh` shrinks to "every marker a
page names exists, and every unit a lab's banner names is included by some
page" (the reverse direction Appendix H and bridgelab hold today). The
banner-stripping convention becomes a marker placed below the banner. The
first pass is Appendix F, because it is the largest, the most-consulted, and
already one-fence-per-unit; then the ticket labs' fixed files; then the rest.

`book/` on GitHub will then show the include directive rather than the code.
That is the accepted cost: the site is the delivered form, and GitHub's
rendering of `book/` becomes what the single file was, a second view nobody
optimises for.

**Status.** DONE in two PRs. PR 1: Appendix F — 49 recipes included from 20 cookbook files by `recipe-N` sections, with the rendered code blocks on every page compared before and after (325 blocks, identical), the generic both-ways include check in `check_verbatim.sh`, and a manual-trigger Pages workflow (`.github/workflows/site.yml`) for the day hosting is switched on. PR 2: every other pairing — 132 includes in all, 28 of them whole files, 104 named sections across 48 marked files, the same oracle identical again; the copied-pairing lists in `check_verbatim.sh` are gone, replaced by the include checks plus a guard that refuses any fence of four lines or more that copies a source region. Two kinds of listing stay copied on purpose and are held by containment: the TASK cards' broken listings (they exist to fail and have no compiled source; an HTML-comment marker inside a card's fence would render as code on GitHub) and two one-line quotations. Five files whose chapter listing kept the banner's title line had their banner reordered so the title sits below the marker. Every lab banner that said "editing one means editing the chapter in the same commit" now says edit here and the page follows.

**Acceptance:** no cpp fence on a page that a pairing covers contains code;
`check_verbatim.sh` still fails when a marker is deleted from a source file
or a page names one that is not there; `build_all.sh` ALL GREEN, since the
markers are comments.

## Step 5 — Rust

In this order, each behind a `cargo` probe with a `--require-cargo` flag CI
passes (the TSan and OpenSSL pattern):

1. **The cookbook as three tabs.** C#, C++, Rust on every recipe where Rust
   has a plain answer, with the Rust half in `exercises/cookbook/rust/` as a
   crate whose tests assert what the recipes claim — the same judge shape as
   the C++ TUs. Recipes with no idiomatic Rust answer say so in one line
   rather than carrying a strained one.
2. **The concept pages where Rust sharpens the C++ point.** Ownership and
   borrowing beside the Rule of Five (Chapters 1, 6); `Option`/`Result`
   beside `optional`/`expected` (Chapters 8, 10); traits beside interfaces
   and concepts (Chapters 4, 7, 41). A short *In Rust* section at the end of
   each, after *In the wild*, never woven into the C++ explanation.
3. **Rust as a client of the boundary.** Chapters 30, 38 and 39 are
   language-neutral at the ABI; a Rust caller with `extern "C"` and
   `#[repr(C)]` joins C# and Python as one more consumer of the same seam,
   with a listing that CI links against abilab or interoplab.

Not in scope: a Rust version of the labs. FakeSDK is about consuming a
C-flavoured SDK from C++; the Rust version of that is bindgen and `unsafe`,
a different discipline and a different book.

## Numbering, titles and identifiers on the web

Chapter numbers and appendix letters were the book's addressing scheme and
are frozen by the versioning policy. On the site they become what they
already are in practice — stable identifiers: file names, anchors, the
"Finding 10" a colleague cites. Nothing renumbers. What changes is that a
number no longer needs to carry *order* to the reader: the nav groups by
entry point, and a ticket page is titled by its symptom with its number in
the breadcrumb.

Retitling is not renumbering. Under CONTRIBUTING.md's policy the structural
pass is a MINOR release (content moves, nothing is renumbered, no citation
breaks), and the same-commit rule for `book/README.md` still applies.

## Open decisions

- **Hosting.** GitHub Pages from the `site` job's output is the obvious
  choice; publishing is the maintainer's call and is not wired up by this
  plan. Until then `scripts/build_site.sh --serve` is the reader.
- **Diagrams at read time or at build time.** Material renders mermaid in
  the browser from a CDN; a browser that blocks it (the proof's did) shows
  the fence. A self-hosted `mermaid.min.js` was proven to work in step 1,
  and CI already installs mermaid-cli for `check_mermaid.sh`, so rendering
  to SVG at build time is the third option and the most "pure technical".
  Decide in step 3, when the diagram rules get their site clause.
- **Directory URLs.** The proof uses `file.html#anchor`, which mirrors the
  GitHub links exactly; `use_directory_urls: true` is prettier and MkDocs
  rewrites the links either way. Decide before hosting, since it changes
  every URL once.
