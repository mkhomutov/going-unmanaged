## Chapter 13 — Toolchain Quick Reference

### Compiler invocations

```bash
# clang / gcc (Mac, Linux) - strict + sanitizers + debug info:
clang++ -std=c++17 -Wall -Wextra -Wpedantic \
        -fsanitize=address,undefined -g main.cpp -o app

# MSVC (Windows, Developer Command Prompt):
cl /std:c++17 /W4 /EHsc /Zi /fsanitize=address main.cpp
```

- Treat warnings as your first code reviewer; fix them, don't silence them.
- Sanitizer builds are for development runs; they slow execution ~2x and are not for shipping.
- Debug vs Release matters more than in C#: UB often hides in Debug and detonates in Release. Test both.

These are the commands you type while learning. On a real project you type them once, into a build description, and a tool reproduces them for every file and every configuration — see [Chapter 26](#chapter-26--build-systems-and-cmake).

### Debugging a plug-in inside a host application

- **Visual Studio:** Debug > Attach to Process > the host's .exe (or set your plug-in project's debug command to launch the host directly). Breakpoints in your plug-in code hit once the DLL is loaded.
- **Xcode:** edit the scheme's Run executable to point at the host .app; build-and-run then launches the host with your bundle debuggable.
- Symbols: keep Debug configuration for your plug-in even though the host ships without symbols — your frames are what matter in the call stack.
- If breakpoints don't bind: the loaded plug-in is not the one you just built. Check the file path the host loads vs your build output path — the #1 wasted-afternoon cause.

Learn your debugger's container visualizers (VS: built-in for vector/map; lldb: `frame variable`). Inspecting a vector without them is miserable; with them it's a C#-like experience.

### Vendor SDK setup checklist

- SDK major version usually must match the host application's major version exactly — check before anything else.
- Windows: check the required Visual Studio toolset version in the SDK docs; ABI mismatches produce baffling link and load errors.
- Mac / Apple Silicon: modern hosts expect arm64 or universal binaries; a mismatch shows up as a silent "plug-in won't load".
- Many SDKs have extra build steps beyond compiling (resource compilers, code generators, signing) — if UI elements or metadata don't appear, suspect those steps before the code.
- Plug-in/developer IDs often must be registered with the vendor for real distribution; samples use placeholders. Your employer likely has this handled — ask.

### Mac vs Windows for practice

C++ practice transfers 100% either way (clang + ASan on Mac is first-class). Toolchain muscle memory does not: if the team is a Windows/Visual Studio shop, do the SDK days on Windows so project settings, attach-to-process, and MSVC's error dialect become familiar. Ask the team which platform(s) they develop on before investing setup time.

---

---

