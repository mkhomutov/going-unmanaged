// Appendix K's probe: which standard is this translation unit being compiled
// as, and which of the features the book names does this toolchain's LIBRARY
// actually ship? Not a recipe - the one cookbook TU with no C# column - and
// the only one built at more than one standard: scripts/build_all.sh builds it
// at -std=c++17, -std=c++20 and (under the C++23 probe) -std=c++23 and asserts
// what each run prints, then asks -std=c++14 to build it and asserts the
// refusal below. The buildlab-msvc job builds it with and without
// /Zc:__cplusplus, because Appendix K's first claim rests on that vendor
// default. Every cpp fence in book/K-the-standards-catalogue.md is included
// from this file between section markers (check_verbatim.sh holds both
// directions), so edit here and the page follows.
#include <cstdio>

// The book's floor, enforced: a toolchain below it is refused with a
// sentence rather than an error novel three headers deep. Read the way a
// portable header must read it: MSVC answers __cplusplus with 199711
// unless /Zc:__cplusplus is on, so _MSVC_LANG is the honest value there -
// the first draft of this file tested __cplusplus alone, and MSVC refused
// it at /std:c++17, which is Appendix K's first trap met before main().
// --8<-- [start:standard-spoken-macro]
#ifdef _MSVC_LANG
#define STANDARD_SPOKEN _MSVC_LANG
#else
#define STANDARD_SPOKEN __cplusplus
#endif
static_assert(STANDARD_SPOKEN >= 201703L, "this book's floor is C++17: pass -std=c++17 or /std:c++17");
// --8<-- [end:standard-spoken-macro]

// <version> is the C++20 header that carries every library feature-test
// macro; all three standard libraries ship it in C++17 mode too, so it is
// the one include this probe needs where it exists. Where it does not, the
// headers that define their own macros stand in.
#if __has_include(<version>)
#include <version>
#else
#include <filesystem>
#include <optional>
#include <variant>
#endif

// One line per question: the macro's name and its value, or "absent". A
// value is the year and month the feature reached its current shape, so
// 201606 under -std=c++20 is not a bug - it says the feature has not
// changed since C++17.
static void report(const char* name, long value) {
    if (value == 0) {
        std::printf("%-32s absent\n", name);
    } else {
        std::printf("%-32s %ld\n", name, value);
    }
}

int main() {
// --8<-- [start:standard-spoken-report]
    // The standard the compiler was TOLD to speak. On MSVC __cplusplus is
    // 199711 unless /Zc:__cplusplus is passed - a lot of code once tested
    // it, so the honest value became opt-in - and _MSVC_LANG carries the
    // real answer whether or not the switch is on.
    std::printf("%-32s %ld\n", "__cplusplus", static_cast<long>(__cplusplus));
#ifdef _MSVC_LANG
    std::printf("%-32s %ld\n", "_MSVC_LANG", static_cast<long>(_MSVC_LANG));
#else
    std::printf("%-32s absent (not MSVC)\n", "_MSVC_LANG");
#endif
// --8<-- [end:standard-spoken-report]

    // Language features: defined by the compiler, so they track -std= exactly.
#ifdef __cpp_if_constexpr
    report("__cpp_if_constexpr", __cpp_if_constexpr);
#else
    report("__cpp_if_constexpr", 0);
#endif
#ifdef __cpp_structured_bindings
    report("__cpp_structured_bindings", __cpp_structured_bindings);
#else
    report("__cpp_structured_bindings", 0);
#endif
#ifdef __cpp_guaranteed_copy_elision
    report("__cpp_guaranteed_copy_elision", __cpp_guaranteed_copy_elision);
#else
    report("__cpp_guaranteed_copy_elision", 0);
#endif
#ifdef __cpp_concepts
    report("__cpp_concepts", __cpp_concepts);
#else
    report("__cpp_concepts", 0);
#endif

// --8<-- [start:library-features]
    // Library features: defined by the standard library's headers, so they
    // track what THIS library has implemented - which can lag the year on
    // the -std= switch by a release or more, and is the reason to ask the
    // macro rather than __cplusplus before reaching for a feature.
#ifdef __cpp_lib_optional
    report("__cpp_lib_optional", __cpp_lib_optional);
#else
    report("__cpp_lib_optional", 0);
#endif
// --8<-- [end:library-features]
#ifdef __cpp_lib_variant
    report("__cpp_lib_variant", __cpp_lib_variant);
#else
    report("__cpp_lib_variant", 0);
#endif
#ifdef __cpp_lib_filesystem
    report("__cpp_lib_filesystem", __cpp_lib_filesystem);
#else
    report("__cpp_lib_filesystem", 0);
#endif
#ifdef __cpp_lib_span
    report("__cpp_lib_span", __cpp_lib_span);
#else
    report("__cpp_lib_span", 0);
#endif
#ifdef __cpp_lib_format
    report("__cpp_lib_format", __cpp_lib_format);
#else
    report("__cpp_lib_format", 0);
#endif
#ifdef __cpp_lib_jthread
    report("__cpp_lib_jthread", __cpp_lib_jthread);
#else
    report("__cpp_lib_jthread", 0);
#endif
#ifdef __cpp_lib_ranges
    report("__cpp_lib_ranges", __cpp_lib_ranges);
#else
    report("__cpp_lib_ranges", 0);
#endif
#ifdef __cpp_lib_erase_if
    report("__cpp_lib_erase_if", __cpp_lib_erase_if);
#else
    report("__cpp_lib_erase_if", 0);
#endif
#ifdef __cpp_lib_expected
    report("__cpp_lib_expected", __cpp_lib_expected);
#else
    report("__cpp_lib_expected", 0);
#endif
#ifdef __cpp_lib_move_only_function
    report("__cpp_lib_move_only_function", __cpp_lib_move_only_function);
#else
    report("__cpp_lib_move_only_function", 0);
#endif
    return 0;
}
