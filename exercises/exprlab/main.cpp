// Chapter 42's judge. Not a solution to attempt: it holds expr.h/expr.cpp to
// a table of hand-computed values (Chapter 34's oracle - no tool knows what
// "wall.width * 2" should be), to error POSITIONS (a parser that says
// "syntax error" is a parser nobody can use), to a depth limit refused at
// N+1 and accepted at N, and to locale independence - the bug that works on
// the bench. Quoted by excerpt in the chapter.
#include "expr.h"

#include <clocale>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

using formula::Error;
using formula::Evaluate;
using formula::Formula;
using formula::ISymbols;

// The injected objects, as a test double (Chapter 28's fake you own): a
// map of objects, each a map of properties, and three functions. The
// formula sees "wall.width"; the split on the dot is this provider's
// decision, and a host adapter would resolve it against a live document.
class ObjectSymbols : public ISymbols {
public:
    void Set(std::string object, std::string property, double value) {
        objects_[std::move(object)][std::move(property)] = value;
    }
    std::optional<double> Value(std::string_view name) const override {
        const auto dot = name.find('.');
        if (dot == std::string_view::npos) return std::nullopt;
        const auto obj = objects_.find(std::string(name.substr(0, dot)));
        if (obj == objects_.end()) return std::nullopt;
        const auto prop = obj->second.find(std::string(name.substr(dot + 1)));
        if (prop == obj->second.end()) return std::nullopt;
        return prop->second;
    }
    std::optional<double> Call(std::string_view name, const std::vector<double>& args) const override {
        if (name == "abs" && args.size() == 1) return std::fabs(args[0]);
        if (name == "max" && args.size() == 2) return std::max(args[0], args[1]);
        if (name == "min" && args.size() == 2) return std::min(args[0], args[1]);
        return std::nullopt;                        // unknown, or the wrong arity: the formula reports it
    }

private:
    std::map<std::string, std::map<std::string, double>> objects_;
};

// The judge: counts failures and sets the exit code - never assert, which a
// Release build compiles away (Appendix H's rule for a harness).
static int g_failures = 0;
#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            ++g_failures;                                                    \
        }                                                                    \
    } while (0)

static bool ValueIs(const std::variant<double, Error>& r, double expected) {
    const double* v = std::get_if<double>(&r);
    return v != nullptr && std::fabs(*v - expected) < 1e-9;
}

static bool ErrorAt(const std::variant<double, Error>& r, std::size_t pos) {
    const Error* e = std::get_if<Error>(&r);
    return e != nullptr && e->pos == pos;
}

int main() {
    ObjectSymbols host;
    host.Set("wall", "width", 4.0);
    host.Set("wall", "height", 2.5);
    host.Set("door", "offset", 1.25);

    // The value table: every expected number computed by hand, and each row
    // a shape the wrong-that-looks-like-working would get wrong.
    const std::vector<std::pair<const char*, double>> table = {
        {"1 + 2 * 3", 7},                        // precedence: * binds tighter
        {"(1 + 2) * 3", 9},                      // parentheses override it
        {"8 - 3 - 2", 3},                        // LEFT-associative: (8-3)-2, not 8-(3-2)=7
        {"16 / 4 / 2", 2},                       // same for division
        {"-2 * -3", 6},                          // unary minus, twice
        {"--4", 4},                              // stacked unary
        {"wall.width * 2", 8},                   // the injected object
        {"wall.width * wall.height", 10},
        {"max(wall.width, door.offset) + 1", 5}, // a call with two arguments
        {"abs(-wall.height)", 2.5},
        {"wall.width > 3", 1},                   // comparisons yield 1 or 0
        {"wall.width * 2 >= door.offset + 7", 0},
        {"1.5 + .5", 2},                         // a leading-dot fraction
        {" 2  +  2 ", 4},                        // whitespace anywhere
        {"min(1, 2) == 1", 1},
    };
    for (const auto& [text, expected] : table) {
        CHECK(ValueIs(Evaluate(text, host), expected));
    }

    // Errors carry the byte offset of the culprit, not "syntax error".
    CHECK(ErrorAt(Evaluate("1 + * 2", host), 4));             // '*' where a value should be
    CHECK(ErrorAt(Evaluate("(1 + 2", host), 6));              // the ')' missing at the end
    CHECK(ErrorAt(Evaluate("1 + 2)", host), 5));              // and one too many
    CHECK(ErrorAt(Evaluate("wall.depth", host), 0));          // an unknown name: the name's position
    CHECK(ErrorAt(Evaluate("2 * roof.height", host), 4));
    CHECK(ErrorAt(Evaluate("max(1)", host), 0));              // known function, wrong arity
    CHECK(ErrorAt(Evaluate("1 / (2 - 2)", host), 2));         // division by zero: the '/'
    CHECK(ErrorAt(Evaluate("3 $ 4", host), 2));               // a character that is nothing
    CHECK(ErrorAt(Evaluate("", host), 0));                    // no text at all
    CHECK(ErrorAt(Evaluate("1..2", host), 0));                // not a number

    // The comma is never a decimal separator: "1,5" is 1, then a stray ','.
    // Under a German locale strtod would have read one and a half; the
    // number scan uses from_chars and never asks the locale. Asserted twice,
    // the second time with the locale actually switched where the machine has it.
    CHECK(ErrorAt(Evaluate("1,5", host), 1));
    if (std::setlocale(LC_NUMERIC, "de_DE.UTF-8") != nullptr) {
        CHECK(ErrorAt(Evaluate("1,5", host), 1));
        CHECK(ValueIs(Evaluate("1.5 * 2", host), 3));
        std::setlocale(LC_NUMERIC, "C");
    }

    // The depth guard: N levels accepted, N + 1 refused - with the position
    // where the level too deep begins, not a stack overflow. Depth is one
    // per parenthesis plus one for the value inside the innermost.
    const int limit = 64;
    const std::string deep(limit - 1, '(');                    // (((...(1)...))): 63 parens, depth 64
    CHECK(ValueIs(Evaluate(deep + "1" + std::string(limit - 1, ')'), host, limit), 1));
    CHECK(ErrorAt(Evaluate(deep + "(1" + std::string(limit, ')'), host, limit), static_cast<std::size_t>(limit)));
    // And the hostile input: a thousand opening parens is refused at the
    // limit, in microseconds, without a stack frame per byte.
    CHECK(ErrorAt(Evaluate(std::string(1000, '('), host, limit), static_cast<std::size_t>(limit)));
    CHECK(ErrorAt(Evaluate(std::string(1000, '-') + "1", host, limit), static_cast<std::size_t>(limit)));

    // Parse once, evaluate per row: the computed column.
    auto parsed = Formula::Parse("wall.width * wall.height > 8");
    CHECK(std::holds_alternative<Formula>(parsed));
    if (auto* f = std::get_if<Formula>(&parsed)) {
        CHECK(ValueIs(f->Evaluate(host), 1));
        host.Set("wall", "height", 1.0);
        CHECK(ValueIs(f->Evaluate(host), 0));               // same tree, the object changed
    }
    // A parse error is reported once, at parse time, before any row is touched.
    CHECK(std::holds_alternative<Error>(Formula::Parse("wall.width +")));

    if (g_failures == 0) {
        std::printf("formula ok: %zu values, the error positions, the depth limit at %d, the locale\n",
                    table.size(), limit);
    }
    return g_failures == 0 ? 0 : 1;
}
