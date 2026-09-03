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
#include <charconv>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
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

int parse_channel_count(std::string_view text, int line) {
    int value = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || end != text.data() + text.size() || value <= 0) {
        throw ParseError(line, "channel count is not a number: '" + std::string(text) + "'");
    }
    return value;
}

int channels_or_default(std::string_view text, int line) {
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

// Recipe 22 - the Result shape, on C++17: a variant behind two named doors.
// This is what std::expected spells in C++23; most codebases are not there,
// and ship one of these (Chapter 8 names the well-known ones).
template <class T, class E>
class Result {
public:
    static Result ok(T value)   { return Result(std::in_place_index<0>, std::move(value)); }
    static Result fail(E error) { return Result(std::in_place_index<1>, std::move(error)); }

    bool has_value() const noexcept { return state_.index() == 0; }
    explicit operator bool() const noexcept { return has_value(); }

    const T& value() const { return std::get<0>(state_); }   // throws bad_variant_access on a failure
    const E& error() const { return std::get<1>(state_); }   // ...and on a success

private:
    template <std::size_t I, class X>                        // built in place: one move, not two
    Result(std::in_place_index_t<I> door, X&& x) : state_(door, std::forward<X>(x)) {}
    std::variant<T, E> state_;                               // index 0 is the value, 1 the error
};

struct ConfigError { int line; std::string what; };
struct Config      { int channels; };

// The translation at the module's edge: the parser throws, this function
// returns. Nothing above it ever sees a ParseError.
Result<Config, ConfigError> load_config(std::string_view text) {
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
    assert(channels_or_default("0", 5) == 2);    // parses, and is not a channel count
    // The trap, as a comment - it is a warning, not a value:
    //   catch (const std::exception&) { ... }   // FIRST
    //   catch (const ParseError&)     { ... }   // dead: never reached
    // Both compilers warn about it by default (clang names it -Wexceptions).

    // Recipe 22: one type, two doors, and the caller must look before it
    // touches the value.
    const auto good = load_config("8");
    assert(good && good.value().channels == 8);
    const auto bad = load_config("many");
    assert(!bad);
    assert(bad.error().line == 1);
    assert(bad.error().what == "line 1: channel count is not a number: 'many'");
    // The translation at the top: a value becomes a throw again, through
    // the accessor for the door that is not open - in both directions.
    bool threw = false;
    try {
        (void)bad.value();
    } catch (const std::bad_variant_access&) {
        threw = true;
    }
    assert(threw);
    threw = false;
    try {
        (void)good.error();
    } catch (const std::bad_variant_access&) {
        threw = true;
    }
    assert(threw);
    return 0;
}
