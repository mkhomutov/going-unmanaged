## Chapter 27 — Dependency Management

In C# you type `dotnet add package Foo`, and everything after that is somebody else's problem. One registry that everyone uses, one package format, transitive dependencies resolved for you, and versions reconciled at build time, so two libraries wanting different versions of a third is a resolver's problem rather than yours. You have almost certainly never thought about *how* that works, which is the highest compliment a toolchain can be paid.

C++ has none of it. No official registry, no standard package format, no standard way to declare a dependency, and — the root of everything else in this chapter — no standard ABI. Getting a library into your build is a decision you make, not a command you run, and it is worth understanding why before learning the mechanics.

### Why there is no NuGet

A NuGet package ships IL. IL is portable across compilers (there is effectively one), across standard library versions, and across Debug and Release, because the runtime is the thing that varies and it stays compatible on purpose. One artifact, everyone's machine.

A compiled C++ library is not portable in any of those directions. To link against a binary it must match your **compiler and its version** (pre-2015 MSVC changed name mangling and class layout at every major version, and GCC and Clang builds are not interchangeable in general — though note that MSVC has promised binary compatibility across every toolset from v140 (Visual Studio 2015) to v145 (2026), a vendor guarantee rather than a language rule, and one that excludes anything built with `/GL` or `/LTCG` even across minor updates), your **standard library implementation** (libstdc++ and libc++ have different `std::string` layouts — Chapter 12's DLL-boundary note, made expensive), your **configuration** (Debug and Release use different C runtimes on Windows, and mixing them corrupts memory), your **architecture**, and your **platform**. Publishing prebuilt binaries means publishing the cross product of all of those, and it is never complete.

So the C++ world mostly gave up on shipping binaries and ships **source** instead, which you build with your toolchain, so everything matches by construction. That single fact explains why the answer to "how do I add a library" is usually "get its source into your build" rather than "install a package".

> [!TIP]
> **Key principle:** "C++ libraries ship as source because binary compatibility is per-ecosystem and fragile — it depends on the compiler, the standard library, the configuration and the architecture all agreeing — so I build my dependencies with my toolchain and they match by construction."

### What a dependency physically is

Chapter 12's trio, again, because everything here reduces to it: **headers** for the compiler, a **library file** for the linker, and possibly a **runtime binary** for the loader. Every strategy below is a different answer to "how do those three arrive, and who builds them".

### The four strategies

**1. Vendored source — copy it into your repository.** The library's source lives in your tree, typically under `third_party/`, and builds as part of your build.

```cmake
add_subdirectory(third_party/mathlib)
target_link_libraries(app PRIVATE mathlib::mathlib)
```

Unfashionable, and by a distance the most common thing you will meet in native SDK work. Everything is in one repository, the build has no network dependency, the code you shipped last year still builds today, and you can patch the library when you must. The costs are real too: updates are manual, your diff history mixes your code with theirs, and "we patched it locally" is discovered years later by whoever tries to upgrade. Record the version and any local patches in a README next to it — this is the single most valuable ten minutes in the whole chapter.

**2. Fetch pinned source at configure time.** CMake can clone a dependency for you, which is vendoring without the copy in your repository.

```cmake
include(FetchContent)
FetchContent_Declare(mathlib
    GIT_REPOSITORY https://example.invalid/mathlib.git
    GIT_TAG        v1.4.2)          # a TAG or commit hash - NEVER a branch
FetchContent_MakeAvailable(mathlib)

target_link_libraries(app PRIVATE mathlib::mathlib)
```

A git submodule achieves the same thing by a different route. Both give reproducible builds *only* if you pin to an immutable tag or commit: pin to `main` and your build changes under you one morning, which is the C++ equivalent of a floating package version, minus the lock file that would have saved you.

**3. A package manager.** vcpkg and Conan are the two that matter, and they are the closest thing to the C# experience — a manifest in your repository, a resolver, and a cache of builds for your toolchain. vcpkg's manifest looks familiar:

```json
{
  "name": "my-app",
  "version": "0.1.0",
  "dependencies": [ "fmt", "zlib" ],
  "builtin-baseline": "<40-character commit hash of the vcpkg registry>"
}
```

That last field is the one newcomers leave out and later regret. The dependency list says *which* libraries; the baseline says *which registry state* their versions are read from, and it is the actual pin — the lock file C++ otherwise does not have. Omit it and the versions you get are whatever your clone of vcpkg happens to be sitting at, which is the floating-version problem in a different costume. `vcpkg x-update-baseline --add-initial-baseline` writes it for you.

Consumed by pointing CMake at the package manager's toolchain file, after which `find_package(fmt CONFIG REQUIRED)` works and you link `fmt::fmt` like any other target. Note what it is still doing underneath: building those libraries *from source* with your compiler, then caching the result. The manifest is the convenience; the compile is still happening.

**4. Provided by the system or the SDK.** The vendor's installer put headers and libraries somewhere, and you locate them:

```cmake
find_package(VendorSDK REQUIRED)                    # a config package, if it ships one
target_link_libraries(app PRIVATE VendorSDK::Core)
```

This is the normal case for the SDK your product is built on, and it is the one dependency you do not get to choose or version — see below.

### Header-only libraries, and why C++ has so many

A header-only library has no library file and no build integration: you put the headers somewhere on the include path and you are finished. That sidesteps the entire ABI problem — nothing was compiled by anyone else, so nothing can mismatch — which is why the header-only style is far more popular in C++ than its ergonomics alone would justify. It is a workaround for the packaging story, and a good one.

The cost is compile time, paid by every translation unit that includes it, forever (Chapter 12's header-hygiene material). For a small utility that trade is obviously right. For something large it is how a five-minute build becomes a forty-minute build.

### The batteries C# included

In C#, parsing JSON, reading XML and calling an HTTP endpoint are not dependency decisions — `System.Text.Json`, `XDocument` and `HttpClient` are in the box. The C++ standard library has none of them: no JSON, no XML, and no HTTP client — the last for a starker reason than the first two.

> [!NOTE]
> **Surprise for C# devs:** the C++ standard library has no networking at all — not just no `HttpClient`, no *sockets*. Anything that touches the network is a dependency you choose, add and build yourself, under the strategies above.

So your first real dependency is very likely one of these, and the ecosystem has largely converged on names. For JSON, **nlohmann/json** ("JSON for Modern C++") is the default — a single header, which makes it the poster child for the header-only story above and the usual first test of whichever strategy your team runs; **RapidJSON** is the one you meet when parsing speed is the point. For XML, **pugixml** or **TinyXML-2** — though notice where you actually meet XML in this line of work: `.vcxproj` and `.csproj` files, Qt `.ui` files — formats you read and let tools own, rarely ones you parse yourself. For HTTP, the workhorse is **libcurl** — a C API straight out of Chapter 16's Bestiary, opaque handle, `setopt` calls, error codes and all, so consuming it is exactly the skill Chapters 17 and 18 train — with **cpr** as the common C++ wrapper over it, and **Boost.Beast** where a codebase is already committed to Boost.

None of them appears in this book's exercises, and that is the offline, standard-library-only rule doing its job rather than an oversight. What the book trains is the part that transfers: the strategies above decide how one of these lands in your build, and the Bestiary shapes describe the API you will meet when it does.

### The diamond, and why C++ makes it dangerous

Two of your dependencies each want a different version of a third. In C# this is routine: NuGet unifies each package to one version at restore, and the runtime binds exactly what `deps.json` records. (On .NET Framework an app.config binding redirect papered over the rest; modern .NET has no such mechanism and does not need one.) Loud when it fails, and it usually does not fail.

In C++ there is no resolver, no unification, and no redirect. If two versions of the same library reach one binary, you have violated the One Definition Rule — the same class name with two different definitions in one program — and the standard's response is that your program is ill-formed, no diagnostic required. Here is what that actually looks like. A `Config` struct gains a field in v2, *before* the existing one:

```cpp
// v1.h
struct Config { int timeout; };
inline int GetTimeout(const Config& c) { return c.timeout; }
```

```cpp
// v2.h
struct Config { int retries; int timeout; };   // the new field went FIRST
inline int GetTimeout(const Config& c) { return c.timeout; }   // byte-identical to v1's
```

One part of the program is compiled against v1, the rest against v2, and they are linked together. The linker says nothing at all — it exits 0 with no diagnostic, because `GetTimeout` is inline, so it appears in both object files as a mergeable symbol and the linker does exactly what it is designed to do: keeps one, discards the other. Which one survives depends on link order. Running it:

```text
$ c++ libpart.o main.o -o demo   &&  ./demo
v2 caller sees: 999              <- asked for timeout (30), got retries
v1 lib  sees: 30

$ c++ main.o libpart.o -o demo   &&  ./demo      # only the ORDER changed
v2 caller sees: 30
v1 lib  sees: 1809115664         <- garbage; varies run to run
```

Both builds are silent. Both produce a working-looking program. The answer depends on the order the object files were listed, and at `-O2` the compiler may inline both copies and hide the problem entirely — so it can pass every test on the build machine and fail in the shipped configuration. This is the same mechanism as Chapter 23's seventh breakage, arriving through your dependency graph instead of a stale object file.

There is no tool that will reliably tell you this happened, and the word doing the work there is *reliably*. Build the two link orders above under `-fsanitize=address,undefined` — this handbook's own flags — and the sanitizer catches exactly one of them. In the second order the surviving `GetTimeout` reads offset 4 of a struct that is only four bytes long, which is a stack overread, and ASan aborts with a clear report naming the function. In the first order nothing is out of bounds at all: the caller asks for `timeout`, receives `retries`, and every byte read was inside the object. Clean sanitizer log, exit 0, wrong number. The loud direction gets caught; the quiet one ships.

The defences are therefore structural: **one version of each library in a binary, decided deliberately**, and — where a library must genuinely be private to a component — hide it behind an interface that exposes none of its types (Chapter 12's DLL boundary material; nothing whose layout the compiler chose should cross it).

> [!TIP]
> **Key principle:** "Two versions of one library in a binary is an ODR violation, and it is silent — no resolver, no binding redirect, no diagnostic. One version per binary, decided on purpose."

### In the wild: the dependency you do not control

Your product's SDK is not a dependency you manage. Its version is dictated by the host application or the device firmware you support, its build configuration must match the host's, and upgrading it is a project rather than a line in a manifest. Chapter 13's setup checklist is the practical form of this. What follows from it:

- **The SDK wins every conflict.** If the SDK requires a particular toolset version, that is your toolset version, and every other dependency must build under it.
- **What you can add is a policy question, not just a technical one.** Many teams working on shipped native software require review before a new dependency lands — licence compatibility (does it ship in a commercial product?), security surface, build-time cost, and who updates it when it breaks. Ask what the process is before you propose adding something; "I checked the licence" is a good sentence to say early.
- **Prefer the standard library, then a header-only utility, then a built dependency,** in that order. This book's exercises use the standard library only, and that constraint is a reasonable default in production too — every dependency is a thing you will one day have to build on a machine you have not met.

### Pitfalls

- **Pinning to a branch.** `GIT_TAG main` is not a version. Use a tag or a commit hash, always.
- **Mismatched configuration.** Linking a Release-built dependency into a Debug build on Windows (or the reverse) produces link errors, or worse, links and corrupts the heap. Build dependencies in the same configuration as the thing consuming them.
- **Letting a dependency's headers into your public headers.** Do that and it stops being your private choice — every consumer of your library now needs it on their include path, and you cannot change it without breaking them. Keep third-party types out of your interface (PRIVATE rather than PUBLIC in Chapter 26's terms).
- **Assuming the build machine's environment.** "It works on my machine because the SDK is installed in the default location" is how a build breaks for the next person. Locate dependencies explicitly; fail loudly at configure time with `REQUIRED` rather than mysteriously at link time.
- **Vendoring without recording the version.** A `third_party/` directory with no note of what version it holds, or what was patched, is a small permanent tax on everyone who comes after you.

> [!TIP]
> **Key principle:** "I pin dependencies to a tag or a commit, never a branch — and I write down the version and any local patch next to the vendored code."

### Try it

Extend the Chapter 26 project. You need no network and no third-party code: write the dependency yourself, then consume it four ways.

1. **Make a library to depend on.** A `mathlib` directory with `include/mathlib/mathlib.h`, `src/mathlib.cpp`, and its own `CMakeLists.txt` exporting an `add_library(mathlib::mathlib ALIAS mathlib)` target with a PUBLIC include directory. It is a dependency now because it is a separate project, not because it came from anywhere.
2. **Vendor it.** Consume it with `add_subdirectory` from the app's CMakeLists. Confirm the app's own CMakeLists never names a header path.
3. **Fetch it.** `git init` the library, commit, `git tag v1.0.0`, and pull it into the app with `FetchContent` and a `file://` URL pointing at your local repository. This exercises the real mechanism offline. Then re-point `GIT_TAG` at a second tag and watch the build follow it.
4. **Reproduce the diamond.** Write the `v1.h` / `v2.h` `Config` above, compile one .cpp against each, link them, and run. Then swap the object file order on the link line and run again. Note the two different wrong answers, and that nothing warned you at any point. Then rebuild at `-O2` and watch the symptom change.
5. **Now point the sanitizer at it.** Back at `-O0`, rebuild both link orders with `-fsanitize=address,undefined` and run each. Before you do: predict which one ASan catches. One aborts with a stack overread naming `GetTimeout`; the other prints its wrong answer and exits 0 with nothing to report at all. Working out *why* that asymmetry falls the way it does is the whole exercise — and it is the reason the pitfall here is structural rather than a tool you can add to CI. Steps 4 and 5 are the most valuable fifteen minutes in the chapter, and a strong candidate for your notes file.

---


<!-- nav:begin -->
[← Chapter 26 — Build Systems and CMake](26-build-systems-and-cmake.md) · [Contents](README.md) · [Chapter 28 — Testing →](28-testing.md)
<!-- nav:end -->
