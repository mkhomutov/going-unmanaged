# Exercise: The Plug-in Lab (a MODULE, a find-module, and the export table)

Full statement and the reasoning behind every constraint: **Chapter 40** of
the book, *Try it*. Do it cold from the tasks below — `sdk/` is vendor code,
`plugin/` is the reference to compare with afterwards, `host/` is the judge.
~2 h.

*Trains: Chapter 40 (the MODULE, the hand-written imported target, hidden
visibility and what it does not cover); Chapter 26's two-step model with a
prefix as the configure-time input; Chapter 30's size field, met from the
receiving side.*

Three projects, each configured and built on its own, in this order:

- `sdk/` — the vendor drop: one header, one static helper library, and
  deliberately no CMake config package. **Vendor code: build it, install it,
  never edit it.**
- `plugin/` — yours to write: a MODULE library that locates the installed
  SDK through a hand-written find-module and exports exactly one function.
- `host/` — the stand-in host that loads your module by path and calls its
  entry point. Test scaffolding; read it, do not change what it asserts.

Every command below runs from `exercises/pluginlab/`, and the build trees
go under `build/` here, which git ignores.

## Tasks

1. **Install the drop.**

   ```bash
   cmake -S sdk -B build/sdk -DCMAKE_INSTALL_PREFIX="$PWD/build/prefix"
   cmake --build build/sdk && cmake --install build/sdk
   find build/prefix -type f
   ```

   Two files. Nothing a `find_package(HostSDK CONFIG)` could load.

2. **Write `plugin/cmake/FindHostSDK.cmake`** from Chapter 40's description:
   `find_path` for `hostsdk/hostsdk.h`, `find_library` for `hostsdk`,
   `find_package_handle_standard_args`, and an `UNKNOWN IMPORTED` target
   named `HostSDK::Core` carrying the include directory. Then configure:

   ```bash
   cmake -S plugin -B build/plugin -DCMAKE_PREFIX_PATH="$PWD/build/prefix"
   ```

   Configure once more with a prefix that does not exist and read the
   failure: it must stop at *configure* time with
   `Could NOT find HostSDK (missing: HostSDK_LIBRARY HostSDK_INCLUDE_DIR)`,
   not at link time with an unresolved external.

3. **Build the module.** `add_library(monitor MODULE ...)`, `PREFIX ""`,
   hidden visibility, an export macro on `Plugin_Entry` and nothing else.
   Build it, then read the export table:

   ```bash
   cmake --build build/plugin
   nm -g --defined-only build/plugin/monitor.so      # Linux and macOS: a MODULE is .so on both
   dumpbin /exports build\plugin\Debug\monitor.dll   # Windows
   ```

   Look for names, not a count (Linux lists a few of the C runtime's own
   too). If the SDK's `HostSdk_VersionString` is there, you have met the
   chapter's finding before reading it: hidden visibility covers what you
   compile, not what you link. Make yours and the SDK's disappear.

4. **Load it.** Build `host/` against the same prefix and run it:

   ```bash
   cmake -S host -B build/host -DCMAKE_PREFIX_PATH="$PWD/build/prefix"
   cmake --build build/host
   build/host/host build/plugin/monitor.so
   build/host/host build/plugin/monitor.so --older
   ```

   The first run's log line arrives through the function table. The second
   passes the table an older host would — the same bytes, a smaller `size`
   — and the plug-in must return `-1` instead of reading past what it was
   given.

5. **Write `plugin/CMakePresets.json`**, take the prefix from an environment
   variable, and rebuild with two commands and no flags:

   ```bash
   cd plugin && HOSTSDK_PREFIX="$PWD/../build/prefix" cmake --preset dev && cmake --build --preset dev
   ```

   (The presets file needs CMake 3.21; the CMakeLists beside it asks for
   3.16. Nothing in either file says so — the schema version is the whole
   warning.)

6. **Stretch.** Drop the linker option and re-read the export table; the
   SDK's helper is back. Then drop `CXX_VISIBILITY_PRESET hidden` and read
   it again — on Linux `Describe` appears, mangled; on macOS nothing
   changes until the linker option goes too, because `-exported_symbol` is
   a whole export list. Then give the host a `Describe` of its own, export
   the plug-in's by mistake, and find out which the loader picks: the
   plug-in's own, on macOS and on Linux as built — and read Chapter 40 for
   the two configurations in which that stops being true.

Your judge is the export table in step 3 and the two runs in step 4:
`Plugin_Entry`, nothing of yours, nothing of the SDK's; `0`, then `-1`.
`scripts/build_all.sh`'s pluginlab section holds the reference files to the
same bar on every push, and the `buildlab-msvc` CI job repeats it under
Visual Studio, where `dumpbin` reads the table and the runtime library back.
