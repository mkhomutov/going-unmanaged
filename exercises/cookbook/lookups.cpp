// Appendix F, Recipe 8 - look up a key without inserting it.
//
// apply_timeout_setting() is quoted VERBATIM in book/F-rosetta-cookbook.md:
// editing it means editing the appendix in the same commit (the testlab
// discipline). main() is scaffolding - it also demonstrates the trap the
// appendix names: reading a missing key with [] inserts it.
#include <cassert>
#include <map>
#include <string>

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
    return 0;
}
