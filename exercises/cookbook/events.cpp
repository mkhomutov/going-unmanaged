// Appendix F, Recipe 14 - expose an event.
//
// SampleSource is quoted VERBATIM in book/F-rosetta-cookbook.md: editing it
// means editing the appendix in the same commit (the testlab discipline).
// main() is scaffolding - it asserts subscription order, token-based
// unsubscribe, and that raising with no subscribers is a plain no-op.
#include <algorithm>
#include <cassert>
#include <functional>
#include <string>
#include <utility>
#include <vector>

class SampleSource {
public:
    using Handler = std::function<void(int)>;

    int subscribe(Handler handler) {
        handlers_.emplace_back(next_id_, std::move(handler));
        return next_id_++;    // the token is how -= works without delegate identity
    }

    void unsubscribe(int id) {
        handlers_.erase(std::remove_if(handlers_.begin(), handlers_.end(),
                            [id](const auto& entry) { return entry.first == id; }),
                        handlers_.end());
    }

    void raise(int sample) {    // the ?.Invoke: an empty list is a zero-pass loop
        for (const auto& entry : handlers_) {
            entry.second(sample);
        }
    }

private:
    std::vector<std::pair<int, Handler>> handlers_;
    int next_id_ = 0;
};

int main() {
    SampleSource source;
    std::string seen;

    source.raise(0);    // no subscribers: nothing happens, nothing to null-check
    assert(seen.empty());

    const int a = source.subscribe([&seen](int s) { seen += "a" + std::to_string(s); });
    const int b = source.subscribe([&seen](int s) { seen += "b" + std::to_string(s); });
    assert(a != b);

    source.raise(1);
    assert(seen == "a1b1");    // every subscriber, in subscription order

    source.unsubscribe(a);
    source.raise(2);
    assert(seen == "a1b1b2");    // only b remains

    source.unsubscribe(b);
    source.raise(3);
    assert(seen == "a1b1b2");    // list empty again - raise is a no-op
    return 0;
}
