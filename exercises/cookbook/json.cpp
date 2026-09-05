// Appendix F, Recipes 25, 26 and 35 - JSON: serialize a record, read a
// config, walk a document you do not own.
//
// The one cookbook TU with a dependency: nlohmann/json, vendored under
// exercises/third_party/ (Chapter 27's first strategy, with the version and
// "no local patches" recorded beside it) and included with -isystem so the
// canonical flags keep meaning what they mean for this file. Reading,
// to_json(), from_json(), serialize(), Config, load_config(), Channel,
// read_channels() and count_numbers() are quoted VERBATIM in
// book/F-rosetta-cookbook.md: editing one means editing the appendix in the
// same commit (the testlab discipline). main() is scaffolding - it asserts
// the round trip, the walk, and the traps.
#include <cassert>
#include <nlohmann/json.hpp>
#include <optional>
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

// Recipe 35 - EnumerateObject / TryGetProperty / EnumerateArray over a document you do not own
struct Channel {
    std::string name;
    double gain = 1.0;
    std::optional<int> delay_ms;          // present on some channels, absent on others
};

std::vector<Channel> read_channels(const json& doc) {
    std::vector<Channel> out;
    for (const auto& [name, node] : doc.at("channels").items()) {   // an object: its keys and values
        Channel c;
        c.name = name;
        c.gain = node.value("gain", c.gain);                       // absent: the default
        if (node.contains("delay_ms")) {                           // TryGetProperty
            c.delay_ms = node.at("delay_ms").get<int>();
        }
        out.push_back(std::move(c));
    }
    return out;
}

int count_numbers(const json& node) {         // walk anything: objects, arrays, scalars, nested
    if (node.is_number()) {
        return 1;
    }
    if (!node.is_structured()) {              // a string, a bool, null: nothing inside
        return 0;
    }
    int n = 0;
    for (const auto& child : node) {          // an array yields its elements, an object its values
        n += count_numbers(child);
    }
    return n;
}

int main() {
    // Recipe 25: the round trip is the assertion.
    const std::vector<Reading> readings{{3, 21.5, "C"}, {9, 0.75, "V"}};
    const std::string text = serialize(readings);
    assert(text.find("\"sensor\": 3") != std::string::npos);
    const auto back = json::parse(text).get<std::vector<Reading>>();
    assert(back.size() == 2 && back[1].sensor == 9 && back[1].unit == "V");
    assert(json(readings[0]).dump() == R"({"sensor":3,"unit":"C","value":21.5})");   // keys sorted

    // Recipe 25's note: an optional member serializes as null, never as an
    // absent key (the library has written one since 3.12) - and in the
    // vendored 3.12.0 comes back only by hand, is_null() then get<int>():
    // the read-side overload sits behind a guard that is never open (fixed
    // upstream in #4742, unreleased at the time of writing).
    {
        const std::optional<int> present = 3;
        const std::optional<int> absent;
        assert((json{{"delay_ms", present}}.dump() == "{\"delay_ms\":3}"));
        assert((json{{"delay_ms", absent}}.dump() == "{\"delay_ms\":null}"));
        const json back = json::parse("{\"delay_ms\":null}");
        assert(back.at("delay_ms").is_null());              // null is a value it wrote, not a key it dropped
        const std::optional<int> read = back.at("delay_ms").is_null()
            ? std::nullopt : std::optional<int>(back.at("delay_ms").get<int>());
        assert(read == std::nullopt);
    }

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
    // document INSERTS a null for a missing key - Recipe 8's map trap in a
    // new coat - where at() throws. (A const document refuses the write and
    // ASSERTS on the read - undefined under NDEBUG - so it is not exercised.)
    json doc = json::parse(R"({"name": "bench"})");
    assert(doc.size() == 1);
    assert(doc["missing"].is_null());          // the read created it
    assert(doc.size() == 2);

    // value()'s default covers absence only: present-with-the-wrong-type is
    // a type_error, and so is a document that parsed but is not an object.
    threw = false;
    try {
        (void)load_config(R"({"name": "bench", "timeout": "5"})");
    } catch (const json::type_error&) {
        threw = true;
    }
    assert(threw);
    threw = false;
    try {
        (void)load_config("5");                 // valid JSON, not an object
    } catch (const json::type_error&) {
        threw = true;
    }
    assert(threw);

    // Recipe 35: the walk over a document whose shape is somebody else's.
    const json site = json::parse(R"({
        "channels": {
            "left":  { "gain": 0.5, "delay_ms": 20 },
            "right": { "gain": 0.5 },
            "sub":   { "delay_ms": 5, "tags": ["lfe", 1, 2] }
        },
        "version": 3
    })");
    const auto channels = read_channels(site);
    assert(channels.size() == 3);
    assert(channels[0].name == "left" && channels[0].delay_ms == 20);        // keys come back sorted
    assert(channels[1].name == "right" && !channels[1].delay_ms && channels[1].gain == 0.5);
    assert(channels[2].name == "sub" && channels[2].gain == 1.0 && channels[2].delay_ms == 5);
    assert(count_numbers(site) == 7);                 // 0.5, 20, 0.5, 5, 1, 2, 3
    assert(count_numbers(json::parse("\"text\"")) == 0);   // a scalar is not iterated as itself
    assert(count_numbers(json::parse("3.5")) == 1);
    // The trap the recipe names: get<int>() on 3.5 is 3, silently.
    const json fraction = json::parse("3.5");
    assert(fraction.is_number() && !fraction.is_number_integer());
    assert(fraction.get<int>() == 3);
    // items() on an array hands back "0", "1", ... as the keys. The array is
    // NAMED first, on purpose: `for (... : json::parse("[10, 20]").items())`
    // is a stack-use-after-scope under ASan (it was run) - items() is a view
    // of the document, a range-for keeps its range expression alive and not
    // the temporary that expression's member call was made on, and C++17's
    // rule is exactly that (C++23 extends the temporary's life).
    const json pair = json::parse("[10, 20]");
    std::string keys;
    for (const auto& [k, v] : pair.items()) {
        keys += k;
        (void)v;
    }
    assert(keys == "01");
    return 0;
}
