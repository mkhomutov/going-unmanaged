# myplugin — the project layout Chapter 26 describes, as a directory to copy

Not an exercise: a starter. Copy it, rename every spelling of `myplugin` —
the project, the include directory, the namespace, the `MYPLUGIN_` options
and the preset's cache variable that sets one of them; a half-rename builds
green with the sanitizers off, which is why `build_all.sh` reads the flag
back from the compile database — and the three commands work.

```bash
cmake --preset dev          # configure: Debug, sanitizers on, compile database
cmake --build --preset dev  # build the library and the test binary
ctest --preset dev          # run the tests; the exit code is the verdict
```

What is where, and the chapter that says why:

| | |
|---|---|
| `CMakeLists.txt` | the root build description; one per directory, the root one adds the others (Chapter 26) |
| `CMakePresets.json` | the configurations by name — `dev` and `release` — the solution file you never write (Chapter 40) |
| `include/myplugin/` | public headers, the ABI surface, included as `<myplugin/x.h>` (Chapters 26, 30) |
| `src/` | `.cpp` files and private headers; nothing here is a promise |
| `tests/` | a second executable with its own `main`, run by CTest (Chapter 28) |
| `cmake/` | find-modules, and the hand-written imported target for an SDK that ships no config (Chapter 40) |
| `third_party/` | vendored dependencies, each with a README naming its version (Chapter 27) |
| `.clang-format` | layout, enforced by a tool (Appendix A.8) |
| `.clang-tidy` | the analyzers and their severities, names included |
| `.gitignore` | `build/`, and every generated directory as one arrives |
| `LICENSE`, `NOTICE` | not copied here — this directory travels under the repository's own; a copy of the tree needs its own, the terms unmodified (Chapter 26) |
| `build/` | generated, ignored, the only directory `rm -rf` should ever touch |

The repository holds this directory to its own rules: `build_all.sh`
configures, builds and runs it under the cmake probe, and CI runs
`clang-format --dry-run --Werror` and `clang-tidy` over its sources — the
two checks Chapter 26 and Appendix A.8 name and, until this directory
existed, nothing in the repository ran.
