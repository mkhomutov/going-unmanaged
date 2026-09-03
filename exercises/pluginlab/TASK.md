# The Plug-in Lab — task card

Full statement, the chapter's reasoning and the reference listings:
**Chapter 40** of the book. Do the tasks cold first; the committed files in
`sdk/`, `plugin/` and `host/` are the reference — no peeking until yours
runs.

Three projects, each configured and built on its own, in this order:

- `sdk/` — the vendor drop: one header, one static helper library, and
  deliberately no CMake config package. **Vendor code: build it, install it,
  never edit it.**
- `plugin/` — yours to write: a MODULE library that locates the installed
  SDK through a hand-written find-module and exports exactly one function.
- `host/` — the stand-in host that loads your module by path and calls its
  entry point. Test scaffolding; read it, do not change what it asserts.

## Tasks

1. **Install the drop.** From a scratch directory of your own:

   ```bash
   cmake -S sdk -B build/sdk -DCMAKE_INSTALL_PREFIX="$PWD/build/prefix"
   cmake --build build/sdk && cmake --install build/sdk
   find build/prefix -type f
   ```

   Two files. Nothing a `find_package(HostSDK CONFIG)` could load.

2. **Write `cmake/FindHostSDK.cmake`** from Chapter 40's description:
   `find_path` for `hostsdk/hostsdk.h`, `find_library` for `hostsdk`,
   `find_package_handle_standard_args`, and an `UNKNOWN IMPORTED` target
   named `HostSDK::Core` carrying the include directory. Configure your
   plug-in with `-DCMAKE_PREFIX_PATH="$PWD/build/prefix"` and confirm that a
   wrong prefix fails at *configure* time with a message naming HostSDK.

3. **Build the module.** `add_library(monitor MODULE ...)`, `PREFIX ""`,
   hidden visibility, an export macro on `Plugin_Entry` and nothing else.
   Then read the export table:

   ```bash
   nm -g --defined-only build/plugin/monitor.so      # Linux / macOS (.dylib on a Mac)
   dumpbin /exports build\plugin\Debug\monitor.dll   # Windows
   ```

   Count the functions. If the SDK's `HostSdk_VersionString` is there, you
   have met the chapter's finding before reading it: hidden visibility
   covers what you compile, not what you link. Make the count one.

4. **Load it.** Build `host/` against the same prefix and run
   `build/host/host build/plugin/monitor.so`. The log line arrives through
   the function table. Then edit the host to pass `api.size =
   sizeof(uint32_t)` and confirm the plug-in returns `-1` instead of reading
   past what it was given.

5. **Write `CMakePresets.json`**, take the prefix from an environment
   variable, and rebuild with `cmake --preset dev` and
   `cmake --build --preset dev`.

6. **Stretch.** Drop the linker option and re-read the export table; drop
   `CXX_VISIBILITY_PRESET hidden` and read it again. Then give the host a
   `Describe` of its own, export the plug-in's by mistake, and reason about
   which the loader picks — before you find out.

The judge is `scripts/build_all.sh`'s pluginlab section: it installs the
drop, builds the module and the host, runs one against the other, and
asserts the export table holds `Plugin_Entry` and nothing else of yours or
the SDK's. Windows readers: the `buildlab-msvc` CI job runs the same three
builds under Visual Studio.
