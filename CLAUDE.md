# CLAUDE.md — project guide for AI-assisted work on Going Unmanaged

## What this project is

"Going Unmanaged — A Hands-On C++ Handbook for C# Developers." A curated,
exercise-driven handbook built by the maintainer (17y C# developer returning
to C++ for SDK work) together with an AI assistant. The canonical content is
the per-chapter files under `book/` — one file per chapter and appendix
(6 parts, chapters 1–42, appendices A–K — Chapter 24 and Appendix C were
retired by SITE-PLAN.md step 3; a retired number or letter is never reused,
and the Contents keeps a one-line entry for each so the ordered list still
renders true), indexed by `book/README.md`. There
is no single-file build any more: the book is read on GitHub and as the
static site `scripts/build_site.sh` renders from the same files (SITE-PLAN.md,
step 2 retired the concatenated file). Appendices run A–K — E is the glossary (item 10), G the bridge catalogue (item 16's
lookup half: the mechanism survey and decision table; no C++ listings —
check_verbatim.sh enforces that no cpp fence lands there), H the choosing
procedures (item 17: which container, how to take a parameter, what to
return, value-or-pointer inside a collection — the opposite contract to
G, its every cpp fence pinned to `exercises/choosing/`, which asserts the
costs the page quotes), and I const-correctness (item 8), whose lab asserts
refusals rather than results. J is the CMake catalogue — Chapters 26,
27 and 40's lookup half, the shape G is to Chapter 38: no cpp fence
(enforced), and its two cmake fences are checked projects: the runtime-delivery
one build_all.sh generates and holds both ways, pinned to that script's
heredoc by check_verbatim.sh, and the system-library one,
`exercises/cookbook/cmake/CMakeLists.txt`, pinned to the page both ways
(banner-stripped) and run under CTest. K is the standards catalogue — the lookup
half of Chapter 10's "check your standard" flags: which standard a
toolchain speaks and how to ask (feature-test macros, MSVC's
`/Zc:__cplusplus`), every feature the book names by the standard it
arrived in with the newer spelling beside the taught one, and the
standards in one sitting. Its probe is `exercises/cookbook/standard.cpp`,
quoted by excerpt (check_verbatim holds page → file), built by
build_all.sh at C++17, C++20 and — under the expected probe — C++23 with
its printed readings asserted, refused at C++14 by its own static_assert
(the constlab grep discipline), and built by the buildlab-msvc job with
and without `/Zc:__cplusplus`.
Part VI ("The Real Codebase") is the home for appended chapters about what a
project has that an exercise does not — build systems, dependencies, testing,
concurrency, authoring an ABI boundary, reading tool output. Chapter 29
discharges the threading promises made in Ch 16/18; Chapter 30 is the
authoring side of Ch 16's Bestiary (which only teaches consuming those
shapes); Chapter 31 supplies the sanitizer reports Chapter 15's sabotage runs tell
the reader to study but never show. Chapters 32–37 are the ticket-shaped
scenario chapters (ROADMAP items 11, 14 and 15, all DONE): symptom first,
no concept named in advance, diagnosis behind a spoiler fold. Ch 33 adds the inversion the
job supplies — the sanitizer report arrives attached to the ticket, and the
diagnosis is made on paper from the report alone before anything is built.
Ch 34 closes item 7 inside the format (padding, endianness, the overlay
cast) with the opposite inversion: the canonical flags stay green on all
three of its bugs, so the attached capture hand-decoded against the ICD is
the only oracle. Ch 35 closes the carried-over Bestiary Shape 3 gap:
FakeSDK ships a refcounted 2.0, two opposite ownership bugs cancel on one
object, and the fix is a type (adopt/share wrapper), not a patch. Ch 36
(item 14) is the performance ticket, whose attached profile appears to
ACQUIT — support's percentages are correct and irrelevant, because the
crime is 33 allocator calls per tick on a deadline thread; the judge is a
replaced-operator-new allocation counter, not a timing. Ch 37 (item 15)
is the crash-dump ticket — nothing to run, a stripped customer crash
report, fault address 0x10 read as null+offsetof, and the guilty frame
inlined out of the stack; the acceptance is a two-configuration matrix.
Chapter 38 (item 16's chapter half) is the bridge-out chapter — the
main-thread queue Ch 29 promised, a frozen command registry, and the
StubHostAdapter seam; two of its three breaks are hangs no sanitizer
names, so bridgelab's judge is a bounded wait on every invoke. Appendix
G is that chapter's lookup half — the survey of bridge mechanisms, led
by the host's own channel, and the decision table. Chapter 39 (item 9) is
the publishing half of P/Invoke — one signature written twice in two
languages and compared by nothing; interoplab judges it with no .NET
anywhere, because marshal.h stands in for the marshaller the way FakeSDK
stands in for a vendor. Chapter 40 (item 22) is the plug-in's build — a
MODULE with one exported symbol, a hand-written find-module for an SDK with
no config package, and the finding that hidden visibility covers what you
compile and not the archive you link, judged by the export table read back.
Chapter 41 (item 23) is the working subset of templates — the seam as a
policy type, Chapter 28's promised compile-time fake — whose lab's broken
policy CALLS Pump on purpose: a member of a class template is compiled only
when used, so without that call the missing function would compile clean
even with the static_assert deleted, and the refusal would prove nothing.
Chapter 42 (item 25) is the formula field — user-typed text evaluated
against injected objects: tokens as a variant that own their text, a
tree of a closed set of node kinds behind `unique_ptr`, recursive descent
with a value-or-error at every level (a loop where right recursion gives
7 for `8 - 3 - 2`), an RAII depth guard, and a provider seam whose dot
belongs to the provider; exprlab's judge is a hand-computed value table,
error positions, the depth limit at N and N+1, and a German locale
switched on where the machine has it.
README.md carries the origin story and contribution invitation; the book
itself stays free of meta-commentary.

NOTE (platform): LeakSanitizer is NOT supported on macOS/arm64 — a leaking
program under ASan reports nothing there, so a clean run says nothing about
leaks. Leak coverage comes from CI/Linux. Stated in Chapter 31 and in
Chapter 25's Finding 10.

## Layout

- `book/` — the book, canonical, one file per chapter and appendix:
  `NN-<slug>.md` for chapters 01–42, `A-`…`K-<slug>.md` for the appendices
  (digits sort before letters, so the listing is the reading order)
- `book/README.md` — front matter and the Contents; GitHub renders it when
  someone opens `book/`, so it is the reader's entry point. The Contents
  leads with three entry-point groups — `### Reference`, `### Concepts`,
  `### Labs` — and the six-Part reading order follows. The site's nav is
  built from those three groups (scripts/site_hooks.py), so every page under
  `book/` must appear in exactly one of them; a page in none is missing from
  the nav and the strict build fails
- `book/symptoms.md` (Symptom Index: from what is on the screen to the page
  that owns it — the table that closed Chapter 31 until SITE-PLAN step 3,
  plus the Gotchas by symptom) and `book/components.md` (the pieces of the
  labs and the cookbook usable as they stand, each with its judge) — the two
  pages that are neither chapter nor appendix; H2-titled like a chapter, no
  number or letter, linked from the Reference group
- `exercises/` — one directory per exercise, each with a TASK.md task card
  (three non-exercise directories aside: `cookbook/`, `choosing/`, `skeleton/`);
  `exercises/README.md` is the index (exercise ↔ chapter ↔ solution)
- `exercises/fakesdk/`, `exercises/fakedevice/` — also carry vendor-style code
  users must NOT edit (contracts included whole by chapters 17/18)
- `exercises/buildlab/` — Greeter.h/.cpp + main.cpp, the Chapter 23 starting
  point; built by build_all.sh so the scaffold stays green. Also
  `CMakeLists.txt`, Chapter 26's reference build description assembled from
  that chapter's snippets — build_all.sh configures/builds/runs it three
  times (default, Debug + `-DGREETER_SANITIZE=ON`, `-DGREETER_AUDIT=ON`) and
  reads each switch's reach back from the compile database
- `exercises/deplab/` — Chapter 27's lab (its *Try it*, steps 1–4), and the
  only one whose subject is entirely build description: `mathlib/` is the
  dependency, and one `app/main.cpp` is consumed three ways — vendored
  (`add_subdirectory`), fetched (`FetchContent` + a `file://` URL) and found
  (`find_package(mathlib CONFIG)` against an installed prefix) — so the three
  `consume-*/CMakeLists.txt` are the whole lesson and the app cannot tell
  them apart. Nothing here is quoted in Chapter 27; `mathlib/CMakeLists.txt`
  is included whole by Chapter 40, so an edit to it shows on that page at
  once. Four rules are
  load-bearing and easy to undo by
  accident. (1) mathlib's install/export half is wrapped in a top-level
  guard: paths 1 and 2 reach it through `add_subdirectory`, which would
  otherwise make those the *consumer's* install rules and publish a private
  vendored dependency's config package out of the app's prefix. (2) The
  exported include directory is
  `$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>`, never a literal
  `include` — the two must agree, or an install-dir override splits them
  and the consumer dies at the `#include`, past configure.
  (3) The fetched path builds at BOTH tags and each run must report the
  version its own tag carries: building once proves only that the mechanism
  runs, and asking merely that the two outputs *differ* passes a pin that
  chose the wrong commit. (4) `MATHLIB_TAG` is a cache variable, so the
  chapter's "re-point GIT_TAG" means `-DMATHLIB_TAG=`, not editing the
  default — the file says so, because a reader who edits it in place sees
  nothing change. Its git use needs a git that can clone `file://`, which is
  `--require-git`; the two `.cpp` files are also built and run under the
  canonical flags in the flat section, since the CMake paths apply none
- `exercises/testlab/` — Chapter 28's `tiny_test.h` and `buffer_test.cpp`,
  included by the chapter from below their banners (the page follows an
  edit; nothing to mirror by hand), plus a
  TASK.md. build_all.sh builds the suite with `-I solutions` and runs it
- `exercises/abilab/` — Chapter 30's three worked boundaries, plus a TASK.md:
  `Widget.h`/`.cpp` (PIMPL), `IScorer.h` + `scorer.cpp` (interface + factory),
  `engine.h`/`.cpp` (`extern "C"` façade), each with a `*_demo.cpp` caller.
  The headers and the two full listings are included by the chapter from
  below their banners, so an edit shows on the page at once.
  build_all.sh builds each as a separate binary of TWO translation units
  (implementation + caller): the caller seeing only the boundary header is the
  subject matter, so never merge a demo into one TU with its implementation
- `exercises/threadlab/` — Chapter 29's lab: a TASK.md and nothing else. It is
  the Chapter 18 device with a driver thread the reader builds, so it links
  `../fakedevice/`'s vendor code rather than copying it; the reference solution
  is `solutions/device_threaded_solution.cpp`
- `exercises/cookbook/` — Appendix F's recipe listings, one TU per domain
  (files, strings, timing, handles, lookups, paths, async, events, logging,
  alternatives, errors, expected, json, containers, flags, ownership,
  crypto, watch, http, database, shm, namespaces, macros, attributes,
  numbers), each with a `main()` asserting what its recipes claim;
  build_all.sh builds and runs all twenty-five. `numbers.cpp` is Recipe 52,
  and its main() is a value table rather than a smoke test: half its rows
  exist because C++ answers them differently from C# (`Math.Round(2.5)` is 2,
  `std::round(2.5)` is 3), and the out-of-range cast the recipe guards is UB,
  so it is never executed — UBSan's report on it is quoted in the Trap, along
  with the reason the same guard is silently wrong if retyped for `int64_t`.
  Two other TUs are not recipes either, like `standard.cpp`:
  `macros.cpp` carries Chapter 12's four preprocessor hazards, and asserts
  the BROKEN spelling of each as well as the fixed one, because the whole
  point is that all four compile clean under `-Wall -Wextra` and answer
  wrong. `attributes.cpp` is Recipe 51, and is judged the way constlab is —
  an attribute changes no instruction, so build_all.sh compiles it three
  more times with `-Werror` and one `-D` each and asserts the build is
  REFUSED naming `nodiscard`, `return-type` and `deprecated`; delete any of
  the three attributes and the clean build stays exactly as clean, which is
  why the refusals are the check. `namespaces.cpp` is the one built from TWO translation units
  (`namespaces_other.cpp` beside it) and the pair is the subject rather than
  a build detail: an unnamed namespace's claim is about what happens ACROSS
  translation units — the same helper name defined twice with different
  bodies, and no collision — which one file cannot demonstrate. It also
  carries Chapter 12's inline-namespace listing, whose claim is about the
  SYMBOL and so is read back with `nm` in build_all.sh: `v1` and `v2` both
  emitted, no unqualified `audio::frame_size`, and `clamp_to_range` absent
  from the table. Marking `v1::frame_size` static leaves every assertion in
  the program green and fails the nm check, which is why the check exists. `json.cpp`
  is the one with a dependency — `exercises/third_party/nlohmann/`, vendored
  with its version recorded, included with `-isystem`. `expected.cpp` is
  the one cut by standard rather than domain — C++23, Chapter 8's chaining
  listing, its own probe. `crypto.cpp` is the second probe: it links the
  system's libcrypto through `pkg-config` (`--require-openssl` in CI, which
  sets PKG_CONFIG_PATH on macOS), nothing vendored, and its judge is
  published test vectors — NIST's, the GCM specification's, and for Recipes
  47–48 RFC 7914's, 5869's and 4231's — because a round trip would prove
  only that seal and open agree with each other. `http.cpp` is the third probe, libcurl the same way
  (`--require-curl`), judged with no network: a `file://` fixture runs the
  write callback and the transport's error path exactly as for `https://`,
  and a loopback server in the harness (POSIX sockets;
  `http.cpp` is not built by the MSVC job) serves a redirect, a 500 and a
  stall, so the server's verdict, the redirect follow and the timeout's
  unit are judged too. Recipe 46 (post a JSON body, read a JSON reply)
  lives in the same TU, which therefore also includes the vendored
  nlohmann/json with `-isystem`; for it the harness's server reads a
  request whole and echoes the body and its Content-Type back as JSON, so
  the round trip is judged on what the server received, then answers a
  400 whose body is JSON (so only the status refuses it), a 200 that is
  HTML, and JSON without the key. `database.cpp` is the fourth probe, sqlite3
  (`--require-sqlite`), judged by an in-memory database — and by
  `sqlite3_close`'s return code, which is `SQLITE_OK` only when every
  statement was finalized, so a leaked statement fails the run the way
  `FakeSdk_LiveAllocations` did. `cmake/CMakeLists.txt` is not a recipe: it
  is Appendix J's system-library entry — the same three libraries found
  through `find_package(SQLite3)`, `find_package(OpenSSL)` and
  `pkg_check_modules(... IMPORTED_TARGET libcurl)`, the three TUs built a
  second way and their judges run under CTest by build_all.sh when cmake
  and all three libraries are present, quoted whole on that page
  (banner-stripped, both ways). `watch.cpp` owns a
  thread and is built under TSan as well. `shm.cpp` is the cookbook's one
  listing that is platform-split end to end (POSIX and Win32 under `#if`;
  Recipes 29 and 38 guard one call): no library, `-lrt`
  on Linux, a fork in the harness so the cross-process claim is real, no
  TSan build because TSan sees one process, and the `buildlab-msvc` job
  builds the Win32 half through `check.ps1` (as it does `timing.cpp`,
  `paths.cpp` and `files.cpp`) mapping one object twice in one process. `files.cpp` also
  carries Recipe 49 (a file mapped rather than read) with a replaced
  `operator new` in BOTH forms as its judge — under ASan `new[]` does not
  route through the scalar replacement, and a heap copy passed the judge
  until the array form was replaced too.
  The recipe
  functions are INCLUDED by the appendix, each between `// --8<-- [start:recipe-N]`
  and `[end:recipe-N]` marker comments: edit the function in the file and the
  page follows; moving a marker changes what the page shows (the mains are
  scaffolding and appear in no listing). `rust/` is the same cookbook in
  Rust (SITE-PLAN step 5): a dependency-free crate, one module per domain
  (`files.cpp` → `src/files.rs`; `async.cpp` → `src/async_work.rs`, since
  `async` is a keyword), each recipe between the same `recipe-N` markers and
  each module's tests asserting what the recipe claims. On the page a recipe
  is two tabs, C++ and Rust, each an include. build_all.sh runs
  `cargo test --offline` with `RUSTFLAGS=-D warnings` behind a probe
  (`--require-cargo`, which CI passes). All 52 recipes have the tab: 39
  carry code (38 tests), and 13 carry a one-line note instead, because the
  standard library has no JSON, calendar, cryptography, HTTP, SQLite, regex
  (Recipe 44 gets a std answer anyway), shared memory or memory mapping and
  the crate takes no dependency — the note opens with the reason there is no
  code and then names the ecosystem crate. Seven concept chapters (1, 4, 6, 7,
  8, 10, 41) close with a short `### In Rust` section placed before their
  last section — Rust where it sharpens the C++ point, never woven into
  the explanation — and Chapters 38 and 39 point at Chapter 30's Rust
  client as one more consumer of the C ABI
- `exercises/constlab/` — Appendix I's lab, and the first of the four places
  in the repo whose judge asserts a build FAILS (`templatelab/`,
  `cookbook/standard.cpp` and `cookbook/attributes.cpp` are the others). `counter.h` + `main.cpp` compile and run
  clean; five const violations behind `-DCONSTLAB_VIOLATION_1..5` must each
  be REFUSED, and the diagnostic must name const or read-only. Two rules keep
  that from being vacuous and are easy to undo: the clean build must succeed
  (so an unrelated typo fails there, not silently here), and the grep sees the
  diagnostic's MESSAGE only, everything up to `error:` cut away first — the
  directory is called `constlab`, so any path left in the string matches
  `const` on its own
- `exercises/choosing/` — Appendix H's measurements, the other non-exercise
  appendix directory: `counted.h` (a copy/move-counting type plus the
  `CHECK` judge), `passing.cpp` (procedures 2–3) and `storing.cpp`
  (procedures 1 and 4), no TASK.md. Its banners name exactly which units
  Appendix H quotes — and which three of `passing.cpp`'s Chapter 6 quotes,
  the value-category traps priced with the same instrument — and
  each named unit is included by its page between section markers, and
  check_verbatim.sh holds that every marked section is on a page. Two rules are load-bearing and easy to undo by accident: the
  judge is `CHECK` (counts failures, sets the exit code), never `assert`,
  which a Release build compiles away; and build_all.sh builds
  `passing.cpp` a SECOND time under `-fno-elide-constructors`, because
  with NRVO on a returned temporary and a returned named local both
  measure zero and the page's guaranteed-vs-permitted distinction is
  never exercised
- `exercises/skeleton/` — Chapter 26's project layout as a directory to
  copy, the third non-exercise directory: `CMakeLists.txt` (root, `src/`,
  `tests/`), `CMakePresets.json` (`dev`, `release`), `.clang-format`
  (Google-based, 4-space, 120 columns), `.clang-tidy` (bugprone, performance,
  `readability-identifier-naming` with the book's PascalCase/`name_`/`k`
  rules, `WarningsAsErrors: '*'`), `include/myplugin/session.h`,
  `src/session.cpp`, `tests/session_test.cpp`, and READMEs in `cmake/` and
  `third_party/` saying what goes there. build_all.sh runs its README's
  three commands literally (`cmake --preset dev`, build, `ctest`) under the
  cmake probe, reads `-fsanitize=address` back from the compile database,
  and removes `exercises/skeleton/build/` afterwards. CI's `skeleton-style`
  job is the style half: `clang-format --dry-run --Werror` and `clang-tidy`
  over the skeleton's sources with pip-pinned tool versions (format output
  moves between clang-format majors), plus a negative test that rewrites
  `count_` to `_Count` and requires tidy to refuse it naming both
  `reserved-identifier` and `identifier-naming` (two grep statements, not one
  `&&` list: under `bash -e` a failure inside `&&` is ignored unless last).
  The buildlab-msvc job builds it under Visual Studio with the sanitizer on
  and runs its test through CTest. It is the ONLY directory the repository
  formats by machine: the labs' layout — one-line structs, aligned trailing
  comments, a short body on the line that declares it — is what the chapters
  quote verbatim, and a formatter over them would break every verbatim
  pairing at once. Two rules are load-bearing: the test binary links the
  sanitizer target BY NAME as well as inheriting it through the library, and
  build_all.sh reads the flag back per translation unit — a grep for the
  flag anywhere passes with the library instrumented and the test not; and
  `MYPLUGIN_BUILD_TESTS` defaults to ON only at top level, because
  `enable_testing()` registers tests for its own directory and below, never
  for a consumer's root
- `exercises/exitlab/` — Chapter 32's ticket lab. TASK.md carries the broken
  2.4.1 listings (book-only, they exist to fail); the committed files are
  the FIXED state, quoted verbatim in the chapter's fix section, and
  build_all.sh builds them TWICE with the translation units in opposite
  orders and runs both — order-independence is the fix's claim, and one
  build cannot prove it
- `exercises/reportlab/` — Chapter 33's ticket lab. TASK.md carries the
  attached sanitizer report and the broken 2.6.0 main (book-only, it exists
  to fail); the committed files are the FIXED state, quoted verbatim in the
  chapter's fix section, and build_all.sh runs the binary at 0 hot-plugs
  AND at 100 — growth-independence is the fix's claim, and one count
  cannot prove it
- `exercises/capturelab/` — Chapter 34's ticket lab. TASK.md carries the
  attached capture, the ICD table and the broken overlay parser (book-only,
  it exists to fail); the committed files are the FIXED state, quoted
  verbatim in the chapter's fix section, and build_all.sh asserts the
  decode against the chapter's hand-decoded values — the sanitizers are
  silent on this chapter's bug class, and the capture's second frame is
  deliberately unaligned
- `exercises/comlab/` — Chapter 35's ticket lab. FakeSDK2.h/.cpp is NEW
  vendor code (the refcounted 2.0 of Chapter 17's SDK — a separate drop,
  fakesdk/ unchanged; contract quoted verbatim in Ch 35, same rules as the
  other Fake* files); TASK.md carries the broken 2.0 port (book-only, it
  exists to fail); ref.h + main.cpp are the FIXED state, quoted verbatim
  in the chapter's fix section, and build_all.sh holds them to two judges
  at once — the binary asserts the vendor's live-object counter reaches 0
  after shutdown (catches a release too few), the sanitizers catch a
  release too many. check.sh links it via the `comlab` argument
- `exercises/perflab/` — Chapter 36's ticket lab. TASK.md carries the
  broken 2.1.0 Tick, the attached profile and the host's engine log (the
  broken shape is book-only, it exists to fail); meter.h/.cpp + main.cpp
  are the FIXED state, quoted verbatim in the chapter, and build_all.sh
  runs the harness at 50 AND 1000 ticks — the harness replaces operator
  new and asserts ZERO heap allocations, because the sanitizers are
  silent on an accidental copy and a timing assert would measure the
  runner instead of the code
- `exercises/dumplab/` — Chapter 37's ticket lab. TASK.md carries the
  broken 3.4.0 session.cpp and the attached customer crash report (the
  broken shape is book-only, it exists to fail — at -O2, so the reader
  can hold a post-mortem on the corpse); session.h/.cpp + main.cpp are
  the FIXED state, quoted verbatim in the chapter, and build_all.sh runs
  both device configurations — the crash lived only in the one the bench
  never had
- `exercises/bridgelab/` — Chapter 38's lab. TASK.md carries the three
  broken shapes (book-and-card, identical by rule — they exist to fail);
  the committed headers + main.cpp are the FIXED state, quoted in the
  chapter BY EXCERPT (the three headers whole from below their banners,
  two excerpts of `main.cpp` between section markers; the card's broken
  listings are held in the chapter by containment),
  and build_all.sh builds it twice — canonical flags, then a second
  source in the probe-gated TSan section — because the three breaks
  split across the two builds. The harness allows no unbounded wait:
  every invoke takes a deadline, the lab's judge, since a hang would
  stop CI rather than fail it
- `exercises/interoplab/` — Chapter 39's lab, and the only one whose caller
  is imaginary: `plugin.h`/`plugin.cpp` are the published boundary and
  `main.cpp` plays the .NET marshaller through that header alone (two TUs,
  like abilab — a caller that can see the implementation is not a
  boundary). `marshal.h` models what the marshaller does to a struct, a
  string and a delegate; it deliberately does NOT model the collector,
  because a collected delegate is a use-after-free and broken programs
  stay book-only — the sink carries an `alive` flag so the header's
  documented callback window is asserted instead. The struct-misdeclaration
  judge is the load-bearing one: delete the size-field check and it is an
  ASan stack-buffer-overflow, not a wrong value
- `exercises/pluginlab/` — Chapter 40's lab: three CMake projects. `sdk/` is a
  vendor-style drop (header + static helper library, installed to a prefix,
  deliberately NO config package — vendor code, never edited); `plugin/` is
  the reference plug-in (a MODULE library, `cmake/FindHostSDK.cmake` writing
  the imported target by hand, hidden visibility plus one exported symbol,
  and linker options because hidden covers what you compile and not the
  static library you link — the chapter's finding); `host/` is a stand-in
  host that dlopens the module and calls its entry point. build_all.sh
  installs the drop, builds both, runs the host against the module (twice:
  the second run passes an older host's shorter table, which the plug-in
  must refuse), reads the export table back with `nm` — `Plugin_Entry`
  present, nothing of the SDK's or the plug-in's own; `Describe` has external
  linkage on purpose so the visibility preset has something to hide — and
  builds both again with the sanitizer flags injected via CMAKE_CXX_FLAGS,
  all under the cmake probe; the buildlab-msvc job builds all three under
  Visual Studio and reads the module's exports and dependents back. Six
  files are included whole by the chapter (the vendor header, four plug-in
  files and deplab's mathlib CMakeLists), so an edit shows on the page at
  once — and a one-line `set()` excerpt is pinned by containment
- `exercises/templatelab/` — Chapter 41's lab: `session.h` (a `Session<Sdk>`
  over a policy, with the detection idiom `HasSdkShape` and a static_assert
  that names the missing function), `policies.h` (the real device over
  `../fakedevice/`, linked not copied, and a recording double), `util.h`
  (`if constexpr`, a fold-expression `Join`, a `Ring<T, N>`) and the judging
  `main.cpp`. build_all.sh builds and runs it against FakeDevice, then
  builds it once more with `-DTEMPLATELAB_BROKEN_POLICY` and asserts the
  build is REFUSED with the static_assert's own text as the first error
  (the constlab discipline: the diagnostic's message only, path cut away).
  The three headers are included by the chapter from below their banners,
  and one `static_assert` from `main.cpp` between section markers
- `exercises/exprlab/` — Chapter 42's lab: `expr.h` (an `Error` with a
  position, the `ISymbols` provider seam, a `Formula` parsed once and
  evaluated per row, `max_depth`), `expr.cpp` (tokenizer, recursive-descent
  parser, the tree, the evaluator) and the judging `main.cpp`, plus a
  TASK.md that carries no listing. `expr.h` is included whole from below
  its banner; the other twelve listings are included from `expr.cpp` and
  `main.cpp` between named section markers. build_all.sh builds the two TUs
  under the canonical flags and runs the judge; `from_chars` for numbers
  is load-bearing — `strtod` reads the locale, and the judge switches to
  `de_DE` where the machine has it (CI's Linux job generates the locale;
  the judge prints which it used, or that it skipped). Two rules are
  load-bearing and easy to undo by accident. (1) `max_depth` bounds BOTH
  the parser's recursion (the `Depth` guard in `Unary`) and the tree's
  height (checked in `Build`, where every node is made): a flat `1+1+…`
  never nests in the parser and still builds a spine the evaluator and
  the destructor recurse through — two hundred terms overflow a 512 KB
  thread with the guard alone. (2) The number parse has two spellings
  under one `#if`: `from_chars` where the library has the `double`
  overload, `strtod_l` with a `"C"` locale where it does not — Apple's
  libc++ has it only for a deployment target of macOS 26 or later — and
  build_all.sh builds the lab a second time on macOS with
  `-mmacosx-version-min=15.0` so the `#else` branch is judged too
- `solutions/` — reference solutions for all exercises; plus `Buffer.h`, the
  Chapter 15 class extracted out of `buffer.cpp` so the testlab suite can
  include it (Chapter 28's structural point, applied)
- `scripts/build_all.sh` — builds AND runs every solution; the repo invariant.
  Its last eleven sections may skip: one builds `exercises/deplab/` three ways
  (Chapter 27), one configures, builds and runs `exercises/buildlab/`'s
  CMakeLists, one runs `exercises/skeleton/`'s three preset commands, one installs `exercises/pluginlab/`'s SDK drop and builds,
  loads and inspects its plug-in (Chapter 40), one generates Appendix J's
  runtime-delivery project and installs it with and without a runpath, one rebuilds
  `solutions/device_threaded_solution.cpp` under
  `-fsanitize=thread` (a second build, because TSan and ASan do not combine),
  one builds `exercises/cookbook/expected.cpp` as C++23, one builds
  `exercises/cookbook/crypto.cpp` against the system's libcrypto, one
  builds `exercises/cookbook/http.cpp` against the system's libcurl, one
  builds `exercises/cookbook/database.cpp` against the system's sqlite3, and
  the last configures `exercises/cookbook/cmake/` (Appendix J's
  system-library entry) and runs those three recipes under CTest, which
  needs cmake and all three libraries at once. Without cmake on PATH, without a git that can clone a `file://`
  repository (deplab's FetchContent path only), without a ThreadSanitizer
  that can compile *and start* a trivial program, or without a compiler
  that has `<expected>`, or without a libcrypto, a libcurl or a sqlite3
  that `pkg-config` can find, each prints SKIPPED and stays green;
  `--require-cmake`, `--require-git`, `--require-tsan`, `--require-expected`,
  `--require-openssl`, `--require-curl` and `--require-sqlite` (CI passes
  all seven) refuse to skip
- `scripts/check.sh` — builds/runs a learner's own attempt under the canonical
  flags: one or more .cpp files, compiled in the order written (= link order,
  which Chapter 32's two-order test turns on), then an optional vendor
  argument (fakesdk/fakedevice/comlab), then run args; `SAN=thread` switches
  the sanitizer for the threadlab's second build, `SAN=none` removes it
  entirely and `OPT` (default 0) sets `-O`. Those last two are for the
  exercises whose subject is what the tools do NOT catch — Chapter 27's ODR
  diamond, whose step 5 cannot be shown by a build that warns — and the
  script drops its "sanitizers quiet" claim under `SAN=none`, because an
  exit 0 from an uninstrumented build is not evidence. An EMPTY `SAN` still
  gets the default: turning the sanitizers off is a thing you say, not a
  thing that happens to you
- `scripts/check.ps1` — check.sh's Windows/MSVC mirror (`cl /std:c++17 /W4
  /EHsc /fsanitize=address`, same source-list/vendor/run-args shapes; MSVC
  has no UBSan and no TSan, and the script says so). Smoke-tested by the
  buildlab-msvc CI job so it cannot rot on a Mac-based maintainer
- `scripts/check_verbatim.sh` — holds the book's listings to the code the
  repo ships, now that a page does not carry code: a listing is
  `--8<-- "path:section"` inside its fence, the section fenced in the source
  by `--8<-- [start:section]`/`[end:section]` comment lines (or the whole
  file, `--8<-- "path"`, where a chapter shows a file entire), and the site
  build renders the file — drift is impossible by construction (SITE-PLAN
  step 4). What the script checks is everything around that: every include
  names a file that exists and a section marked exactly once in it; every
  section a source marks is included by some page (a marked unit is a
  promise a page shows it); no cpp or cmake fence of four lines or more on
  any page is a copy of a source region (a listing pasted back instead of
  included is refused); the seven ticket/lab TASK cards' broken listings
  appear in their chapters (book-and-card code with no compiled source — it
  exists to fail — so it stays copied, held by containment); two one-line
  quotations (Chapter 39, Chapter 40) are held by containment; Appendix G
  holds no cpp fence and Appendix J only cmake fences, every one an include.
  Chapter 26's and 27's ODR listings are included from the heredocs in
  `check_platform_claims.sh`, which generates and asserts them, and Appendix
  J's runtime-delivery project from `build_all.sh`'s. A new listing goes on
  the include form: mark the section, include it, and the script and the
  strict site build both hold it. Never spell the marker syntax literally in
  a comment or a page — the snippet engine takes it for a marker. CI runs
  it in the book job
- `scripts/check_markup.sh` — enforces the alert and mermaid-fence shapes
  below over `book/`, plus one typographic rule:
  no two `---` rules with only blank lines between them, which GitHub draws
  as two dividers with a gap rather than the single separator the source
  looks like, and which seventeen files had acquired invisibly. Run by CI,
  and worth running locally after touching either. Structure only, never
  mermaid grammar
- `scripts/check_mermaid.sh` — the other half: hands every chapter with a
  diagram to mermaid-cli and fails if one does not draw. Needs `mmdc`
  (`npm install -g @mermaid-js/mermaid-cli`); without it a local run says
  SKIPPED rather than passing, and CI's `--required` refuses to skip
- `scripts/check_platform_claims.sh` — runs the sanitizer demonstrations and
  asserts what the chapters promise *per platform*: ASan's exit code (134 on
  macOS, 1 on Linux), TSan's (134 / 66), whether LeakSanitizer reports at all
  (no on macOS/arm64), and whether a frame carries a column number (Ch 31's
  atos-vs-llvm-symbolizer point), and what a null `const char*` handed to
  `std::string` does under each standard library (libc++ faults in the
  constructor, libstdc++ throws — Recipe 23's trap, detected by macro rather
  than by OS), and what a file truncated under a live read-only mapping does
  (Recipe 49's trap: `SIGBUS` on Linux, a completed read of the old byte on
  macOS — the one section that holds the two platforms to opposite
  outcomes rather than to different codes for the same one). It also holds Chapter 26's macro-ODR pair — a define that
  changes a struct's layout in one TU only: silent link, order-dependent
  answer, and, unlike Chapter 27, no order caught by the sanitizers for that
  listing (the object is built in the larger layout), with `session.h`
  pinned to the script's heredoc by check_verbatim.sh — and
  Chapter 27's ODR diamond —
  both link orders link silently, the two orders disagree, and exactly one
  is caught, naming `GetTimeout` — the one section whose first two claims
  are about the linker rather than a compiler-rt runtime and so hold on
  every platform alike; its two headers are the chapter's own listings,
  pinned to the page by check_verbatim.sh. CI runs it on ubuntu AND macos with
  `--required`, because the platform overclaims it exists to catch are exactly
  what a one-platform check cannot see. The broken programs are generated into
  a temp dir, never committed — `solutions/` stays clean
- `scripts/check_search.sh`, `scripts/search_rank.js`,
  `scripts/search_queries.tsv`, `scripts/search_tags.yml` — the search, held
  to a fixture, because retrieval is what a daily reference is (SITE-PLAN.md)
  and it was the one part of the build nothing checked: `--strict` proves
  every link resolves and says nothing about whether a page can be *found*.
  `search_queries.tsv` is the promise — a query a reader types, and the page
  it must reach — in three groups, by the C# API, by the C++ spelling, and by
  what is on the screen; a widened tolerance on a line is the honest record
  of a query the corpus cannot rank higher (`Task.Run` is two of the
  commonest words in a book full of exercises). `search_rank.js` does NOT
  reimplement the ranking: it loads the site's own search worker, the file
  the browser loads, and talks its message protocol, so what is checked is
  what a reader gets and a Material bump needs no change here. The first
  draft did reimplement it on lunr and got three things wrong that changed
  the answer, which is the reason for the rule. `search_tags.yml` is the
  synonyms — the words a reader types that a page's own words do not carry —
  applied by the hook rather than written as front matter, so `book/` stays
  the files GitHub renders; it lists EVERY page, and a page it does not list
  fails the build the way a page in none of the README's three groups does.
  Its own header carries the five rules, each learned by measuring: a tag
  reaches every section of its page, a common word as a tag hijacks every
  query containing it, tags are tokenized like everything else, every term
  the reader types matches as a prefix, and a one-letter word reaches every
  tag starting with it. CI runs it in the `site` job
- `.github/workflows/ci.yml` — runs build_all.sh on every push/PR, plus a
  `platform-claims` job (check_platform_claims.sh on ubuntu and macos), a
  `buildlab-msvc` job (Chapter 26's CMakeLists under MSVC both ways, then
  Chapter 40's three pluginlab projects with `dumpbin /exports` reading the
  module's one symbol back, then
  Chapter 14's `/Zc:nrvo` claim: the chapter's own `cl /std:c++17 /W4 /EHsc`
  line must print exactly one extra move-construction in the RVO section, and
  adding `/Zc:nrvo` must print none — it is the one claim in the book resting
  on a vendor default rather than on the standard — then a check.ps1 smoke
  test: one plain build, one through the fakesdk vendor path, and one in the
  ticket labs' multi-TU form, two reportlab sources plus a run arg), and a
  `book` job: check_markup.sh, check_verbatim.sh,
  check_mermaid.sh (which installs mermaid-cli, the job's one slow step),
  and a lychee link check
  (`--offline --include-fragments`: relative links and anchors are blocking,
  external URLs are not checked); the `site` job below checks the same
  links again through MkDocs' parser
- `.github/workflows/release.yml` — on a `v*` tag, creates the GitHub
  release page for it (notes point at CHANGELOG.md); it attaches nothing
  since the single file retired
- `mkdocs.yml`, `scripts/site_hooks.py`, `scripts/site-requirements.txt`,
  `scripts/build_site.sh` — the book as a static site (MkDocs Material) over
  the unchanged `book/` files, built strictly into `build/site/`; the hooks
  file generates the nav from `book/README.md`'s three entry-point groups,
  applies `scripts/search_tags.yml` (below), turns the GitHub alerts into
  admonitions, marks the `<details>` folds for md_in_html, and
  `pymdownx.slugs` reproduces GitHub's anchors
  so no link changes. `SITE-PLAN.md` is the plan this serves — the book is
  becoming a reference consulted daily, not a book read once — with the
  measurements behind it and the next steps; read it before changing the
  book's structure. CI's `site` job runs the strict build, which is also a
  second link-and-anchor checker, with mermaid-cli installed so the
  diagrams render at build time exactly as they are published.
  `.github/workflows/site.yml` deploys the site to GitHub Pages on every
  push to `main` — https://mkhomutov.github.io/going-unmanaged/ — with
  directory URLs (`site_url` in mkdocs.yml), and refuses to publish a page
  that still carries a diagram fence for the browser to render

## Where chapter code lives (decided once; full text in CONTRIBUTING.md)

Part VI chapters carry code that is not an exercise solution. It goes under
`exercises/<lab-name>/`, next to that lab's task card, as buildlab and testlab
do; `solutions/` stays flat, stdlib-only `.cpp` files — with one amendment: a
header is permitted there exactly when a chapter requires the demo/test split
(Ch 28 forced the first, `solutions/Buffer.h`, because a class sharing a TU
with `main()` cannot be tested; duplicating it into the lab was rejected).
Everything verifiable is wired into `build_all.sh` (still the one invariant,
still ALL GREEN). A step needing a tool that may be missing locally (cmake now,
TSan later) copies check_mermaid.sh: SKIPPED locally, plus a `--require-<tool>`
flag CI passes so it can never skip there (`--require-cmake`, `--require-git`,
`--require-tsan`, `--require-expected`; check_mermaid.sh's own predates the
pattern and is just `--required`) — and the
probe does the thing rather than looking for the tool, since TSan can be
installed and still fail to start. A lab that revisits an SDK the repo already
has links that vendor code in place (threadlab). Deliberately broken
demonstration programs — Ch 31's sabotage runs, Ch 30's break-it-first steps,
Ch 29's Try it step 4 — stay book-only and unverified on purpose (ROADMAP item
5). Ch 26, 28, 29 and 30 are all done under this convention: the four-chapter
Part VI code debt is closed, and a future Part VI chapter reuses it.

## Hard invariants (never break these)

1. Every file in `solutions/` compiles clean and runs clean under
   `-std=c++17 -Wall -Wextra -fsanitize=address,undefined` (words.cpp and
   invalid.cpp use `-std=c++20`; `exercises/cookbook/expected.cpp` is the
   one C++23 file, built as its own probe with `--require-expected` in CI). Run `./scripts/build_all.sh` after ANY
   change to code; it must print ALL GREEN.
2. `exercises/*/Fake*.h|.cpp` are "vendor code": the book includes their
   public contracts whole, so a change shows on Chapter 17/18's page (or
   35's, for comlab's FakeSDK2) the moment it is made — and is almost never
   the right move.
3. Chapter numbering is load-bearing: the book cross-references chapters by
   number ("Chapter 6", "Finding 3 of Chapter 25"). Inserting a chapter means
   renumbering ALL later chapters AND every in-text reference, including
   inside code comments. Prefer appending; grep before and after:
   `grep -n "Chapter [0-9]" book/*.md`
4. No real vendor/product names in the book's SDK material (the point is
   generality). Open-source ecosystems named as study material are fine
   (libusb, PortAudio, SQLite, Qt, Unreal, STM32 HAL, COM as a technology).
5. Solutions never use anything beyond the standard library, and the Rust
   crate under `exercises/cookbook/rust/` never beyond Rust's (no
   `[dependencies]`; Recipe 7 declares its two C functions with
   `extern "C"` rather than pull in the libc crate). `exercises/`
   may carry a vendored third-party header under `exercises/third_party/`
   (today: nlohmann/json, for the cookbook's three JSON recipes), included
   with `-isystem`, with the version and any patch recorded in that
   directory's README — Chapter 27's own vendoring rule, applied to the
   repo. A second category — a library the system provides and the
   repository links but never copies in (Chapter 27's fourth strategy):
   located through `pkg-config`, built behind a probe with a
   `--require-<lib>` flag CI passes, judged against an oracle that needs no
   network and no state outside the run — published vectors for libcrypto
   (`crypto.cpp`); a `file://` fixture and a loopback server of the
   harness's own for libcurl (`http.cpp`); an in-memory database for
   sqlite3 (`database.cpp`) — reachable
   from the cookbook only, and nothing for NOTICE, since nothing is
   redistributed. A network in CI is not an oracle: a recipe whose only
   judge is a server somewhere is a recipe nobody checks the day the server
   is down.
6. The site stays buildable from `book/`: after ANY change there run
   `./scripts/build_site.sh`, which is strict — a link to a missing file or
   anchor, a page the generated nav does not know, or a page
   `scripts/search_tags.yml` does not list, fails it. Then
   `./scripts/check_search.sh`, which holds the search to its fixture. CI
   runs both.

## Content conventions

- New material (chapters, exercises, appendix sections) is reviewed against
  the twelve questions in CONTRIBUTING.md, "The questions every piece of
  material answers" (decided once; full text there): moment of need, which
  C# reflex it confronts, what wrong-that-looks-like-working looks like,
  the feedback loop that tells the reader they are wrong, the one-sentence
  principle for Appendix B, findability from a symptom, and what re-verifies
  each claim. Answers are mechanisms in the material, never intentions;
  corrections need only the last question. ROADMAP.md's "What earns a place
  here" section is the same list applied to whether an item belongs at all.
- Voice: honest, practical, first-person-curator; C# comparisons are the
  pedagogical spine ("in C# this would..."). British-neutral English.
- Every exercise chapter follows: *trains / vendor code (if any) / the task /
  reference solution / pitfalls / stretch goals*.
- Findings (Chapter 25, titled *Gotchas*) follow strictly: **Symptom / The theory /
  broken-vs-fixed code / Habit**. Community findings via PR keep this shape.
- Recipes (Appendix F) follow strictly: **In C# / The recipe / Why it looks
  like this / Trap** — the trap a one-line `[!WARNING]`, the why
  cross-references to the owning chapter, numbers append-only like Findings.
  The recipe is two tabs — `=== "C++"` and `=== "Rust"` — each holding one
  indented include, `--8<-- "exercises/cookbook/<domain>.cpp:recipe-N"` and
  `--8<-- "exercises/cookbook/rust/src/<module>.rs:recipe-N"`; the code
  lives only in the files, between their `recipe-N` markers. A recipe with
  no idiomatic Rust answer says so in one line under the tab instead of
  carrying a strained one. Where Rust sharpens the point, one sentence
  opening **In Rust** closes the Why paragraph; never a parallel
  explanation.
- Key-principle quotes are in speakable first person ("I check every error
  code...") — they double as a cheat sheet (Appendix B mirrors them; keep
  the two in sync when adding one).
- Code style in the book: 4 spaces, `name_` members, comments explain WHY.
- Book heading scheme: H1 = parts (and the title/Appendices separators),
  H2 = chapters and appendices, H3 = sections. Unchanged by the split: a
  part's H1 (with its intro prose) lives at the top of that part's FIRST
  chapter file, and the `# Appendices` H1 at the top of
  `A-fundamentals-refresher.md`.
- One `---` between sections, never two. A second thematic break with only
  blank lines before it draws as a second divider with a gap, not a heavier
  one — legible enough that seventeen files carried one unnoticed.
  `check_markup.sh` enforces it; only `---` is checked, since that is the
  only spelling the book uses.
- Adding a chapter = a new `NN-<slug>.md` file plus its entries in
  `book/README.md`'s Contents: one in the reading order, one in whichever of
  Reference / Concepts / Labs it belongs to (the site nav comes from those,
  and a file in none of them fails the strict build). Links between files keep the GitHub anchor as
  a suffix — `](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake)`
  — because that one spelling resolves on GitHub and on the site alike
  (`pymdownx.slugs` reproduces GitHub's slugs). Same-file links stay
  `](#anchor)`. Chapter files carry no navigation of their own: the site
  supplies it, and GitHub has the Contents.
- Reference solutions in exercise chapters sit inside `<details>` spoiler
  folds ("Show the solution — do the exercise cold first"); keep that shape
  for new exercise chapters.
- Callouts are a `> [!TYPE]` marker line followed by a one-line blockquote
  body that opens with the bold label. GitHub draws them as alerts; the
  marker line is stripped of meaning everywhere else, which is why the label
  carries the weight. Type follows from the label, not from how strongly you
  feel about the sentence:
  - `[!TIP]` — **Key principle:**, **The stance to hold:**, **Habit:**. The
    book's advice, and by far the common case (36 of the 72 at last count).
  - `[!WARNING]` — **Trap:**, **Gotcha:**. Something that compiles, runs,
    and is wrong.
  - `[!IMPORTANT]` — the two non-negotiable rules only: Chapter 5's virtual
    destructor, Chapter 30's one rule. **Keep it rare.** The first pass
    typed 31 of 40 IMPORTANT and the colour stopped saying anything — three
    identical purple boxes closed Chapter 26 and told the reader nothing.
    A new callout is IMPORTANT only if breaking it is a bug, not a smell.
  - `[!NOTE]` — the C#-developer surprise: **The big reveal:**,
    **Surprise for C# devs:**.
  - **The bold label stays.** Appendix B mirrors the key principles by name,
    the labels keep the callouts greppable, and they are the whole callout
    in any renderer without alert support.
  - **Top-level only.** GitHub does not render an alert nested inside a list
    or a `<details>` fold. A callout that must live there stays plain bold
    with no marker — there are none today. Blank line before the marker.
- Diagrams are mermaid in a ```` ```mermaid ```` fence, rendered natively by
  GitHub and, on the site, to inline SVG at build time (light and dark, by
  `site_hooks.py` when `mmdc` is on PATH; a local build without it falls
  back to browser-side rendering from a CDN, and CI and the Pages workflow
  always have it). The rules:
  - **A diagram is additive.** It illustrates prose that already stands on
    its own — nothing is deleted or rewritten to make room, because a
    renderer without mermaid shows the fence and the prose must still
    carry the point. At most one lead-in sentence.
  - **Basic `flowchart` and `sequenceDiagram` only**, no `style`/`classDef`
    and no hardcoded colours — GitHub themes mermaid for light and dark
    itself, and a hardcoded colour is unreadable in one of them.
  - **Fence unindented, blank line before it**, and never inside a
    `<details>` fold or a list.
  - **Look at it before committing; a green CI is not enough.** CI now draws
    every diagram (`check_mermaid.sh`), so a broken one cannot merge — but
    that only proves it renders, and every layout bug below rendered fine.
    Nothing automatable can tell you the picture is any good. Extract the
    block from the committed file, render it
    in a browser (`mermaid.render`, then `getBBox()`), and look at it:
    layout bugs are only visible drawn. The ones already paid for — a
    decision tree stacked into a 1000px column by `flowchart TD` when it
    wanted `LR`; crossed edges from declaring nodes in the wrong order
    (dagre places by declaration, so declare the node the reader starts
    from first); `subgraph`s rendering in reverse of declaration order; an
    `alt` with one branch reading as the whole story.
  - **Watch the width, not just the height.** GitHub scales a diagram to the
    container width, so a wider picture is smaller *text* for every reader.
    Prefer splitting a diagram at a seam the chapter already teaches over
    letting it grow.

## Known gaps / roadmap (good first tasks)

`ROADMAP.md` is the full ranked list of missing content, with evidence and a
sketch of what each contribution looks like. Everything on it APPENDS
(Chapter 43+, Appendix L+) — no item requires renumbering. Delivered items
stay on the list marked DONE so item numbers never shift. Short version:

- Tier 1 (load-bearing): CLOSED. Build systems/CMake was item 1 and is now
  Chapter 26; dependency management was item 2 and is now Chapter 27;
  testing was item 3 and is now Chapter 28; concurrency was item 4 and is
  now Chapter 29
- Tier 2: two items open — the framework shape (item 18: Bestiary Shape 5 is named in Chapter 16 and
  taught nowhere — two readers of the 2026-09 study stopped there, and
  Shape 4 has no lab either — though only its interrupt-context half is
  genuinely untaught, which is item 21 — so this item is about the shape
  with no treatment at all rather than the last shape without a lab; still
  a legitimate candidate for out-of-scope-with-a-sentence), and below
  the mutex (item 19: Chapter 29 and Chapter 36 between them state the
  deadline path's prohibition and never its alternative — their two opposite
  defaults for a foreign thread now cross-reference each other, which is all
  of the item that is done). Both were sequenced after item 9 (P/Invoke),
  which is **DONE as of 2026-09-02** — Chapter 39 — so both are now
  unblocked. Templates you will write was item 23 and is now Chapter 41 +
  `exercises/templatelab/`, the second lab whose judge asserts a build
  FAILS. CMake for the plug-in was item 22 and is now Chapter 40 +
  `exercises/pluginlab/`. Consolidated const-correctness was item 8 and is now Appendix I
  plus `exercises/constlab/`, the one lab whose judge asserts a build FAILS;
  it builds on item 17 — *Choosing*, now DONE as Appendix H plus the
  copy/move-counting `exercises/choosing/` — and points at that appendix's
  parameter procedure rather than re-deriving `const&`. Scenario chapters were item 11 and are DONE — Chapters 32–35, then
  items 14 and 15 appended the performance ticket (Chapter 36 + perflab)
  and the crash-dump ticket (Chapter 37 + dumplab) in the same format,
  which stays open to new tickets by PR. Byte-level protocol work was
  item 7 and is now Chapter 34; authoring an ABI boundary was item 6 and
  is now Chapter 30; the debugging chapter was item 5 and is now
  Chapter 31
- Tier 3: C++/C# interop was item 9 and is now Chapter 39 + `exercises/
  interoplab/` — the publishing half of P/Invoke, judged with no .NET
  anywhere because `marshal.h` stands in for the marshaller the way
  FakeSDK stands in for a vendor. Item 9's scope note stays OPEN: a full
  treatment of being loaded BY a runtime (JNI, a Python extension, a Node
  addon) is the shape two readers actually wanted, and Chapter 39 only
  names it. SOLID without the runtime (item 13 — the
  reader's design vocabulary, un-fused from the .NET machinery; a
  gather-and-translate chapter like item 8), the retrofit (item 20, also
  after item 9 — no lab modernises working code whose callers must keep
  compiling: they start blank, or from buildlab's working trio, or from a
  ticket lab's already-broken code. The reader who is handed the native
  layer *because* it is old has no chapter, and the acceptance test is that
  the caller's TU is byte-identical before and after), the interrupt-context
  callback (item 21 — Bestiary Shape 4's second addition to Shape 1; its
  first, initialization order, is Chapter 32's whole subject and is done.
  The smallest item on the list, the only one filed with no reader
  evidence, and probably a section of item 19 rather than its own material:
  an ISR is item 19's deadline path with a harder deadline. Sequenced after
  19, and closable by it), and the bridge out
  was item 16 and is now DONE — Chapter 38 + stdlib-only
  `exercises/bridgelab/` (the main-thread queue under a bounded-wait
  judge), plus Appendix G, the survey of mechanisms and its decision
  table.
  The glossary was item 10 and is now Appendix E (letters run A–K; C has
  since been retired). The Rosetta Cookbook was item 12 and is now Appendix F — Recipes
  1–8, then 9–13 (files, paths, async), then 14–16 (events, logging,
  timers), then 17 (UTF-8↔UTF-16), then 18–20 (find, optional, variant),
  then 21–23 (a custom exception, value-or-error, the empty string), then
  24 (a diagnostic compiled out of Release);
  it grows by PR like the Findings log
  (Recipe template in CONTRIBUTING.md)
- Carried over: both DONE. The COM-style refcounting exercise (Bestiary
  Shape 3's missing lab) is Chapter 35 + `exercises/comlab/`; the
  threaded-callback lab is `exercises/threadlab/` plus
  `solutions/device_threaded_solution.cpp` under a TSan CI gate
- Deliberately out of scope (the section exists so the same suggestion does
  not arrive twice — read it before filing anything): **classroom
  scaffolding** — slides, rubrics, per-chapter lecture timings, a separated
  answer key — because it is a different product, not a missing chapter, and
  the CC-BY/MIT split already lets a trainer build it; and **general
  lock-free data-structure design**, the half of item 19 that is not ours.
  Closing a *numbered* item this way is a decision that belongs in an issue
  first; an entry that was never an item is recorded there directly
- Structural item: splitting the book per-chapter under `book/` with a build
  script that concatenates — DONE, see the ROADMAP entry
- Structural item 2: the web resource — IN PROGRESS, see `SITE-PLAN.md` (the
  plan) and the ROADMAP entry; step 1, the generator over the unchanged book,
  is done

## Versioning

Semver-ish with chapter/Finding numbering as the public contract:
renumbering = MAJOR, appended content = MINOR, corrections = PATCH.
Releases are annotated git tags + a CHANGELOG.md entry. Numbering freezes
at v1.0. Full policy in CONTRIBUTING.md.

## Licensing

Dual, and the boundary runs *through* the chapter files: prose under `book/`
(and the site built from it) is CC-BY 4.0; all code is MIT — `exercises/`,
`solutions/`, `scripts/`, `.github/`, and every code sample inside a chapter, so
a reader can paste a snippet without an attribution obligation.
`exercises/third_party/` is the exception — vendored code under its author's
own license (today nlohmann/json, MIT, with its `LICENSE.MIT` beside the
header), recorded in that directory's README and in `NOTICE`. `LICENSE` is the
**unmodified** MIT text and must stay that way: GitHub's detector scores the
whole file against canonical MIT, and a preamble in it cost the repo its
detected license once already (it reported `NOASSERTION` until the preamble came
back out). The split lives in `NOTICE` instead. `LICENSE-CC-BY-4.0` is the
verbatim CC-BY legal text — never reword it either. `book/README.md`'s front
matter carries a license line because the site travels without the repo.
Contributed material lands
under the same split (CONTRIBUTING.md), so relicensing later would need every
contributor's consent.

## Working with the maintainer

The maintainer also uses the exercises for personal training. When asked for
help with an exercise ATTEMPT (as opposed to repo maintenance), default to
REVIEW mode: critique their code against the chapter's pitfalls; don't write
the solution for them unless they explicitly ask.
