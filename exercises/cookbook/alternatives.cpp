// Appendix F, Recipes 19 and 20 - a value that may be absent, and a value
// that is one of several kinds: the two sum types a C# developer has never
// spelled.
//
// parse_port(), port_or_default(), digits_in(), and the Recipe 20 listing
// from `struct Temperature` through describe() are quoted VERBATIM in
// book/F-rosetta-cookbook.md: editing one means editing the appendix in the
// same commit (the testlab discipline). main() is scaffolding - it asserts
// what the recipes claim, and states the two refusals the appendix names as
// comments, because a refusal cannot be compiled.
#include <cassert>
#include <charconv>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>

// Recipe 19 - int.TryParse / ?? / ?.
std::optional<int> parse_port(std::string_view text) {
    int value = 0;
    const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || end != text.data() + text.size() || value < 0 || value > 65535) {
        return std::nullopt;              // not a port: absence, not an error (Chapter 8)
    }
    return value;                         // TryParse's out-parameter, as the return
}

int port_or_default(std::optional<int> port) {
    return port.value_or(8080);           // the ?? operator
}

std::optional<std::size_t> digits_in(const std::optional<std::string>& text) {
    if (!text) {
        return std::nullopt;              // ?. by hand: C++17 has no null-propagating call
    }
    return text->size();                  // -> is only legal once you have checked
}

// Recipe 20 - switch (e) { case Temperature t: ... }. The overloaded idiom
// is the two template lines: one callable with one operator() per
// alternative, which C++17 does not ship and every codebase has.
struct Temperature { int centi; };        // centi-degrees, as the wire carries them
struct Fault       { int code; };
struct Heartbeat   {};
using Event = std::variant<Temperature, Fault, Heartbeat>;

template <class... Fs> struct overloaded : Fs... { using Fs::operator()...; };
template <class... Fs> overloaded(Fs...) -> overloaded<Fs...>;

std::string describe(const Event& e) {
    return std::visit(overloaded{
        [](const Temperature& t) { return "temperature " + std::to_string(t.centi) + " centi-degrees"; },
        [](const Fault& f)       { return "fault " + std::to_string(f.code); },
        [](Heartbeat)            { return std::string("heartbeat"); },
    }, e);
}

int main() {
    // Recipe 19: one type, and the caller cannot forget to look because the
    // value is behind operator* rather than in front of it.
    assert(parse_port("8080") == 8080);
    assert(!parse_port(""));
    assert(!parse_port("80a"));
    assert(!parse_port("-1"));            // from_chars accepts a sign; a port does not
    assert(!parse_port("70000"));
    assert(!parse_port("99999999999"));   // overflow is an error code, not UB
    assert(port_or_default(parse_port("443")) == 443);
    assert(port_or_default(parse_port("")) == 8080);
    assert(digits_in(std::string("12345")) == std::size_t{5});
    assert(!digits_in(std::nullopt));
    // What the appendix says does not exist, stated where a build can at
    // least document it:
    //   std::optional<int&> r;   // refused before C++26: "instantiation of
    //                            // optional with a reference type is ill-formed" (libc++)
    // and what compiles and is undefined behavior instead of null-propagating:
    //   std::optional<Temperature> none; none->centi;   // runs, prints garbage,
    //                                                   // and the sanitizers say nothing

    // Recipe 20: every alternative dispatches, and the tag cannot lie.
    Event e = Temperature{2345};
    assert(describe(e) == "temperature 2345 centi-degrees");
    e = Fault{7};
    assert(describe(e) == "fault 7");
    assert(std::holds_alternative<Fault>(e));
    e = Heartbeat{};
    assert(describe(e) == "heartbeat");
    bool threw = false;
    try {
        (void)std::get<Fault>(e);         // the wrong alternative: an exception, not garbage
    } catch (const std::bad_variant_access&) {
        threw = true;
    }
    assert(threw);
    // The refusal the recipe's trap names, as a comment for the same reason:
    //   std::visit(overloaded{ [](const Temperature&){}, [](const Fault&){} }, e);
    //   // error: static assertion failed ... `std::visit` requires the
    //   // visitor to be exhaustive (libc++); GCC's wording differs, the
    //   // verdict does not
    return 0;
}
