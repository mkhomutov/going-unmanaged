// Appendix F, Recipe 22's third spelling - std::expected, which is C++23.
//
// This is the one cookbook TU built with -std=c++23, and scripts/build_all.sh
// builds it as its own probe: on a toolchain that cannot compile it the
// section prints SKIPPED, and CI passes --require-expected so it can never
// skip there. Everything else in the cookbook is C++17, the book's pin,
// which is why the C++17 Result in errors.cpp is the recipe's listing and
// this file is Chapter 8's chaining example. channels_doubled() is quoted
// VERBATIM in book/08-error-handling.md, whole and by name (the
// check_verbatim UNITS table): editing it means editing the chapter in the
// same commit. main() is scaffolding.
#include <cassert>
#include <charconv>
#include <expected>
#include <string>
#include <string_view>
#include <system_error>

// The same two structs as errors.cpp, kept in step by hand: that file is
// C++17 and this one is not, and neither page quotes a shared header.
struct ConfigError { int line; std::string what; };
struct Config      { int channels; };

std::expected<Config, ConfigError> load_config(std::string_view text) {
    int channels = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), channels);
    if (ec != std::errc{} || end != text.data() + text.size() || channels <= 0) {
        return std::unexpected(ConfigError{1, "channel count is not a number"});
    }
    return Config{channels};
}

// The chain: and_then for a step that may itself fail, transform for one
// that cannot. Five early returns, spelled once.
std::expected<int, ConfigError> channels_doubled(std::string_view text) {
    return load_config(text)
        .and_then([](Config c) -> std::expected<int, ConfigError> {
            if (c.channels > 64) return std::unexpected(ConfigError{1, "too many channels"});
            return c.channels;
        })
        .transform([](int n) { return n * 2; });
}

int main() {
    const auto good = load_config("16");
    assert(good && good->channels == 16);          // -> and *, like optional
    const auto bad = load_config("x");
    assert(!bad && bad.error().what == "channel count is not a number");
    assert(channels_doubled("8") == 16);
    const auto many = channels_doubled("99");
    assert(!many && many.error().what == "too many channels");
    assert(channels_doubled("").value_or(-1) == -1);
    // value() on the error side throws bad_expected_access<E>, which carries
    // the error - the translation at the top of the program, and nowhere else.
    bool threw = false;
    try {
        (void)bad.value();
    } catch (const std::bad_expected_access<ConfigError>& e) {
        threw = (e.error().line == 1);
    }
    assert(threw);
    return 0;
}
