# cookbook/ — Appendix F's listings, compiled

This directory is not an exercise: there is no task card and nothing to
attempt cold. It holds the recipe listings of Appendix F (*The Rosetta
Cookbook*), one translation unit per domain —

| File | Recipes |
|---|---|
| `files.cpp` | 1, 9, 38 — read and write a whole file; save one without losing the old one (the judge is the inode: a save must replace the file, not rewrite it) |
| `strings.cpp` | 2–5, 23 — split, join, build, format; the empty string that is not null |
| `timing.cpp` | 6, 16, 28–30 — time a call; a repeating timer; a scoped timer and a forwarding wrapper; a timestamp for a log line; a timeout handed to a C API |
| `handles.cpp` | 7 — wrap a C handle so it frees itself |
| `lookups.cpp` | 8, 18 — look up a key without inserting it; find an element, an index, or a substring |
| `paths.cpp` | 10–12, 39 — combine, the exists pair, listing; create, copy, move and delete, and a whole tree (the empty-name trap asserted; the POSIX libraries' cross-volume rename refusal on Linux only — MSVC copies instead — and the `u8path` round trip on Windows only, and each says so) |
| `async.cpp` | 13 — run work on another thread and wait for it |
| `events.cpp` | 14 — expose an event |
| `logging.cpp` | 15, 24 — print a diagnostic you will actually see; compile one out of Release (built twice, once with `-DNDEBUG`) |
| `alternatives.cpp` | 19–20 — a value that may be absent; a value that is one of several kinds |
| `errors.cpp` | 21–22 — an exception type of your own; a value or an error, on C++17 |
| `expected.cpp` | Chapter 8's chaining listing — `std::expected`, the one C++23 TU, built as its own probe |
| `json.cpp` | 25–26, 35 — serialize a record, read a config, walk a document you do not own; the one TU with a dependency, `exercises/third_party/nlohmann/`, included with `-isystem` |
| `crypto.cpp` | 36–37 — hash bytes; seal bytes for a reader in C#: the second TU behind a probe, it links the system's libcrypto through `pkg-config` (nothing vendored) and is held to NIST's and the GCM specification's own test vectors |
| `http.cpp` | 41 — call an HTTP endpoint: the third TU behind a probe, it links the system's libcurl through `pkg-config`; its judge needs no network — a `file://` fixture for the callback and the transport's error path, and a forty-line loopback server (POSIX sockets, so not on Windows) for the server's verdict, the redirect follow and the timeout's unit |
| `containers.cpp` | 27 — pre-size a collection: `reserve` against `vector(n)` and `resize` |
| `flags.cpp` | 31–32 — a feature flag read once and kept as a member; a `[Flags]` enum as an `enum class` with its operators |
| `ownership.cpp` | 33–34 — an owned object as a field, and who disposes it; an object too big for the stack |
| `watch.cpp` | 40 — notice a file changed: the polling watcher, judged by a bounded wait, a restored-older-timestamp change, and silence after the join; built under TSan as well, since it owns a thread |

— each with a `main()` that asserts what its recipes claim.
`scripts/build_all.sh` builds and runs all of them on every push, so a recipe
that stops being true stops being green — all but one under the canonical
flags, which pin C++17. `expected.cpp` is C++23, the one file here cut by
standard rather than by domain, and it is its own probe: a toolchain that
cannot build it prints SKIPPED, and CI passes `--require-expected` so it can
never skip there. `crypto.cpp` and `http.cpp` are the other probes: they need libcrypto and
libcurl, which `build_all.sh` locates through `pkg-config` and links from the
system — dependencies the repository links rather than vendors — and CI passes
`--require-openssl` (on macOS with `PKG_CONFIG_PATH` pointed at the keg-only
OpenSSL) and `--require-curl` (the Ubuntu runner installs
`libcurl4-openssl-dev`; the macOS SDK ships libcurl, whose licence is the curl
licence, MIT-style). To build either by hand, crypto for instance: `clang++ -std=c++17 -Wall -Wextra
-fsanitize=address,undefined -g $(pkg-config --cflags libcrypto) crypto.cpp
$(pkg-config --libs libcrypto)`.

The sync rule is the testlab discipline: the recipe functions here are quoted
**verbatim** in `book/F-rosetta-cookbook.md` — and three units are quoted
whole by Chapter 8 as well (`Result` and `load_config` from `errors.cpp`,
`channels_doubled` from `expected.cpp`), which each file's banner names.
Editing a recipe on either side means editing every page that quotes it in
the same commit. The `main()` functions are scaffolding
and appear in no listing — change those freely.
