## Appendix J — The CMake Catalogue

[Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake), [Chapter 27](27-dependency-management.md#chapter-27--dependency-management) and [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) teach CMake the way the rest of the book teaches everything: in the order the job presents it, each verb arriving with the problem it solves. This page is the other shape, for the moment the verb is already in your head — a colleague's CMakeLists, a vendor's sample, an error naming a command you have not met — and you need the page that owns it in fifteen seconds. It is [Appendix G](G-the-bridge-catalogue.md#appendix-g--the-bridge-catalogue)'s shape applied to the build: a lookup table first, then the handful of tools the chapters never needed, each priced, and a decision table.

One honesty note before the table. Every listing in the three chapters is built by `build_all.sh`; the entries in the second half of this page are not, except the first, because a DLL copy step or a cross-compile is a fact about a machine the CI matrix does not have. Read the rest as Chapter 40 reads its toolchain-file section — prose with the honesty stated: nothing checks it but you, the day you need it.

### The verbs, and the page that owns each

| You met | It means | Owned by |
|---|---|---|
| `cmake -S . -B build` then `cmake --build build` | configure, then build — CMake generates the build system and something else runs it | [Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake) |
| `cmake_minimum_required`, `project(... VERSION ... LANGUAGES CXX)` | the floor, and the name and version that `PROJECT_VERSION` and a config package read back | [Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake) and [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |
| `CMAKE_CXX_STANDARD`, `CMAKE_CXX_STANDARD_REQUIRED`, `CMAKE_CXX_EXTENSIONS`, `target_compile_features(... cxx_std_17)` | the standard, stated once — CMake spells `-std=` and `/std:` for you | [Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake) and [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |
| `add_executable`, `add_library` (`STATIC`, `SHARED`) | the artifacts Chapter 12's trio links and loads | [Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake) |
| `add_library(... MODULE)`, `PREFIX ""` | the plug-in: loaded by name, linked by nothing | [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |
| `add_library(... INTERFACE)` | a target with no sources that carries requirements — the sanitizer flags as a target | [Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake) |
| `add_library(ns::name ALIAS name)` | the namespaced name a consumer links whichever way it got the library | [Chapter 27](27-dependency-management.md#chapter-27--dependency-management) and [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |
| `add_library(... UNKNOWN IMPORTED)`, `IMPORTED_LOCATION`, `INTERFACE_INCLUDE_DIRECTORIES` | a library you did not build, presented as a target | [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |
| `target_include_directories`, `target_link_libraries`, `target_compile_options`, `target_compile_definitions`, `target_link_options` | requirements attached to a target, not a project | [Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake) |
| `PRIVATE`, `PUBLIC`, `INTERFACE` | who else inherits the requirement: nobody, my consumers too, only my consumers | [Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake) |
| `option(NAME "..." OFF)`, `-DNAME=ON` | a configure-time switch, and how it reaches a compile definition | [Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake) |
| `if(MSVC)` ... `else()` | the flag vocabulary CMake does not abstract: warnings, sanitizers | [Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake) |
| `CMAKE_BUILD_TYPE`, `--config Debug` | one configuration per build directory, or all of them at once under a multi-config generator | [Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake) and [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |
| `$<CONFIG:Debug>`, `$<BUILD_INTERFACE:...>`, `$<INSTALL_INTERFACE:...>`, `$<TARGET_FILE:t>` | decided later — a value the generated build evaluates, never the configure step | [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |
| `CMAKE_MSVC_RUNTIME_LIBRARY` | `/MD` versus `/MDd`, matched to the host, per configuration | [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |
| `add_subdirectory` | vendored source built as part of this build | [Chapter 27](27-dependency-management.md#chapter-27--dependency-management) |
| `include(FetchContent)`, `FetchContent_Declare`, `FetchContent_MakeAvailable`, `GIT_TAG` | pinned source cloned at configure time — a tag or a hash, never a branch | [Chapter 27](27-dependency-management.md#chapter-27--dependency-management) |
| `find_package(X CONFIG REQUIRED)`, `CMAKE_PREFIX_PATH` | a package that installed its own config file, located by prefix | [Chapter 27](27-dependency-management.md#chapter-27--dependency-management) and [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |
| `find_path`, `find_library`, `find_package_handle_standard_args`, `CMAKE_MODULE_PATH` | the find-module you write when the SDK shipped no config package | [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |
| `install(TARGETS ... EXPORT)`, `install(EXPORT ...)`, `install(DIRECTORY)`, `install(FILES)` | the producing half of `find_package` | [Chapter 27](27-dependency-management.md#chapter-27--dependency-management) and [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |
| `configure_package_config_file`, `write_basic_package_version_file`, `include(GNUInstallDirs)` | the config and version files, and the install directories a distro may override | [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |
| `enable_testing`, `add_test`, `ctest --test-dir build` | the second executable, run by the harness that reads exit codes | [Chapter 28](28-testing.md#chapter-28--testing) |
| `CMakePresets.json`, `cmake --preset dev` | the configurations by name, with the machine-specific part in `$env{}` | [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |
| `CMAKE_EXPORT_COMPILE_COMMANDS`, `compile_commands.json` | the flags per file, for clangd, clang-tidy and any check that asks what reached a file | [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |
| `add_custom_command(OUTPUT ...)`, `target_sources` | a generated file with an edge in the dependency graph | [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |
| `CMAKE_TOOLCHAIN_FILE`, `CMAKE_SYSTEM_NAME Generic`, `CMAKE_TRY_COMPILE_TARGET_TYPE` | compile for a different machine, as a configure-time input | [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |
| `file(GLOB ...)` | the pitfall: sources found at configure time, invisible when added later | [Chapter 26](26-build-systems-and-cmake.md#chapter-26--build-systems-and-cmake) |
| `set(HostSDK_DIR "C:/...")` in a CMakeLists | the other pitfall: a path in the project, correct on one machine | [Chapter 40](40-cmake-for-the-plug-in.md#chapter-40--cmake-for-the-plug-in) |

### What the chapters never needed

Each entry below is a tool a plug-in shop meets that no chapter had a reason to introduce. Each says what it is for, what it costs, and when to reach for it; the first is the one the repository checks, and the note above covers the rest.

**Getting the runtime to the loader.** Chapter 12's trio ends with a runtime binary the *loader* must find, and Chapter 26 said the step was yours — PATH, an explicit copy, or RPATH — without spelling any of the three. The spelling depends on the platform, and it is the one entry on this page the repository does check. On Linux and macOS an executable carries a list of directories the loader searches before the system directories, its *runpath* (an `LD_LIBRARY_PATH` or `DYLD_LIBRARY_PATH` in the environment still wins, which is why the check below unsets both), and CMake writes it at install time from one property — relative to the executable itself, so the whole prefix can move:

```cmake
cmake_minimum_required(VERSION 3.16)
project(rpathlab LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The runtime half of Chapter 12's trio: a SHARED library the executable
# needs at load time, installed beside it.
add_library(telemetry SHARED src/telemetry.cpp)
target_include_directories(telemetry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_executable(report src/main.cpp)
target_link_libraries(report PRIVATE telemetry)

# Where the loader looks, written into the executable at install time:
# relative to the executable itself, so the prefix can move as a whole.
# $ORIGIN is the executable's directory on Linux, @loader_path on macOS;
# Windows has no such field - there the library is copied beside the
# executable instead (see the catalogue).
include(GNUInstallDirs)
if(APPLE)
    set_target_properties(report PROPERTIES INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
elseif(UNIX)
    set_target_properties(report PROPERTIES INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
endif()

install(TARGETS telemetry report
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
```

`build_all.sh` generates that project into a temporary directory under the cmake probe, installs it, and runs the installed executable from a directory that is not the prefix — then installs it again with `-DCMAKE_SKIP_INSTALL_RPATH=ON` and asserts the same run *fails to load*, because a step that only ever succeeds proves nothing about what the runpath did. (In the build tree CMake writes a runpath for you, which is why the bug appears only in the installed copy.) The symptom, when it arrives, is a program that runs from the build tree and dies on the customer's machine before `main` — `dyld: Library not loaded` on macOS, `error while loading shared libraries` on Linux, *the code execution cannot proceed because X.dll was not found* on Windows. Windows has no runpath: the loader searches the executable's own directory, then the system directories, then `PATH`, so the library is *copied* beside the executable — `$<TARGET_RUNTIME_DLLS:report>` (CMake 3.21) names the `SHARED` targets a target links whose location CMake knows, and an `add_custom_command(TARGET report POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:report> $<TARGET_RUNTIME_DLLS:report> COMMAND_EXPAND_LISTS)` inside an `if(WIN32)` puts them there (the expression is empty elsewhere, and an `UNKNOWN IMPORTED` target from a find-module like Chapter 40's is not among what it lists — that DLL you copy by name). For a plug-in the host's directory is the search path, and where the vendor's DLL goes is the bundling step Chapter 40's pitfalls name. **Price:** an install prefix that must keep its shape; a DLL copy step that runs every build. **When:** the moment the runtime half of the trio is a library you or the vendor built, which for a plug-in is usually day one.

**Precompiled headers.** `target_precompile_headers(monitor PRIVATE <vector> <string> <hostsdk/hostsdk.h>)` compiles the named headers once per target and reuses the result for every translation unit, which is the MSVC `pch.h` you may remember, made portable and generated — and `REUSE_FROM` lets several targets share one. **Price:** every translation unit in the target now depends on every header in the list, so a change to one recompiles all of them, and a header that was included through the PCH but not through the file compiles in the project and fails in a consumer that has no PCH. **When:** the build is slow and the profile of it says the same vendor header is parsed two hundred times; not before measuring.

**Link-time optimization.** `set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)` — after `include(CheckIPOSupported)` and `check_ipo_supported()`, because not every toolchain can — makes the optimizer see the whole program at link time, which is where the devirtualization Chapter 5 mentioned comes from, and where the inlining Chapter 37 warned about starts reaching across translation units. **Price:** longer links, and an ABI hazard Chapter 27 named: objects built with `/GL` (Chapter 27's exclusion) or `-flto` are not linkable by a different toolchain version, so a static library shipped that way ties its consumers to your compiler. **When:** a Release build you measured, never a library you ship.

**ExternalProject.** `FetchContent` pulls a dependency's source into *your* configure step and builds it as part of your build, which requires it to be a CMake project that tolerates your flags. `include(ExternalProject)` and `ExternalProject_Add` run the dependency's own configure and build as a *step of your build* — its own build system, its own flags, its own install prefix — and hand you back files, not targets, so you write the imported target by hand (Chapter 40's find-module shape). **Price:** the dependency's targets are invisible at your configure time, and a wrong path fails at build time, as a missing file the build tool names, rather than at configure. **When:** the dependency is not CMake, or must not see your flags — a vendor library with its own Makefile, an autotools project.

**CTest, past `add_test`.** Chapter 28 registered the test binary and stopped. Two properties carry the rest of that chapter's lesson into CI: `set_tests_properties(buffer_test PROPERTIES ENVIRONMENT "UBSAN_OPTIONS=halt_on_error=1")` so a UBSan finding fails the test rather than printing and exiting 0, and `TIMEOUT 60` so a hang — Chapter 38's whole subject — fails with a name instead of stopping the job. `ctest --output-on-failure` prints a failed test's output and nothing from the passing ones. The discovery helpers — `gtest_discover_tests` from CMake's own GoogleTest module, `catch_discover_tests` from the CMake file Catch2 ships — turn each test case into its own CTest entry, which is what makes the IDE test tree of Chapter 28's opening paragraph appear. **Price:** the properties live in the CMakeLists, so an option changed in CI and not there is silently not applied, and `TIMEOUT` is per test, so a slow runner fails a passing test with a name that says hang. **When:** the day the test binary runs anywhere but your terminal.

**CPack.** `include(CPack)` after the `install()` rules turns them into an installer or an archive — `cpack -G ZIP`, `TGZ`, `NSIS` on Windows, `DragNDrop` on macOS — from the same description that `cmake --install` reads. **Price:** the code-signing and notarization a host demands before it will load a plug-in are not CPack's job and sit outside it as Chapter 40's custom commands; a host that documents an installer format documents its own tooling for it, which leaves CPack the archive. **When:** you ship to people who do not build.

**Unity builds and a compiler cache.** `set(CMAKE_UNITY_BUILD ON)` concatenates translation units before compiling them, which speeds a cold build and breaks the assumption Chapter 12 rests on — two files' anonymous namespaces and `static` helpers now share one translation unit, and a name that was private to a file collides with its twin in another. `set(CMAKE_CXX_COMPILER_LAUNCHER ccache)` caches object files by the hash of their inputs and costs nothing in correctness. **Price:** a unity collision names a translation unit that exists in no directory; ccache costs a cache directory that grows. **When:** ccache whenever it is installed; unity builds only after the collision class above has been hunted, and never in a build you are diagnosing.

**Reading a configure you do not understand.** `cmake --trace-expand` prints every command as it runs with its variables expanded, `cmake --graphviz=deps.dot` draws the target dependency graph, and `cmake -LAH` lists every cache variable with its help text. The `CMakeCache.txt` Chapter 26 said not to debug is what the third one reads. **Price:** `--trace-expand` is long enough that you redirect it and grep, and none of the three explains a generator expression, which is decided after configure ends. **When:** a configure fails somewhere in a file you did not write, or a flag reaches a target you did not expect.

### The decision table

| You need | Reach for | Think twice about |
|---|---|---|
| The installed executable to find its own shared library | `INSTALL_RPATH` with `$ORIGIN` / `@loader_path`; on Windows a `POST_BUILD` copy of `$<TARGET_RUNTIME_DLLS>` | `LD_LIBRARY_PATH` or `PATH` set in a wiki page |
| A vendor DLL next to a host plug-in | the vendor's documented search location, then the copy step | anything that edits the host's own directory layout |
| A dependency that is not a CMake project | `ExternalProject_Add` plus a hand-written imported target | forcing it through `FetchContent` |
| A pinned CMake dependency | `FetchContent` with a tag or hash (Chapter 27) | `ExternalProject`'s extra ceremony |
| Tests that fail on a sanitizer finding or a hang | `set_tests_properties` `ENVIRONMENT` and `TIMEOUT` | a script that greps the log |
| A faster rebuild | `ccache` first; a PCH after measuring; unity builds last | LTO — it slows the link and buys speed at run time, not at build time |
| A shipping archive | `CPack` from the install rules | a hand-written zip step that drifts from `install()` |
| A Release binary that inlines across files | `CMAKE_INTERPROCEDURAL_OPTIMIZATION` after `check_ipo_supported` | shipping a static library built that way to another toolchain |
| To know why configure did what it did | `--trace-expand`, `-LAH`, `--graphviz` | editing `CMakeCache.txt` |

<!-- nav:begin -->
[← Appendix I — Const-Correctness](I-const.md) · [Contents](README.md)
<!-- nav:end -->
