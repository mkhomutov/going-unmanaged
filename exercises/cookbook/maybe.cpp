// Appendix F, Recipes 19 and 20 - a value that may be absent, and a value
// that is one of several kinds.
//
// parse_port(), port_or_default(), digits_in(), the Event alternatives,
// `overloaded` and describe() are quoted VERBATIM in
// book/F-rosetta-cookbook.md: editing one means editing the appendix in the
// same commit (the testlab discipline). main() is scaffolding - it asserts
// what the recipes claim, including the two refusals the appendix names,
// which are stated here as comments because a refusal cannot be compiled.
#include <cassert>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

// Recipe 19 - int? / ?? / ?.
std::optional<int> parse_port(std::string_view text) {
    if (text.empty()) {
        return std::nullopt;              // no value - the C# null, spelled
    }
    int value = 0;
    for (char c : text) {
        if (c < '0' || c > '9') {
            return std::nullopt;          // not a number: absence, not an error (Chapter 8)
        }
        value = value * 10 + (c - '0');
        if (value > 65535) {
            return std::nullopt;
        }
    }
    return value;
}

int port_or_default(const std::optional<int>& port) {
    return port.value_or(8080);           // the ?? operator
}

std::optional<std::size_t> digits_in(const std::optional<std::string>& text) {
    if (!text) {
        return std::nullopt;              // ?. by hand: C++17 has no null-propagating call
    }
    return text->size();                  // -> is only legal once you have checked
}

// Recipe 20 - switch (e) { case Temperature t: ... }
struct Temperature { int centi; };        // centi-degrees, as the wire carries them
struct Fault       { int code; };
struct Heartbeat   {};
using Event = std::variant<Temperature, Fault, Heartbeat>;

// The overloaded-lambdas idiom: one callable with one operator() per
// alternative. Two lines every codebase on C++17 has somewhere.
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
    // Recipe 19: three outcomes, one type, and the caller cannot forget to
    // look because the value is behind operator* rather than in front of it.
    assert(parse_port("8080") == std::optional<int>{8080});
    assert(!parse_port("").has_value());
    assert(parse_port("80a").has_value() == false);
    assert(parse_port("70000") == std::nullopt);
    assert(port_or_default(parse_port("443")) == 443);
    assert(port_or_default(parse_port("")) == 8080);
    assert(digits_in(std::string("12345")) == std::optional<std::size_t>{5});
    assert(digits_in(std::nullopt) == std::nullopt);
    // What the appendix says does not exist, stated where a build can at
    // least document it:
    //   std::optional<int&> r;   // refused: "instantiation of optional with
    //                            // a reference type is ill-formed" (libc++)
    // and what compiles and is undefined behaviour instead of null-propagating:
    //   std::optional<Temperature> none; none->centi;   // runs, prints garbage,
    //                                                   // and the sanitizers say nothing

    // Recipe 20: every alternative dispatches, and the tag cannot lie.
    Event e = Temperature{2345};
    assert(describe(e) == "temperature 2345 centi-degrees");
    e = Fault{7};
    assert(describe(e) == "fault 7");
    assert(std::holds_alternative<Fault>(e));
    assert(std::get_if<Temperature>(&e) == nullptr);
    e = Heartbeat{};
    assert(describe(e) == "heartbeat");
    assert(e.index() == 2);
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
