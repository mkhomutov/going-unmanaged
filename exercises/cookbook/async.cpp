// Appendix F, Recipe 13 - run work on another thread and wait for it.
//
// overlap_work() is quoted VERBATIM in book/F-rosetta-cookbook.md: editing it
// means editing the appendix in the same commit (the testlab discipline).
// count_defects() and do_other_work() stand for real work; main() is
// scaffolding, including the one C# behavior that ports exactly - a throw
// inside the work surfaces at get().
#include <cassert>
#include <future>
#include <stdexcept>

int count_defects() {
    int found = 0;
    for (int i = 0; i < 100000; ++i) {
        if (i % 1000 == 0) {
            ++found;
        }
    }
    return found;    // 100
}

int do_other_work() {
    return 4;
}

int overlap_work() {
    std::future<int> task = std::async(std::launch::async, count_defects);
    const int other = do_other_work();    // runs while count_defects runs
    return other + task.get();            // the await: blocks until the result arrives
}

int main() {
    assert(overlap_work() == 104);

    // A throw inside the work is captured and rethrown at get() - await's
    // exception unwrapping, without the runtime.
    std::future<int> failing = std::async(std::launch::async, []() -> int {
        throw std::runtime_error("boom");
    });
    bool rethrown = false;
    try {
        failing.get();
    } catch (const std::runtime_error&) {
        rethrown = true;
    }
    assert(rethrown);
    return 0;
}
