// Appendix F, Recipes 6 and 16 - time a call; run something every interval.
//
// report_batch_time() and RepeatingTimer are quoted VERBATIM in
// book/F-rosetta-cookbook.md: editing one means editing the appendix in the
// same commit (the testlab discipline). run_the_batch() stands for whatever
// is being timed; main() is scaffolding - its second half asserts that ticks
// arrive while the timer lives and that none can fire after the join.
#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

void run_the_batch() {
    // Just enough work to be measurable; volatile so the optimizer cannot
    // collapse the loop into one addition (Chapter 29's lesson).
    volatile long long sink = 0;
    for (int i = 0; i < 1000000; ++i) {
        sink = sink + i;
    }
}

// Recipe 6 - Stopwatch
void report_batch_time() {
    const auto start = std::chrono::steady_clock::now();
    run_the_batch();    // the code being timed
    const auto elapsed = std::chrono::steady_clock::now() - start;
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    std::cout << ms.count() << " ms\n";
}

// Recipe 16 - System.Timers.Timer
class RepeatingTimer {
public:
    RepeatingTimer(std::chrono::milliseconds interval, std::function<void()> tick)
        : worker_([this, interval, tick = std::move(tick)] {
              while (!stop_) {
                  // Task.Delay, spelled honestly: a thread you own, blocked.
                  std::this_thread::sleep_for(interval);
                  if (!stop_) {
                      tick();
                  }
              }
          }) {}

    ~RepeatingTimer() {
        stop_ = true;
        worker_.join();    // Chapter 29's obligation - and this join IS the Stop()
    }

private:
    std::atomic<bool> stop_{false};    // declared before worker_: initialized first
    std::thread worker_;
};

int main() {
    report_batch_time();

    std::atomic<int> ticks{0};
    {
        RepeatingTimer timer(std::chrono::milliseconds(10),
                             [&ticks] { ++ticks; });
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
    }    // destructor stops and joins HERE

    const int seen = ticks;
    assert(seen >= 2);    // several intervals passed while the timer lived
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(ticks == seen);    // and no tick fired after the join
    return 0;
}
