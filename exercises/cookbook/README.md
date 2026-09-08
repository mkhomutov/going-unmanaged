# cookbook/ — Appendix F's listings, compiled

This directory is not an exercise: there is no task card and nothing to
attempt cold. It holds the recipe listings of Appendix F (*The Rosetta
Cookbook*), one translation unit per domain —

| File | Recipes |
|---|---|
| `files.cpp` | 1, 9, 38, 49 — read and write a whole file; save one without losing the old one (the judge is the inode: a save must replace the file, not rewrite it); read a large file without copying it (the judge is a replaced `operator new`, both forms, counting zero allocations across the mapping, every byte compared, the file deleted under the live mapping on POSIX and the descriptor checked closed) |
| `strings.cpp` | 2–5, 17, 23, 44–45 — split, join, build, format; the UTF-8/UTF-16 boundary; the empty string that is not null; a pattern matched with `std::regex` (anchored, the capture read back as a number, both refusals asserted); trim, compare ignoring case, prefix and suffix over `string_view` (the byte-wise compare asserted to differ on `ü`/`Ü`) |
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
| `crypto.cpp` | 36–37, 47–48 — hash bytes; seal bytes for a reader in C#; derive a key from a password (PBKDF2) or a secret (HKDF); sign and verify bytes (HMAC-SHA-256, the verify in constant time): the second TU behind a probe, it links the system's libcrypto through `pkg-config` (nothing vendored) and is held to NIST's, the GCM specification's, RFC 7914's, RFC 5869's and RFC 4231's own test vectors; Recipe 48's `EVP_MAC` needs OpenSSL 3 |
| `http.cpp` | 41, 46 — call an HTTP endpoint; post a JSON body and read a JSON reply: the third TU behind a probe, it links the system's libcurl through `pkg-config` (and, for Recipe 46, includes the vendored nlohmann/json with `-isystem`); its judge needs no network — a `file://` fixture for the callback and the transport's error path, and a small loopback server (POSIX sockets, so not on Windows) for the server's verdict, the redirect follow and the timeout's unit, which for Recipe 46 also echoes the request body and its `Content-Type` back as JSON so the round trip is judged on what the server received, then answers a 400 whose body is JSON (so only the status refuses it), a 200 that is HTML, and a 200 whose JSON lacks the key |
| `database.cpp` | 42 — open a local database and run a query: the fourth TU behind a probe, it links the system's sqlite3 through `pkg-config`; its judge is an in-memory database — the values, the step-code sequence 100, 100, 101, a rollback on a throw, and `sqlite3_close` returning `SQLITE_OK`, which it does only when every statement was finalized |
| `shm.cpp` | 43 — share a buffer with another process: the one listing platform-split end to end (POSIX `shm_open`/`mmap`, Win32 `CreateFileMapping`/`MapViewOfFile`); on POSIX the harness forks — the child writes the frame, the parent reads it under a deadline — then proves the name outlives both mappings and dies on unlink; the `buildlab-msvc` job builds the Win32 half and maps one object twice in one process, the cross-process half stated as unverified there |
| `containers.cpp` | 27 — pre-size a collection: `reserve` against `vector(n)` and `resize` |
| `flags.cpp` | 31–32 — a feature flag read once and kept as a member; a `[Flags]` enum as an `enum class` with its operators |
| `ownership.cpp` | 33–34 — an owned object as a field, and who disposes it; an object too big for the stack |
| `standard.cpp` | Appendix K's probe, not a recipe — which standard the compiler was told to speak (`__cplusplus`, and `_MSVC_LANG` where `__cplusplus` lies) and which of the book's named features this toolchain's library actually ships, one feature-test macro per line; built at C++17 and C++20 with its readings asserted, at C++23 under the `expected.cpp` probe, refused at C++14 by its own `static_assert`, and by the `buildlab-msvc` job with and without `/Zc:__cplusplus` |
| `watch.cpp` | 40 — notice a file changed: the polling watcher, judged by a bounded wait, a restored-older-timestamp change, and silence after the join; built under TSan as well, since it owns a thread |
| `cmake/CMakeLists.txt` | not a recipe: Appendix J's *A library the system provides* — the three probed TUs built a second way, through `find_package(SQLite3)` (with the alias CMake 4.3's rename of its target calls for), `find_package(OpenSSL)` and `pkg_check_modules(... IMPORTED_TARGET libcurl)`, each judge run under CTest; quoted whole on that page, banner-stripped, and held both ways by `check_verbatim.sh` |

— each `.cpp` with a `main()` that asserts what its recipes claim.
`scripts/build_all.sh` builds and runs all of them on every push, so a recipe
that stops being true stops being green — all but one under the canonical
flags, which pin C++17 (`standard.cpp`, Appendix K's probe, is built at
those flags and again with the standard raised, because its readings are
the claim). `expected.cpp` is C++23, the one file here cut by
standard rather than by domain, and it is its own probe: a toolchain that
cannot build it prints SKIPPED, and CI passes `--require-expected` so it can
never skip there. `crypto.cpp`, `http.cpp` and `database.cpp` are the other probes: they need
libcrypto, libcurl and sqlite3, which `build_all.sh` locates through
`pkg-config` and links from the system — dependencies the repository links
rather than vendors — and CI passes `--require-openssl` (on macOS with
`PKG_CONFIG_PATH` pointed at the keg-only OpenSSL), `--require-curl` and
`--require-sqlite` (the Ubuntu runner installs `libcurl4-openssl-dev` and
`libsqlite3-dev`; the macOS SDK ships both, and Homebrew's pkg-config shim
answers for them; libcurl's licence is the curl licence, MIT-style, and
SQLite is public domain). To build either by hand, crypto for instance: `clang++ -std=c++17 -Wall -Wextra
-fsanitize=address,undefined -g $(pkg-config --cflags libcrypto) crypto.cpp
$(pkg-config --libs libcrypto)`.

The sync rule is the testlab discipline: the recipe functions here are quoted
**verbatim** in `book/F-rosetta-cookbook.md` — and three units are quoted
whole by Chapter 8 as well (`Result` and `load_config` from `errors.cpp`,
`channels_doubled` from `expected.cpp`), which each file's banner names.
Editing a recipe on either side means editing every page that quotes it in
the same commit. The `main()` functions are scaffolding
and appear in no listing — change those freely.
