// Appendix F, Recipes 8 and 18 - look up a key without inserting it; find
// an element, an index, or a substring.
//
// apply_timeout_setting(), index_of() and contains_word() are quoted
// VERBATIM in book/F-rosetta-cookbook.md: editing one means editing the
// appendix in the same commit (the testlab discipline). main() is
// scaffolding - it also demonstrates the traps the appendix names: reading
// a missing key with [] inserts it, and std::find on a map is a linear
// walk that compiles.
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace {
    int applied = -1;
    void apply_timeout(int value) { applied = value; }
}

void apply_timeout_setting(const std::map<std::string, int>& settings) {
    const auto it = settings.find("timeout");
    if (it != settings.end()) {
        apply_timeout(it->second);    // found - the iterator is the out-parameter
    }
}

// Recipe 18 - IndexOf / Contains, on a sequence and on a string
std::optional<std::size_t> index_of(const std::vector<int>& values, int wanted) {
    const auto it = std::find(values.begin(), values.end(), wanted);
    if (it == values.end()) {
        return std::nullopt;             // an algorithm says "not found" as end()
    }
    return static_cast<std::size_t>(std::distance(values.begin(), it));
}

bool contains_word(const std::string& text, const std::string& word) {
    return text.find(word) != std::string::npos;    // a string says it as npos
}

int main() {
    const std::map<std::string, int> settings{{"timeout", 30}};
    apply_timeout_setting(settings);
    assert(applied == 30);

    applied = -1;
    apply_timeout_setting({});    // missing key: no call, no throw, no insert
    assert(applied == -1);

    // The trap, demonstrated: [] on a missing key default-constructs a value
    // and INSERTS it - the read mutates the map. (On the const map above it
    // would not even compile, which is the compiler making the same point.)
    std::map<std::string, int> counters;
    const int seen = counters["missing"];
    assert(seen == 0);
    assert(counters.size() == 1);

    // Recipe 18: the index comes back as an optional, so "not found" is a
    // value the caller must look at rather than -1 or end() leaking out.
    const std::vector<int> ids{7, 11, 42};
    assert(index_of(ids, 42) == std::optional<std::size_t>{2});
    assert(!index_of(ids, 5).has_value());
    assert(index_of({}, 5) == std::nullopt);
    assert(contains_word("sensor offline", "offline"));
    assert(!contains_word("sensor offline", "online"));
    // The trap the recipe names: `if (text.find(word))` tests the POSITION,
    // so a match at offset 0 reads as false and npos reads as true.
    assert(std::string("offline").find("offline") == 0);
    assert(std::string::npos != 0);

    // The other trap: std::find on a map compiles and walks every node.
    // The member find is the hash or tree lookup you meant (Recipe 8).
    const std::map<std::string, int> big{{"a", 1}, {"b", 2}, {"c", 3}};
    const auto slow = std::find(big.begin(), big.end(), std::pair<const std::string, int>{"c", 3});
    assert(slow != big.end() && slow == big.find("c"));   // same answer, O(n) vs O(log n)
    return 0;
}
