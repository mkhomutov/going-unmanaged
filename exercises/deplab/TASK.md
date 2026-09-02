# Exercise: The Dependency Lab

Consuming a dependency three ways, and watching a version pin actually pin:
**Chapter 27** of the book, the *Try it*, steps 1–3. ~90 min.

You need **no network and no third-party code**. You write the dependency
yourself; the only thing that makes it a dependency is that it is a separate
project with its own `CMakeLists.txt`.

## The task

Extend the Chapter 26 project. Write your own version of all of this in a
directory of your own **before** reading the files here — they are the answer,
the same way `buildlab/CMakeLists.txt` is Chapter 26's answer.

1. **Make a library to depend on.** `include/mathlib/mathlib.h`,
   `src/mathlib.cpp`, and its own `CMakeLists.txt` exporting a
   `mathlib::mathlib` alias target with a **PUBLIC** include directory. Give it
   a `Version()` that returns the version it was built from — step 3 needs
   something observable.
2. **Vendor it.** Consume it with `add_subdirectory` from the app's
   CMakeLists. Then confirm the claim: **the app's own CMakeLists never names
   a header path.** If you can delete a line and the build still finds
   `<mathlib/mathlib.h>`, you have proved PUBLIC did it.
3. **Fetch it.** `git init` the library, commit, `git tag v1.0.0`, and pull it
   into the app with `FetchContent` and a `file://` URL pointing at your local
   repository — a real git remote as far as git is concerned, so nothing here
   is a simulation. Then make a second tag, re-point `GIT_TAG`, and watch the
   build follow it.

Then one more, which the chapter names in a snippet and never asks you to do:

4. **Install it and find it.** `install()` + `install(EXPORT)` +
   `configure_package_config_file` so the library ships a config package, then
   consume it from a third app with `find_package(mathlib CONFIG REQUIRED)` and
   `-DCMAKE_PREFIX_PATH`. This is the producing half of Chapter 27's
   `find_package(VendorSDK REQUIRED)  # a config package, if it ships one` —
   the half you only meet when it is your library someone else consumes.

## What is here

`mathlib/` is the dependency. `app/main.cpp` is the application — **one file,
consumed three ways**, and that is the lesson: it never says where mathlib came
from, and the three `consume-*/CMakeLists.txt` differ only in how they get the
`mathlib::mathlib` target. The link line is identical in all three.

## The judge

`scripts/build_all.sh` builds all three paths and runs each. Two of its checks
are the ones worth stealing for your own attempt:

- **The vendored app's CMakeLists is grepped** for an include path naming
  mathlib. Finding one fails the build — "it works" would not have caught a
  belt-and-braces `include_directories()` that quietly makes PUBLIC redundant.
- **The fetched app is built at both tags and the two outputs must differ.**
  Building once proves the mechanism runs; only building twice proves the
  *pin* is what selected the version.

Without `cmake` on your PATH the whole section prints SKIPPED and stays green;
CI passes `--require-cmake`, which refuses to skip.

## Not here, on purpose

Chapter 27's *Try it* steps 4 and 5 — the ODR diamond, the two link orders, and
predicting which one AddressSanitizer catches. Those stay a hand exercise,
because **the prediction in step 5 is the entire exercise** and a committed
answer would spoil it in the time it takes to read a filename. Do them by hand
with `scripts/check.sh`.

(The chapter's *claim* about them is checked, so it cannot rot:
`scripts/check_platform_claims.sh` builds both link orders and asserts they
disagree and that exactly one is caught. Read it after you have made your own
prediction, not before.)

vcpkg and Conan are also absent, and that is the offline rule doing its job
rather than an oversight — both need a network and a registry, and the chapter
presents them as reading rather than as a lab.
