// tiny_test.h - the three jobs of any test framework: register, check, report.
//
// This file is quoted IN FULL in Chapter 28 ("A test framework in forty
// lines"). Changing it means updating that listing in the same commit - the
// same discipline the Fake* vendor code is held to. Write your own first: the
// chapter's Try it step 1 asks you to build it from the description, not from
// the listing, and comparing afterwards is the point.
#pragma once
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace tiny {

struct Test { std::string name; std::function<void()> body; };

// Function-local static: the registry is created on first use, so a registrar
// running before main in ANY translation unit always finds it alive. A plain
// namespace-scope vector would be a bet on initialization order.
inline std::vector<Test>& Registry() {
    static std::vector<Test> tests;
    return tests;
}

inline int& FailureCount() { static int n = 0; return n; }

struct Registrar {
    Registrar(std::string name, std::function<void()> body) {
        Registry().push_back({std::move(name), std::move(body)});
    }
};

inline void Check(bool ok, const char* expr, const char* file, int line) {
    if (!ok) {
        ++FailureCount();
        std::cout << "    FAILED: " << expr << "\n"
                  << "    at " << file << ":" << line << "\n";
    }
}

inline int RunAll() {
    int failed_tests = 0;
    for (const auto& t : Registry()) {
        const int before = FailureCount();
        t.body();
        const bool ok = (FailureCount() == before);
        if (!ok) ++failed_tests;
        std::cout << (ok ? "  [ ok ] " : "  [FAIL] ") << t.name << "\n";
    }
    std::cout << Registry().size() << " tests, " << failed_tests << " failed\n";
    return failed_tests == 0 ? 0 : 1;   // the EXIT CODE is the result CI reads
}

}  // namespace tiny

// Macros, not functions: only the preprocessor can capture the source text of
// the expression (#expr) and the call site (__FILE__ / __LINE__).
#define CHECK(expr) ::tiny::Check((expr), #expr, __FILE__, __LINE__)

#define TEST(name)                                          \
    static void name();                                     \
    static ::tiny::Registrar registrar_##name(#name, name); \
    static void name()
