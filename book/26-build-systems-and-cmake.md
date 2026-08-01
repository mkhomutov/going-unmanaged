# Part VI — The Real Codebase

The exercises are deliberately small: one file, one command, one thing to learn. A real project is none of those things, and the difference is not more C++ — it is everything *around* the C++. This part covers what a codebase has that an exercise does not, starting with the one you meet on day one: something other than you deciding how the compiler gets called.

---

## Chapter 26 — Build Systems and CMake

Every command in this book so far has been the same shape: hand the compiler a file, get a program. `clang++ -std=c++17 -Wall -Wextra main.cpp -o app` works perfectly — to exactly the size of an exercise. A real project is two hundred source files, three configurations, two platforms, and six libraries, and nobody types that command.

In C# you never met this problem, because you never met it *consciously*. `dotnet build` reads the .csproj; the .csproj is a dozen lines because MSBuild already knows what a C# project looks like — compile everything under the folder, target this framework, restore these packages. The conventions are the product. C++ has no conventions: no standard project layout, no standard package format, no agreement on where headers live. So the build description carries everything, and someone has to write it. That someone is increasingly you.

### What a build system is actually for

Four jobs. Which sources are in the build; which flags each one gets; what links against what, in what order; and — the one that earns the tool its keep — **what needs rebuilding when a file changes**.

That last job is Chapter 23's seventh breakage, solved. Recall it: edit a header, rebuild only one translation unit, link, and get an ODR violation that no tool reports because the object files disagree about a class's layout and the linker cannot tell. That is not an exotic failure. It is what happens *by default* whenever a build forgets that main.cpp depends on Greeter.h. A build system's central promise is that it never forgets — it scans your `#include`s, records the graph, and rebuilds everything downstream of a touched header:

```text
$ touch Greeter.h
$ cmake --build build
[ 25%] Building CXX object CMakeFiles/greeter.dir/Greeter.cpp.o
[ 75%] Building CXX object CMakeFiles/greet.dir/main.cpp.o    <- main.cpp too
```

`main.cpp` was not edited. It was rebuilt because it includes the header that was. This is the whole reason "clean build fixes it" stops being folklore: with dependencies tracked correctly, you should almost never need one.

### CMake is a generator — get this one idea straight first

Here is the thing that confuses every arrival from the managed world, and it is worth thirty seconds of deliberate attention.

MSBuild *is* the build system. The .csproj goes in, binaries come out, one tool, one step.

CMake is not that. **CMake does not build your code. It reads `CMakeLists.txt` and writes a build system** — a Makefile, a Ninja file, a Visual Studio solution, an Xcode project — and *that* generated thing builds your code. Always two steps: **configure**, then **build**.

| C# | C++ with CMake |
|---|---|
| `.csproj` — the build description | `CMakeLists.txt` — the build description |
| MSBuild — reads it and builds | the *generated* Makefile / Ninja / .vcxproj — builds |
| — | CMake — reads the description and *writes* the above |
| `dotnet build` | `cmake -S . -B build` then `cmake --build build` |

Why tolerate the indirection? Because it is what lets one build description serve every platform: the same CMakeLists gives a Linux developer a Ninja build, a Mac developer an Xcode project, and a Windows developer a .sln they open in Visual Studio and debug normally. A cross-platform SDK ships one build description, and its Windows users never know.

### Your first CMakeLists.txt

Use the Greeter trio from Chapter 23 — `Greeter.h`, `Greeter.cpp`, `main.cpp`, already on your disk in `exercises/buildlab/`. Put this next to them:

```cmake
cmake_minimum_required(VERSION 3.16)

project(greeter LANGUAGES CXX)

# The standard IS abstracted by CMake - it knows each compiler's spelling
# (-std=c++17 vs /std:c++17), so you state the intent once.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # fail loudly rather than silently downgrade
set(CMAKE_CXX_EXTENSIONS OFF)         # plain C++17, not the compiler's dialect of it

# Sources are LISTED, never globbed - see the pitfalls below.
add_executable(greet main.cpp Greeter.cpp)
```

Then the two commands:

```bash
cmake -S . -B build      # configure: generate a build system into build/
cmake --build build      # build: run whatever was generated
./build/greet
```

`-S` is the source directory (where CMakeLists.txt lives), `-B` the build directory. Everything CMake produces goes in `build/`, nothing goes next to your source, and `build/` belongs in .gitignore. That separation is called an **out-of-source build**, and deleting `build/` is the real, total "clean" — the one that actually works.

### Targets are the unit, and flags belong to targets

The above is honest but small. Real projects are built from **targets** — libraries and executables — and modern CMake attaches every requirement to the target that has it, rather than setting global flags and hoping.

```cmake
# The library: the code worth reusing, testing, and shipping.
add_library(greeter Greeter.cpp)

# PUBLIC: anything linking greeter also needs this on its include path,
# because Greeter.h is part of the library's *interface*.
target_include_directories(greeter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# PRIVATE: these compile the library itself and stop there - consumers have
# their own opinion about warnings and shouldn't inherit mine.
if(MSVC)
    target_compile_options(greeter PRIVATE /W4)
else()
    target_compile_options(greeter PRIVATE -Wall -Wextra)
endif()

add_executable(greet main.cpp)
target_link_libraries(greet PRIVATE greeter)   # include dirs arrive automatically
```

`greet` never mentions `Greeter.h`'s location. It links `greeter`, and the PUBLIC include directory travels with it. That propagation is the whole idea, and it has a C# analogue you already know: a `ProjectReference` gives you the referenced project's public surface transitively, without you restating it. **PRIVATE** = I need this to build. **PUBLIC** = I need it, and so does anyone who uses me. **INTERFACE** = I don't need it, but my consumers do (the header-only-library case).

Notice also what the `if(MSVC)` is telling you: CMake abstracts the *build*, not the compiler's flag vocabulary. Standards, optimization levels, and debug info are abstracted; warning flags and sanitizer flags are not — you write both dialects yourself.

### Build types, and wiring in the handbook's flags

C++ builds come in configurations, and Chapter 13's warning applies with force: UB hides in Debug and detonates in Release.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug     # -g, no optimization
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release   # -O2/-O3, NDEBUG defined
```

To get this book's canonical sanitizer build as a switch rather than a memory:

```cmake
option(GREETER_SANITIZE "Build with Address and UB sanitizers" OFF)

if(GREETER_SANITIZE)
    if(MSVC)
        # MSVC ships ASan only - there is no /fsanitize=undefined - and its
        # linker pulls in the runtime by itself, so no link options here.
        target_compile_options(greeter PUBLIC /fsanitize=address)
    else()
        # PUBLIC so the flags reach every target that links this one: only the
        # code compiled with them is checked.
        target_compile_options(greeter PUBLIC -fsanitize=address,undefined -g)
        target_link_options(greeter PUBLIC -fsanitize=address,undefined)
    endif()
endif()
```

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DGREETER_SANITIZE=ON
cmake --build build && ./build/greet
```

> **Trap:** sanitizers must be passed to **both** the compiler and the linker. Drop the `target_link_options` line and the compile succeeds, then the link collapses into a wall of undefined symbols: `"___asan_init", referenced from: _asan.module_ctor in main.cpp.o`. Read it with Chapter 12's cause list in hand and it decodes immediately — unresolved external means *a definition is missing at link time*, and the missing definitions here are the sanitizer's runtime library. The flag is how you link that library; passing it only to the compiler instruments the code and never brings the runtime along. It is the same diagnosis as a forgotten SDK .lib, wearing different symbol names.

Note how far that PUBLIC actually reaches: to the targets that link `greeter`, and no further. Add a second library that never links `greeter`, and it compiles uninstrumented — no error, no warning; the code you forgot is simply never checked. Mixing instrumented and uninstrumented objects is legal, which is exactly what makes it dangerous: the build stays green and the coverage quietly shrinks. Past two or three targets, give the flags a target of their own — the INTERFACE case from the previous section, a library that compiles nothing and carries requirements:

```cmake
add_library(sanitizers INTERFACE)   # no sources; the flags ARE the target

if(GREETER_SANITIZE AND NOT MSVC)   # (the MSVC branch moves across unchanged)
    target_compile_options(sanitizers INTERFACE -fsanitize=address,undefined -g)
    target_link_options(sanitizers INTERFACE -fsanitize=address,undefined)
endif()

target_link_libraries(greeter PUBLIC sanitizers)   # and every other target too
```

One place to change when the flag list grows, and a checklist you can read: any target not linking `sanitizers` is a target nobody is checking.

### In the wild: vendor SDKs and IDE-native projects

A well-maintained SDK ships CMake support, and consuming it is two lines:

```cmake
find_package(VendorSDK REQUIRED)                       # locates it, defines targets
target_link_libraries(greet PRIVATE VendorSDK::Core)   # headers + libs + defines
```

That imported target carries its own include directories, its libraries, and any required compile definitions — the Chapter 12 trio (headers for the compiler, .lib for the linker, runtime for the loader) handled in one line.

Plenty of SDKs ship no such thing. Then you locate the pieces by hand — `find_path` for headers, `find_library` for the binary — and wrap them in an imported target yourself so the rest of your build stays clean. Do that once, in one file, rather than scattering include paths through the project.

And the honest note: a great many native SDK shops never use CMake at all. They keep a checked-in Visual Studio solution or Xcode project, because the SDK's own samples ship that way and the team edits build settings in a properties dialog. That is a legitimate, extremely common setup — and the compilation model underneath it is identical, which is why Chapter 12 is the chapter that matters and this one is the tooling on top. If you land in such a team, read their project's settings the way you would read a CMakeLists: which sources, which flags, which libraries, which configurations.

### Pitfalls

- **Globbing sources.** `file(GLOB SOURCES *.cpp)` looks like a labour-saver and is a trap: CMake evaluates it at *configure* time, so a newly added file is invisible until someone re-configures — and the failure lands on whoever pulls your commit, as an unresolved external. `CONFIGURE_DEPENDS` (CMake 3.12+) buys the correctness back by re-globbing on every build, at the price of a directory scan every build — and CMake's own documentation declines to promise it works on every generator, which is a strong hint about how much weight to put on it. List your sources. The diff noise is the point: adding a file to the build should be a visible act.
- **A stale cache.** `build/CMakeCache.txt` remembers your configure-time choices, including the compiler. Changing toolchain or fighting an inexplicable configure result: delete the build directory, don't debug the cache.
- **`CMAKE_BUILD_TYPE` on a multi-config generator.** Visual Studio and Xcode hold all configurations at once and ignore it entirely; there you pass `--config Debug` at *build* time instead. Setting it and seeing no effect is not a bug.
- **Mixing configurations on Windows.** Debug and Release use different C runtimes (`/MDd` vs `/MD`). A plug-in built Debug against a Release host — or against Release SDK libraries — produces link errors, or loads and corrupts memory in ways that look like your bug. Match the host's configuration; this is one of the highest-value entries your notes file will ever hold.
- **Assuming the generated build is the source of truth.** Never edit files inside `build/`. They are output. The next configure overwrites them.

> **Key principle:** "CMake doesn't build my code — it generates the thing that builds my code. Configure, then build: two steps, always."

> **Key principle:** "Requirements belong to targets, not to the whole project — PRIVATE for what I need, PUBLIC for what my consumers need too."

> **Key principle:** "I list source files, never glob them — a file joining the build should be visible in the diff."

### Try it

Write the CMakeLists for the Greeter trio from scratch — file listed, then library-plus-executable — and get `./build/greet` running. Then earn the chapter:

1. **Prove the dependency graph.** `touch Greeter.h`, rebuild, and confirm `main.cpp` recompiles. Then break the graph deliberately: edit the header's class definition, rebuild *only* the library target (`cmake --build build --target greeter`), link by hand, and reproduce Chapter 23's breakage 7. Now you have seen both sides of the promise.
2. **Prove the sanitizer switch.** Configure with `-DGREETER_SANITIZE=ON`, add a deliberate heap overflow to main.cpp, and confirm ASan reports it — the report should name `main.cpp` and the line. Then comment out `target_link_options`, rebuild, and read the resulting link failure until the `___asan_init` symbols make sense. Predict the *stage* before you run it: this is Chapter 23's error-stage triage on a real, non-obvious case.
3. **Generate a native project.** With the full IDE installed, `cmake -S . -B build-ide -G Xcode` (or `-G "Visual Studio 17 2022"`), then open the result and build from the IDE. Same CMakeLists, same code, a project file you never wrote — the payoff for the two-step model. `cmake --help` lists the generators your installation actually offers, which is the honest way to find out what is available on your machine.
4. **Split it.** Move Greeter into a `src/` subdirectory with its own CMakeLists and pull it in with `add_subdirectory`. That is the shape every real project has, and doing it once removes the mystery.

---

---


<!-- nav:begin -->
[← Chapter 25 — Findings from Practice: a Living Log](25-findings-from-practice.md) · [Contents](README.md) · [Chapter 27 — Dependency Management →](27-dependency-management.md)
<!-- nav:end -->
