// Appendix F, Recipes 25 and 26 - JSON: serialize a record, read a config.
//
// The one cookbook TU with a dependency: nlohmann/json, vendored under
// exercises/third_party/ (Chapter 27's first strategy, with the version and
// "no local patches" recorded beside it) and included with -isystem so the
// canonical flags keep meaning what they mean for this file. Reading,
// to_json(), from_json(), serialize(), Config and load_config() are quoted
// VERBATIM in book/F-rosetta-cookbook.md: editing one means editing the
// appendix in the same commit (the testlab discipline). main() is
// scaffolding - it asserts the round trip and the two traps.
#include <cassert>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <vector>

using json = nlohmann::json;

// Recipe 25 - JsonSerializer.Serialize(record)
struct Reading {
    int sensor;
    double value;
    std::string unit;
};

// Two free functions the library finds by argument-dependent lookup - no
// attribute, no reflection: this IS the [JsonPropertyName] table, by hand.
void to_json(json& j, const Reading& r) {
    j = json{{"sensor", r.sensor}, {"value", r.value}, {"unit", r.unit}};
}

void from_json(const json& j, Reading& r) {
    j.at("sensor").get_to(r.sensor);
    j.at("value").get_to(r.value);
    j.at("unit").get_to(r.unit);
}

std::string serialize(const std::vector<Reading>& readings) {
    return json(readings).dump(2);            // 2 = indent; dump() alone is one line
}

// Recipe 26 - JsonSerializer.Deserialize<Config>(text), with defaults
struct Config {
    int timeout = 30;
    std::string name;
};

Config load_config(std::string_view text) {
    const json j = json::parse(text);         // junk throws json::parse_error - the event pole
    Config c;
    c.timeout = j.value("timeout", c.timeout);      // TryGetValue with a default: absent is fine
    c.name = j.at("name").get<std::string>();        // at(): required - missing throws out_of_range
    return c;
}

int main() {
    // Recipe 25: the round trip is the assertion.
    const std::vector<Reading> readings{{3, 21.5, "C"}, {9, 0.75, "V"}};
    const std::string text = serialize(readings);
    assert(text.find("\"sensor\": 3") != std::string::npos);
    const auto back = json::parse(text).get<std::vector<Reading>>();
    assert(back.size() == 2 && back[1].sensor == 9 && back[1].unit == "V");
    assert(json(readings[0]).dump() == R"({"sensor":3,"unit":"C","value":21.5})");   // keys sorted

    // Recipe 26: the optional field defaults, the required one throws.
    const Config c = load_config(R"({"name": "bench", "timeout": 5})");
    assert(c.timeout == 5 && c.name == "bench");
    assert(load_config(R"({"name": "bench"})").timeout == 30);
    bool threw = false;
    try {
        (void)load_config(R"({"timeout": 5})");
    } catch (const json::out_of_range&) {
        threw = true;                          // "name" is required
    }
    assert(threw);
    threw = false;
    try {
        (void)load_config("not json");
    } catch (const json::parse_error&) {
        threw = true;
    }
    assert(threw);

    // The trap the appendix names, demonstrated: operator[] on a non-const
    // document INSERTS a null for a missing key - Chapter 11's map trap in
    // a new coat - where at() throws and a const document refuses to compile
    // the write.
    json doc = json::parse(R"({"name": "bench"})");
    assert(doc.size() == 1);
    assert(doc["missing"].is_null());          // the read created it
    assert(doc.size() == 2);
    return 0;
}
