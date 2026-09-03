// Appendix F, Recipes 21 and 22 - throw and catch your own exception type;
// return a value or an error, on C++17.
//
// ParseError, parse_channel_count(), channels_or_default(), Result,
// ConfigError, Config and load_config() are quoted VERBATIM in
// book/F-rosetta-cookbook.md, and Result and load_config() again in
// Chapter 8's "Living in both dialects" (book/08-error-handling.md), whole
// and by name - scripts/check_verbatim.sh holds both pages: editing one
// means editing both pages in the same commit (the testlab discipline).
// main() and log_line() are scaffolding - main() asserts what the recipes
// claim, and records the catch-order trap as a comment, because a dead
// handler is a warning rather than a behavior a test can observe.
#include <cassert>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

namespace {
    int logged_line = -1;
    std::string logged_what;
    void log_line(int line, const std::string& what) {
        logged_line = line;
        logged_what = what;
    }
}

// Recipe 21 - class ParseException : Exception
class ParseError : public std::runtime_error {
public:
    ParseError(int line, const std::string& what)
        : std::runtime_error("line " + std::to_string(line) + ": " + what),
          line_(line) {}
    int line() const noexcept { return line_; }    // the payload what() cannot carry
private:
    int line_;
};

int parse_channel_count(const std::string& text, int line) {
    if (text.empty() || text.size() > 3 ||
        text.find_first_not_of("0123456789") != std::string::npos) {
        throw ParseError(line, "channel count is not a number: '" + text + "'");
    }
    return std::stoi(text);                 // at most three digits: cannot overflow
}

int channels_or_default(const std::string& text, int line) {
    try {
        return parse_channel_count(text, line);
    } catch (const ParseError& e) {          // the derived type FIRST
        log_line(e.line(), e.what());
        return 2;
    } catch (const std::exception& e) {      // then the base: order is the rule
        log_line(line, e.what());
        return 2;
    }
}

// Recipe 22 - the Result shape, on C++17: a variant with two named doors.
// This is what std::expected spells in C++23; most codebases are not there,
// and ship one of these (Chapter 8 names the well-known ones).
template <class T, class E>
class Result {
public:
    static Result ok(T value)   { return Result(State(std::in_place_index<0>, std::move(value))); }
    static Result fail(E error) { return Result(State(std::in_place_index<1>, std::move(error))); }

    bool has_value() const noexcept { return state_.index() == 0; }
    explicit operator bool() const noexcept { return has_value(); }

    const T& value() const { return std::get<0>(state_); }   // throws bad_variant_access on a failure
    const E& error() const { return std::get<1>(state_); }   // ...and on a success

private:
    using State = std::variant<T, E>;       // by index, so T and E may be the same type
    explicit Result(State s) : state_(std::move(s)) {}
    State state_;
};

struct ConfigError { int line; std::string what; };
struct Config      { int channels; };

// The translation, inward: the parser throws, the module boundary returns.
// Nothing above this function ever sees a ParseError.
Result<Config, ConfigError> load_config(const std::string& text) {
    try {
        return Result<Config, ConfigError>::ok(Config{parse_channel_count(text, 1)});
    } catch (const ParseError& e) {          // the throw stops here: failure becomes a value
        return Result<Config, ConfigError>::fail(ConfigError{e.line(), e.what()});
    }
}

int main() {
    // Recipe 21: the derived handler runs, and what() carries the message
    // the constructor built.
    assert(channels_or_default("16", 4) == 16);
    assert(channels_or_default("sixteen", 4) == 2);
    assert(logged_line == 4);
    assert(logged_what == "line 4: channel count is not a number: 'sixteen'");
    // The trap, as a comment - it is a warning, not a value:
    //   catch (const std::exception&) { ... }   // FIRST
    //   catch (const ParseError&)     { ... }   // dead: never reached
    // clang under -Wall says "exception of type 'const ParseError &' will
    // be caught by earlier handler" (-Wexceptions); GCC warns by default.

    // Recipe 22: one type, two doors, and the caller must look before it
    // touches the value.
    const auto good = load_config("8");
    assert(good && good.value().channels == 8);
    const auto bad = load_config("many");
    assert(!bad);
    assert(bad.error().line == 1);
    assert(bad.error().what == "line 1: channel count is not a number: 'many'");
    // The translation, outward: at the top of the program a value becomes a
    // throw again - the one place that is allowed to.
    bool threw = false;
    try {
        (void)bad.value();
    } catch (const std::bad_variant_access&) {
        threw = true;
    }
    assert(threw);
    // T and E may be the same type, because the doors are numbered.
    const auto same = Result<int, int>::fail(7);
    assert(!same && same.error() == 7);
    return 0;
}
