## Chapter 40 — CMake for the Plug-in

[Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake) built an executable from a library and a `main`, which is the shape of an exercise and of most tools. A plug-in is a different artifact: a binary nobody links against, loaded by name at run time into a process you do not own, against an SDK that arrived as a directory of headers and libraries rather than as a CMake package. Week two of plug-in work is the week that difference lands on the build, and this chapter is the three or four things Chapter 26 did not have to say. The reference lab is `exercises/pluginlab/` — a vendor-style SDK drop, the plug-in that consumes it, and a stand-in host that loads the result — built, loaded and inspected by `build_all.sh` on every push.

### The plug-in is a MODULE library

CMake has three kinds of library, and the third is the one Chapter 26 never needed:

| `add_library(x ...)` | What it produces | Who uses it |
|---|---|---|
| `STATIC` | an archive of object files (`.a`, `.lib`) | linked into something else at build time — Chapter 12's `.lib` |
| `SHARED` | a dynamic library with a public link interface (`.so`, `.dylib`, `.dll` plus its import `.lib`) | linked against at build time, loaded at run time |
| `MODULE` | a dynamic library with no link interface | loaded at run time by name, by `dlopen` or `LoadLibrary` — nothing links it |

A plug-in is `MODULE`. The host will never link against it; it will open the file, look up one symbol by string, and call it. Declaring that in the build buys three things: no import library is generated on Windows (there is nothing to import), no versioned-soname arrangements on Linux, and CMake refuses to let another target `target_link_libraries` it — which is the right refusal, because a plug-in that something links against has stopped being a plug-in. The `PREFIX ""` in the listing below is the other half of the same fact: the host loads `monitor.so`, not `libmonitor.so`, because it is not a library in the linker's sense.

C# had this distinction too, and hid it: an assembly loaded with `Assembly.LoadFrom` is a module, one referenced by the project is a shared library, and the runtime gave both the same file format and the same metadata. Here the loader gets a file and a name, and everything it does not find by name does not exist.

### The SDK that ships no config package

Chapter 27 showed the well-behaved case: an SDK that installs a config package, so `find_package(mathlib CONFIG)` produces an imported target and the consumer links it. Most native SDKs do not do that. What arrives is a directory — `include/` with headers, `lib/` with archives or import libraries, a `bin/` with runtime DLLs, and no CMake support of any kind. Chapter 26 said to locate the pieces by hand "once, in one file"; this is that file.

```cmake
# FindHostSDK.cmake - the imported target, written by hand.
#
# The SDK ships a header and a library and no CMake config package, so the
# consumer builds the target a config package would have generated: locate
# the two files under CMAKE_PREFIX_PATH (or HostSDK_ROOT), and present them
# as HostSDK::Core, carrying its include directory as usage requirements.
# After this file, the rest of the project links HostSDK::Core exactly as it
# would link an SDK that had done this work itself.
find_path(HostSDK_INCLUDE_DIR hostsdk/hostsdk.h)
find_library(HostSDK_LIBRARY NAMES hostsdk)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HostSDK
    REQUIRED_VARS HostSDK_LIBRARY HostSDK_INCLUDE_DIR)

if(HostSDK_FOUND AND NOT TARGET HostSDK::Core)
    # UNKNOWN: static or shared, CMake need not know - the file is the file.
    add_library(HostSDK::Core UNKNOWN IMPORTED)
    set_target_properties(HostSDK::Core PROPERTIES
        IMPORTED_LOCATION "${HostSDK_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${HostSDK_INCLUDE_DIR}")
endif()

mark_as_advanced(HostSDK_INCLUDE_DIR HostSDK_LIBRARY)
```

Three things to read off it. `find_path` and `find_library` search a list of prefixes, and the one that matters is `CMAKE_PREFIX_PATH`: the consumer says `-DCMAKE_PREFIX_PATH=/opt/HostSDK-3.0` at configure time, and no path is ever written into the project — the same mechanism deplab uses for its installed package, which is why a config package and a hand-written find-module look identical from the consuming side. `find_package_handle_standard_args` is what makes `REQUIRED` mean *fail at configure time with a message naming the SDK*, rather than at link time with an unresolved external, which is Chapter 27's pitfall about assuming the build machine's environment. And the imported target is `UNKNOWN`, because the consumer does not care whether the vendor shipped a static archive or an import library; the file is the file.

The C# analogue is the `<HintPath>` in an old-style csproj — a reference to a DLL by location — and the improvement is the same one CMake made everywhere: the location is a configure-time input, and the target that results carries its include directory with it, so `target_link_libraries(monitor PRIVATE HostSDK::Core)` is the whole consumption.

### One symbol, and what "hidden" does not cover

A shared library exports symbols, and by default on Linux and macOS it exports *all* of them: every function in every translation unit, with external linkage, is visible to the loader and to every other library in the process. For a plug-in that is a liability twice over. It is Chapter 30's ABI surface, published by accident — every function name is now a promise. And it is Chapter 27's diamond with the loader as the linker: if your plug-in and the host both contain a function called `Describe`, or both contain a copy of the same third-party library, the dynamic loader picks one per name, and which one is a fact about load order. [Appendix G](G-the-bridge-catalogue.md#appendix-g--the-bridge-catalogue)'s warning that gRPC must be linked with symbols hidden is this paragraph applied to one library.

The discipline is two lines in the build and one macro in the source: hide everything, then mark the one function that is the surface.

```cpp
// The export macro: the one symbol that crosses the boundary wears it, and
// everything else in the module stays hidden (CXX_VISIBILITY_PRESET hidden in
// CMakeLists.txt). Two spellings, one meaning: "this is the plug-in's surface".
#pragma once
#if defined(_WIN32)
#define MONITOR_EXPORT __declspec(dllexport)
#else
#define MONITOR_EXPORT __attribute__((visibility("default")))
#endif
```

```cpp
// monitor.cpp - the plug-in. One exported entry point, everything else hidden.
#include <hostsdk/hostsdk.h>
#include "monitor_export.h"

#include <string>

namespace {
// Internal linkage AND hidden visibility: no symbol of ours leaks into the
// host's process, so a same-named function in the host - or in another
// plug-in - can never be the one the loader picks (Chapter 27's diamond, at
// plug-in scale).
std::string Describe(const HostApi& host) {
    return std::string("monitor loaded against ") + HostSdk_VersionString()
         + ", host api " + std::to_string(host.api_version);
}
}  // namespace

extern "C" MONITOR_EXPORT int32_t Plugin_Entry(const HostApi* host) {
    if (host == nullptr || host->size < sizeof(HostApi)) {
        return -1;                          // an older host: do not read past what it gave us
    }
    if (host->api_version != HOSTSDK_API_VERSION) {
        return -2;
    }
    try {
        host->log(Describe(*host).c_str());
        return 0;
    } catch (...) {
        return -3;                          // nothing escapes into the host's frames
    }
}
```

Every rule from Chapter 30 is in that entry point — `extern "C"` so the name is findable, the size field checked before anything past it is read, `catch (...)` because the frame above is the host's — and the build description around it is the whole plug-in CMakeLists:

```cmake
# The plug-in's build: a MODULE library, an SDK located by a hand-written
# find-module, and a symbol surface of exactly one function. Chapter 40's
# reference; do the chapter's "Try it" from scratch before reading it.
cmake_minimum_required(VERSION 3.16)

project(monitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Match the host's C runtime on Windows before any target exists: a plug-in
# on the debug CRT loaded by a release host corrupts the heap it shares
# (Chapter 26's pitfall). The generator expression picks per configuration.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

# The SDK ships no config package, so cmake/FindHostSDK.cmake writes the
# imported target by hand; this line is what makes find_package find it.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(HostSDK REQUIRED)

# MODULE, not SHARED: a library that exists to be loaded at run time by
# name, that nothing links against - so no import library on Windows, no
# soname dance on Linux, and CMake refuses to let another target link it.
add_library(monitor MODULE monitor.cpp)
target_link_libraries(monitor PRIVATE HostSDK::Core)

set_target_properties(monitor PROPERTIES
    PREFIX ""                            # monitor.so / .dylib / .dll, not libmonitor
    CXX_VISIBILITY_PRESET hidden         # export nothing by default...
    VISIBILITY_INLINES_HIDDEN ON)        # ...inline bodies included
# ...and monitor_export.h marks the one function that IS exported.

# Hidden visibility covers code THIS project compiles. A static library it
# links brings its own: libhostsdk was built with default visibility, so
# without the line below every hostsdk function is exported from the module
# too - and the host, which links the same library, now has two copies of
# each in one process (Chapter 27's diamond, delivered by your plug-in). Tell
# the linker the surface is one name. MSVC needs nothing: it exports only
# what wears __declspec(dllexport).
if(APPLE)
    target_link_options(monitor PRIVATE "-Wl,-exported_symbol,_Plugin_Entry")
elseif(NOT MSVC)
    target_link_options(monitor PRIVATE "-Wl,--exclude-libs,ALL")
endif()

if(MSVC)
    target_compile_options(monitor PRIVATE /W4)
else()
    target_compile_options(monitor PRIVATE -Wall -Wextra)
endif()
```

The paragraph in the middle of that file is the one the first draft of this lab did not have, and it is the chapter's finding. `CXX_VISIBILITY_PRESET hidden` is a *compiler* flag, `-fvisibility=hidden`, and it applies to the code this project compiles. The SDK's helper library was compiled by the vendor, with the default, and when the plug-in links it the linker copies its functions in *with their visibility* — so the first build of `monitor.so` exported `Plugin_Entry` and `HostSdk_VersionString`, and the host, which links the same archive, had two of the latter in one process. Nothing warned. The judge that saw it is the one to keep: `nm -g --defined-only monitor.so` on Linux and macOS, `dumpbin /exports monitor.dll` on Windows, read after every build that touches the surface. The linker options are the fix, one spelling per linker, and MSVC needs none because it never exported anything that did not ask.

> [!TIP]
> **Key principle:** "A plug-in is a MODULE library with one exported symbol — hidden by default, and the export table read back after every build, because hidden covers what I compile and not what I link."

### Generator expressions, once

You have now read `$<...>` three times — `$<INSTALL_INTERFACE:...>` in deplab, `$<$<CONFIG:Debug>:Debug>` above, and you will meet `$<TARGET_FILE:monitor>` the day a test needs the module's path. They all mean the same thing: **a value CMake cannot know at configure time**, because it depends on the generator or the configuration, written as an expression the *generated* build system evaluates. `$<CONFIG:Debug>` is 1 in a Debug build and 0 otherwise — and with a multi-config generator like Visual Studio, both at once, which is why a plain `if(CMAKE_BUILD_TYPE STREQUAL Debug)` is wrong there (Chapter 26's pitfall) and the generator expression is right. `$<BUILD_INTERFACE:x>` is `x` when the target is used from its own build tree and nothing when used from an install; `$<TARGET_FILE:t>` is the full path of `t`'s output, whatever the generator decided to call it. Read them as "decided later", and reach for one whenever an `if` on a configuration variable feels necessary.

### The runtime library, on Windows

Chapter 26 named the pitfall — Debug and Release use different C runtimes on Windows, and a plug-in on one loaded by a host on the other corrupts the heap they share — and gave no CMake spelling for the fix. It is one variable, set before any target is created, and it is a generator expression because the answer differs per configuration:

```cmake
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
```

That expands to `MultiThreadedDLL` (`/MD`) in Release and `MultiThreadedDebugDLL` (`/MDd`) in Debug — the defaults, spelled out so they can be changed: a host that ships against the static runtime (`/MT`) needs the plug-in to drop the `DLL` suffix, and that is a fact you learn from the host's documentation or its `dumpbin /dependents`, not from your own build. The `buildlab-msvc` CI job builds this lab under MSVC for exactly this line.

### Presets — the solution file you never write

Chapter 26's two commands grow options fast: a prefix path, a build type, a compile database, a generator. `CMakePresets.json` next to the CMakeLists records them under a name:

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "dev",
      "displayName": "Developer build against the installed SDK",
      "binaryDir": "${sourceDir}/build/dev",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON",
        "CMAKE_PREFIX_PATH": "$env{HOSTSDK_PREFIX}"
      }
    }
  ],
  "buildPresets": [
    { "name": "dev", "configurePreset": "dev" }
  ]
}
```

`cmake --preset dev` then `cmake --build --preset dev`, and every developer and every IDE that reads presets (Visual Studio, CLion, VS Code's CMake extension all do) gets the same configuration without a wiki page. The SDK's location comes from an environment variable, because it is the one thing that differs per machine — which is the csproj's `$(HostSdkDir)` property, arriving here as `$env{}`. `CMAKE_EXPORT_COMPILE_COMMANDS` is worth turning on everywhere: `compile_commands.json` is what clang-tidy, clangd and every editor's C++ mode read to know your flags, and it is what `build_all.sh` reads to check that a flag reached a file at all.

### The install/export half, finally shown

Chapter 27's try-it asked you to write the producing half of `find_package` and pointed at deplab; the chapter never showed it, and it is the one piece of CMake a plug-in author writes for someone else — the day your plug-in ships a library of its own, or the day you are the vendor. The whole file, comments included, because the comments are where the two easy mistakes are:

```cmake
# The dependency: a standalone project, buildable on its own.
#
# It has to be standalone or two of the three consumption paths cannot work -
# FetchContent clones and configures it as a project, and the install/export
# half below is what makes find_package(mathlib) possible at all.

cmake_minimum_required(VERSION 3.16)

# The VERSION here is the thing step 3 watches move. It reaches the code as a
# compile definition, not as a header the consumer parses.
project(mathlib VERSION 1.0.0 LANGUAGES CXX)

add_library(mathlib src/mathlib.cpp)

# The namespaced alias. Consumers link mathlib::mathlib whichever way they got
# it, so switching between the three paths does not touch their CMakeLists -
# and a typo becomes a configure-time error instead of a linker guess, because
# CMake knows a name with :: in it must be a target.
add_library(mathlib::mathlib ALIAS mathlib)

target_compile_features(mathlib PUBLIC cxx_std_17)

# GNUInstallDirs before the include directories, not just before install():
# CMAKE_INSTALL_INCLUDEDIR is needed by both, and the two have to agree.
include(GNUInstallDirs)

# PUBLIC: consumers compile against this header, so the include directory is
# part of the interface. The two generator expressions are the same directory
# seen from two places - the source tree while building here, the install tree
# after install(). Hardcode the source path and the installed package points at
# a directory that exists only on the machine that built it.
#
# The install half says ${CMAKE_INSTALL_INCLUDEDIR} rather than `include` for
# the same reason, and it is the easier half to get wrong: the literal is
# correct until someone configures with -DCMAKE_INSTALL_INCLUDEDIR=... (a
# normal thing for versioned headers or a distro layout), and then the headers
# go one place while the exported target advertises another. The consumer
# still finds the package, still configures, and fails at the #include.
target_include_directories(mathlib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# PRIVATE: compiles this library and stops there (Chapter 26's distinction).
target_compile_definitions(mathlib PRIVATE MATHLIB_VERSION="${PROJECT_VERSION}")
if(MSVC)
    target_compile_options(mathlib PRIVATE /W4)
else()
    target_compile_options(mathlib PRIVATE -Wall -Wextra)
endif()

# --- the install/export half -------------------------------------------------
# Everything below exists so that find_package(mathlib CONFIG REQUIRED) works
# in another project. This is the "if it ships one" in Chapter 27's
# `find_package(VendorSDK REQUIRED)  # a config package, if it ships one`:
# a config package is a thing the library AUTHOR generates and installs, and
# this is what generating one looks like.
#
# And it is guarded, which is the part that is easy to leave out. install()
# rules belong to the directory that declares them, and a parent's install
# collects its subdirectories' rules - so when this file is reached through
# add_subdirectory (path 1) or FetchContent_MakeAvailable (path 2, which is
# add_subdirectory underneath), these rules become the CONSUMER's. Installing
# that app would then also install libmathlib.a, mathlib's headers and this
# config package into the app's prefix: a private, statically-absorbed
# dependency publishing itself, ready for some third project to find_package
# and link against a build nobody meant to ship.
#
# PROJECT_IS_TOP_LEVEL says this in one word but arrived in CMake 3.21, and
# this file promises 3.16 - so it is spelled out. A library that genuinely
# needs to be installed from inside a consumer would make this an option()
# instead; defaulting it off for a subdirectory build is the same decision.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(CMakePackageConfigHelpers)

    # EXPORT records the target in a set; the install(EXPORT) below writes that
    # set out as importable CMake code.
    install(TARGETS mathlib EXPORT mathlibTargets)
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(EXPORT mathlibTargets
        FILE mathlibTargets.cmake
        NAMESPACE mathlib::      # so the consumer's name matches the alias above
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mathlib)

    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/mathlibConfig.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/mathlibConfig.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mathlib)

    # SameMajorVersion: find_package(mathlib 1.0 REQUIRED) accepts 1.1, refuses
    # 2.0. The policy is the author's to choose and the consumer's to rely on.
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/mathlibConfigVersion.cmake"
        COMPATIBILITY SameMajorVersion)

    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/mathlibConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/mathlibConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mathlib)
endif()
```

Four verbs, in order: `install(TARGETS ... EXPORT set)` records the target, `install(EXPORT set)` writes the set out as CMake code under a namespace, `configure_package_config_file` writes the `Config.cmake` a consumer's `find_package` loads, and `write_basic_package_version_file` decides which versions that consumer's `find_package(mathlib 1.0)` will accept. That is the entire mechanism behind every `find_package(Vendor CONFIG)` that ever worked for you, and the two comments in capitals are the two ways it silently does not: an unguarded install block that publishes a vendored dependency out of the consumer's prefix, and a literal `include` that parts company with `CMAKE_INSTALL_INCLUDEDIR` the first time someone overrides it. `build_all.sh` builds this file and consumes its output on every push, which is the only reason the chapter is allowed to quote it.

> [!TIP]
> **Key principle:** "The SDK's location is a configure-time input — a prefix path or a preset's environment variable — never a path in the project; and an SDK that ships no config package gets one hand-written find-module, in one file, that the rest of the build links like any other target."

### Two things the lab cannot verify, said out loud

Two more CMake tools live in every plug-in shop and neither can be judged by this repository's CI, so they are here as prose with the honesty stated: nothing checks these listings.

**Custom commands.** Chapter 13 warned that SDK builds have extra steps — a resource compiler, a code generator for the plug-in's manifest or its message catalogue. In CMake that is a rule with an `OUTPUT`, a `COMMAND` and a `DEPENDS`, and the output listed as a *source* of the target that needs it, so the build system's dependency graph (Chapter 26's whole reason to exist) includes the generated file:

```cmake
add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/manifest.cpp
    COMMAND VendorResourceCompiler ${CMAKE_CURRENT_SOURCE_DIR}/monitor.rsrc
            -o ${CMAKE_CURRENT_BINARY_DIR}/manifest.cpp
    DEPENDS monitor.rsrc
    COMMENT "Compiling the plug-in manifest")
target_sources(monitor PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/manifest.cpp)
```

The mistake to avoid is `add_custom_target(ALL ...)` for the same job: it runs every build, produces nothing the graph knows about, and the day the generator's output changes the consumers do not rebuild.

**Toolchain files.** Bestiary Shape 4's world compiles on a laptop for a microcontroller, and CMake's spelling of *compile for a different machine* is a file naming the compiler, passed once: `cmake -S . -B build-mcu -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake`. The file sets `CMAKE_SYSTEM_NAME` (`Generic` for bare metal), `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER`, and tells CMake not to try running the programs it compiles to probe the compiler (`CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY`) — because they would run on the wrong machine. Everything else in the project stays the same, which is the point of the two-step model: the toolchain is a configure-time input like the prefix path, and the CMakeLists never names a compiler. The vendor's HAL usually ships one; use theirs.

### In the wild: the plug-in shop's build

- **Presets per host version.** A plug-in that supports three host versions is three configure presets differing in one prefix path, and CI is a matrix over them — the SDK version matrix Chapter 13's checklist told you to ask about, made a file.
- **The host decides the toolset, and the preset records it.** Chapter 27's rule that the SDK wins every conflict shows up here as the `generator` and `toolset` fields of a preset: `"generator": "Visual Studio 17 2022"` with `"toolset": "v143"` when the host was built with that, and nobody has to remember.
- **IDE-native projects are still everywhere.** Chapter 26 said so and it stays true: a plug-in shop with a checked-in `.sln` has the same MODULE, the same exports, the same runtime-library setting, in a properties dialog. Read them with this chapter open — *Configuration Type: Dynamic Library*, *Runtime Library*, the module definition file that lists the exports — and they are the same three decisions.
- **Signing and bundling are build steps.** On macOS a plug-in is often a bundle, and always signed before a host will load it; on Windows, increasingly signed too. These are `add_custom_command` steps at the end of the build, and the honest note from Chapter 13 stands: when the host refuses to load the plug-in and nothing in your code changed, suspect these before the code.

### Pitfalls

- **`SHARED` for a plug-in.** It works, and it generates an import library nobody uses, and it lets a second target link the plug-in — which someone will, and now the plug-in has a consumer that breaks when its surface changes. `MODULE` says what it is.
- **Reading the export table once.** Visibility is a property of every object file and every archive that goes into the link; adding a static library, or a translation unit compiled by a different rule, changes the table without a diagnostic. `build_all.sh` reads it after every build for this lab, and that is the habit to copy.
- **A path in the CMakeLists.** `set(HostSDK_DIR "C:/SDKs/Host 3.0")` works on the machine it was typed on and nowhere else. The prefix path is a configure-time input — a `-D`, an environment variable, a preset — and the project never spells it.
- **Mixing runtime libraries on Windows.** The plug-in must match the host, not the developer's preference; `CMAKE_MSVC_RUNTIME_LIBRARY` is where the match is written, and it is per-configuration, so a generator expression rather than a string.
- **`if(CMAKE_BUILD_TYPE ...)` for a per-configuration decision.** Empty under a multi-config generator, so the `if` is silently false in Visual Studio and Xcode. `$<CONFIG:...>` is decided when the configuration is.
- **Reading a toolchain file's `CMAKE_SYSTEM_NAME` as documentation.** `Generic` means *no operating system*; the compile checks CMake runs at configure time cannot execute their results, and without `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY` the configure fails with an error naming a program that could not run, on a machine that was never meant to run it.

> [!TIP]
> **Key principle:** "A per-configuration decision is a generator expression, not an if on CMAKE_BUILD_TYPE — under a multi-config generator the variable is empty and the if is silently false."

### Try it

The lab is `exercises/pluginlab/` — the SDK drop, the plug-in and the stand-in host, each a project of its own. The task card walks the same road as this chapter, cold; in outline:

1. **Install the drop.** Configure and build `sdk/` with `-DCMAKE_INSTALL_PREFIX` pointing at a scratch prefix, then `cmake --install`. Look at what landed: a header and an archive, and nothing CMake can `find_package`. That is what most SDKs look like on disk.
2. **Write the find-module** from the description, not the listing: `find_path`, `find_library`, the standard-args call, the imported target. Consume it from a one-line CMakeLists with `-DCMAKE_PREFIX_PATH`, and confirm that a wrong prefix fails at *configure* time naming the SDK.
3. **Build the module** with hidden visibility and one exported function, then read the export table — `nm -g --defined-only`, or `dumpbin /exports` — and count. If the count is two, you have met the chapter's finding before reading it; make it one.
4. **Load it** with the stand-in host and watch the log line arrive through the function table. Then shrink the struct the host passes — set `api.size` to `sizeof(uint32_t)` — and confirm the plug-in refuses with `-1` rather than reading past the end; that is Chapter 30's size field doing its job from the receiving side.
5. **Write the preset**, put the prefix in an environment variable, and rebuild with two commands and no flags.
6. **Stretch: break the surface.** Remove the linker option and re-read the export table; remove `CXX_VISIBILITY_PRESET hidden` and read it again. Then give the host a `Describe` function of its own, export the plug-in's by mistake, and reason about which one the loader will choose — before you find out.

`build_all.sh` does steps 1, 3 and 4 on every push, under the cmake probe: it installs the drop, builds the module and the host, runs the host against the module, and asserts that the export table holds `Plugin_Entry` and nothing of the SDK's or the plug-in's own. The `buildlab-msvc` job does the same under Visual Studio, because the runtime-library line is the one thing here that only Windows can check.

---


<!-- nav:begin -->
[← Chapter 39 — The Round Trip Home](39-the-round-trip-home.md) · [Contents](README.md) · [Appendix A — Fundamentals Refresher →](A-fundamentals-refresher.md)
<!-- nav:end -->
