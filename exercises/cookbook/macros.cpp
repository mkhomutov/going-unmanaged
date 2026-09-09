// Chapter 12's preprocessor listings, and the assertions that make them
// claims rather than warnings.
//
// The three broken macros here are included by
// book/12-the-compilation-model.md between their section markers: edit here
// and the page follows. Not a recipe - like standard.cpp, this is a cookbook
// translation unit whose subject is the toolchain rather than a task, and
// Appendix F's index does not list it.
//
// Every broken macro below COMPILES AND RUNS, which is the entire reason the
// chapter shows them. The bad ones are kept beside the fixed ones and both
// are asserted, because the interesting number is not that the fix works but
// that the break is silent: no warning from -Wall -Wextra, no sanitizer
// finding, and an answer that is merely wrong.
#include <cassert>
#include <string>

// --8<-- [start:macro-parens]
#define SQUARE_BAD(x) x * x           // no parentheses: text, substituted
#define SQUARE(x) ((x) * (x))         // the fix, and why every macro wears them
// --8<-- [end:macro-parens]

// --8<-- [start:macro-double-eval]
#define MAX_BAD(a, b) ((a) > (b) ? (a) : (b))   // whichever wins is evaluated TWICE
// --8<-- [end:macro-double-eval]

// --8<-- [start:macro-do-while]
#define BUMP_TWICE_BAD(counter) ++(counter); ++(counter)
#define BUMP_TWICE(counter) do { ++(counter); ++(counter); } while (0)
// --8<-- [end:macro-do-while]

// --8<-- [start:macro-stringify]
#define NAME_OF(expr) #expr           // the one thing no C++ feature can do
// --8<-- [end:macro-stringify]

namespace {
    int calls = 0;

    int next() {
        ++calls;
        return calls;
    }
}

int main() {
    // 1. Missing parentheses. The macro is text: SQUARE_BAD(1 + 2) becomes
    //    1 + 2 * 1 + 2, which is 5 by precedence and not a mistake any
    //    function could make.
    assert(SQUARE_BAD(1 + 2) == 5);
    assert(SQUARE(1 + 2) == 9);

    // 2. Double evaluation. The argument text appears twice in the
    //    expansion, so an argument with a side effect has it twice - and
    //    the parentheses of fix 1 do nothing about this one.
    calls = 0;
    const int winner = MAX_BAD(next(), 0);
    assert(calls == 2);        // called once in the test, once in the result
    assert(winner == 2);       // ...and the value returned is the SECOND call's

    // 3. Two statements wearing one statement's clothes. The `if` takes the
    //    first, and the second runs whatever the condition said.
    int guarded = 0;
    if (false) BUMP_TWICE_BAD(guarded);
    assert(guarded == 1);      // the "unreached" branch incremented it anyway

    int braced = 0;
    if (false) BUMP_TWICE(braced);
    assert(braced == 0);       // do/while(0) makes it one statement again

    // 4. And the capability nothing else has: the preprocessor can quote
    //    source text, which is why every C++ test framework is macros
    //    (Chapter 28) and why __FILE__ and __LINE__ have no rival before
    //    C++20's std::source_location.
    assert(std::string(NAME_OF(a + b)) == "a + b");
    return 0;
}
