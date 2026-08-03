// Appendix F, Recipes 2-5 - strings: split, join, build, format.
//
// The recipe functions below are quoted VERBATIM in book/F-rosetta-cookbook.md:
// editing one means editing the appendix in the same commit (the testlab
// discipline). main() is scaffolding, not part of any recipe - it asserts
// what the recipes claim, so build_all.sh keeps the cookbook honest.
#include <cassert>
#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// Recipe 2 - string.Split
std::vector<std::string> split(const std::string& text, char sep) {
    std::vector<std::string> parts;
    std::istringstream stream(text);
    std::string field;
    while (std::getline(stream, field, sep)) {
        parts.push_back(field);
    }
    return parts;
}

// Recipe 3 - string.Join
std::string join(const std::vector<std::string>& parts, const std::string& sep) {
    std::string result;
    for (const auto& part : parts) {
        if (!result.empty()) {
            result += sep;    // between elements only - never leading
        }
        result += part;
    }
    return result;
}

// Recipe 4 - StringBuilder
std::string build_report(const std::vector<int>& values) {
    std::string out;
    // one allocation up front - the StringBuilder(capacity) constructor
    out.reserve(values.size() * 12);
    for (int value : values) {
        out += "value=";
        out += std::to_string(value);
        out += '\n';
    }
    return out;
}

// Recipe 5 - string.Format
std::string describe(int count, double ratio) {
    std::ostringstream out;
    out << count << " samples, ratio "
        << std::fixed << std::setprecision(2) << ratio;
    return out.str();
}

std::string describe_c(int count, double ratio) {
    char buffer[64];
    std::snprintf(buffer, sizeof buffer, "%d samples, ratio %.2f", count, ratio);
    return buffer;
}

int main() {
    // Recipe 2, including the two behaviors the appendix claims: interior
    // empty fields are kept, the final empty field is not (C# keeps it).
    assert((split("a,b,c", ',') == std::vector<std::string>{"a", "b", "c"}));
    assert((split("a,,b", ',') == std::vector<std::string>{"a", "", "b"}));
    assert((split("a,b,", ',') == std::vector<std::string>{"a", "b"}));

    // Recipe 3: no leading or trailing separator at any element count.
    assert(join({}, ", ").empty());
    assert(join({"x"}, ", ") == "x");
    assert(join({"x", "y", "z"}, ", ") == "x, y, z");

    // Recipe 4.
    assert(build_report({1, 2}) == "value=1\nvalue=2\n");
    assert(build_report({}).empty());

    // Recipe 5: both spellings produce the same text.
    assert(describe(3, 0.5) == "3 samples, ratio 0.50");
    assert(describe_c(3, 0.5) == "3 samples, ratio 0.50");
    return 0;
}
