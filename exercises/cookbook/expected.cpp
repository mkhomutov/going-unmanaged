// Appendix F, Recipe 22's third spelling - std::expected, which is C++23.
//
// This is the one cookbook TU built with -std=c++23, behind a probe in
// scripts/build_all.sh: on a toolchain without <expected> it prints SKIPPED,
// and CI passes --require-expected so it can never skip there. Everything
// else in the cookbook is C++17, the book's pin, which is why the C++17
// Result in errors.cpp is the recipe's main listing and this is its
// footnote. load_config() and channels_doubled() are quoted VERBATIM in
// book/F-rosetta-cookbook.md: editing either means editing the appendix in
// the same commit. main() is scaffolding.
#include <cassert>
#include <expected>
#include <string>
#include <string_view>

struct ConfigError { int line; std::string what; };
struct Config      { int channels; };

std::expected<Config, ConfigError> load_config(std::string_view text) {
    if (text.empty() || text.size() > 3 ||
        text.find_first_not_of("0123456789") != std::string_view::npos) {
        return std::unexpected(ConfigError{1, "channel count is not a number"});
    }
    return Config{std::stoi(std::string(text))};
}

// The chain: and_then for a step that may itself fail, transform for one
// that cannot. Five `if (!r) return r.error();` lines, spelled once.
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
    assert(!channels_doubled("99").has_value());
    assert(channels_doubled("99").error().what == "too many channels");
    assert(channels_doubled("").value_or(-1) == -1);
    // value() on the error side throws bad_expected_access<E>, which carries
    // the error - the outward translation, at the top of the program only.
    bool threw = false;
    try {
        (void)bad.value();
    } catch (const std::bad_expected_access<ConfigError>& e) {
        threw = (e.error().line == 1);
    }
    assert(threw);
    return 0;
}
