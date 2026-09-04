// Appendix F, Recipes 6, 16 and 28-30 - time a call; run something every
// interval; a scoped timer and a forwarding wrapper; a timestamp for a log
// line; a timeout handed to a C API.
//
// report_batch_time(), RepeatingTimer, ScopedTimer, time_call(),
// timestamp_utc(), the Device_Wait declaration and wait_for_sample() are
// quoted VERBATIM in book/F-rosetta-cookbook.md: editing one means editing
// the appendix in the same commit (the testlab discipline). run_the_batch()
// and Device_Wait()'s body stand for whatever is being timed or called;
// main() is scaffolding - it asserts that ticks arrive while the timer
// lives and none after the join, that the scoped timer records on the
// throwing path too, that the wrapper forwards an lvalue as an lvalue and
// an rvalue as an rvalue (judged by a callee overloaded on the value
// category, so a wrapper that copies or moves everything fails), that the timestamp has the shape and the century
// it claims, and that a seconds literal reaches the C API multiplied out.
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>

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

// Recipe 28 - Stopwatch.StartNew() with the stop in a finally, so it runs on
// every exit path, and a wrapper that times one call and hands its result back
class ScopedTimer {
public:
    explicit ScopedTimer(std::chrono::nanoseconds& record)
        : record_(record), start_(std::chrono::steady_clock::now()) {}
    ~ScopedTimer() { record_ = std::chrono::steady_clock::now() - start_; }   // return, throw: every path
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
    std::chrono::nanoseconds& record_;
    std::chrono::steady_clock::time_point start_;
};

template <class F, class... Args>
auto time_call(std::chrono::nanoseconds& record, F&& f, Args&&... args)
    -> std::invoke_result_t<F, Args...> {
    ScopedTimer timer(record);
    return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);   // each argument passed on as it arrived
}

// Recipe 29 - DateTime.UtcNow.ToString("o"), to the millisecond rather than the tick
std::string timestamp_utc() {
    const auto now = std::chrono::system_clock::now();          // the wall clock: the one with a calendar
    const auto since_epoch = now.time_since_epoch();
    const auto whole = std::chrono::floor<std::chrono::seconds>(since_epoch);   // what to_time_t would give, rounding settled
    const std::time_t seconds = whole.count();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch - whole);
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &seconds);              // the thread-safe spellings: never std::gmtime
#else
    gmtime_r(&seconds, &utc);
#endif
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << millis.count() << 'Z';
    return out.str();
}

// Recipe 30 - TimeSpan.FromSeconds(2) handed to an SDK that wants an integer
int Device_Wait(std::uint32_t timeout_ms);   // the vendor's declaration: a bare integer, the unit in the name

int wait_for_sample(std::chrono::milliseconds timeout) {
    return Device_Wait(static_cast<std::uint32_t>(timeout.count()));   // the unit left the type HERE, and only here
}

namespace {
    std::uint32_t last_timeout_ms = 0;
}

// Scaffolding: the vendor's function, standing in. Records what it was given.
int Device_Wait(std::uint32_t timeout_ms) {
    last_timeout_ms = timeout_ms;
    return 0;
}

int add_one(int x) { return x + 1; }

// Reports the value category it was handed - the judge for std::forward.
struct Which {
    const char* operator()(std::string&) const { return "lvalue"; }
    const char* operator()(std::string&&) const { return "rvalue"; }
};

int main() {
    report_batch_time();

    std::atomic<int> ticks{0};
    {
        RepeatingTimer timer(std::chrono::milliseconds(10),
                             [&ticks] { ++ticks; });
        // Wait for ticks rather than for time: a loaded CI runner can starve
        // the worker for longer than any fixed sleep, and this assertion is
        // about the timer firing while it lives, not about its cadence.
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (ticks < 2 && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }    // destructor stops and joins HERE

    const int seen = ticks;
    assert(seen >= 2);    // intervals passed while the timer lived
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(ticks == seen);    // and no tick fired after the join

    // Recipe 28: the scoped timer records on the normal path...
    std::chrono::nanoseconds took{0};
    {
        ScopedTimer timer(took);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    assert(took >= std::chrono::milliseconds(5));
    // ...and on the throwing path, because the destructor is the stop.
    std::chrono::nanoseconds thrown{0};
    bool threw = false;
    try {
        ScopedTimer timer(thrown);
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        throw std::runtime_error("mid-block");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw && thrown >= std::chrono::milliseconds(2));

    // The wrapper hands the result back and forwards each argument as it
    // arrived: an lvalue stays an lvalue (the string is intact afterwards),
    // an rvalue stays an rvalue (the callee may steal it).
    std::chrono::nanoseconds call{0};
    assert(time_call(call, add_one, 41) == 42);
    assert(time_call(call, [](int a, int b) { return a * b; }, 6, 7) == 42);
    std::string kept = "lvalue";
    const std::size_t n1 = time_call(call, [](const std::string& s) { return s.size(); }, kept);
    assert(n1 == 6 && kept == "lvalue");                   // borrowed, not moved
    std::string given = "rvalue";
    const std::string taken = time_call(call, [](std::string s) { return s; }, std::move(given));
    assert(taken == "rvalue");                              // moved through: the cast survived the hop
    static_assert(std::is_same_v<decltype(time_call(call, add_one, 1)), int>);
    // The two lines above are satisfied by a wrapper that copies, and by one
    // that std::moves everything; this callee is not - it reports the value
    // category it was handed, so a wrapper that changed it fails here.
    std::string probe = "x";
    assert(std::string_view(time_call(call, Which{}, probe)) == "lvalue");             // std::move would say rvalue
    assert(std::string_view(time_call(call, Which{}, std::move(probe))) == "rvalue");  // a by-value pass would say lvalue

    // Recipe 29: the shape and the century, not the value - the value is
    // whatever now is.
    const std::string stamp = timestamp_utc();
    assert(stamp.size() == 24);                             // 2026-09-04T12:34:56.789Z
    assert(stamp[4] == '-' && stamp[10] == 'T' && stamp[19] == '.' && stamp[23] == 'Z');
    assert(stamp.compare(0, 2, "20") == 0);
    for (const std::size_t i : {0u, 1u, 2u, 3u, 5u, 6u, 8u, 9u, 11u, 12u, 14u, 15u, 17u, 18u, 20u, 21u, 22u}) {
        assert(stamp[i] >= '0' && stamp[i] <= '9');
    }

    // Recipe 30: seconds arrive multiplied out, because the parameter's type
    // did the conversion before count() ever ran.
    using namespace std::chrono_literals;
    wait_for_sample(250ms);
    assert(last_timeout_ms == 250);
    wait_for_sample(2s);                                    // seconds -> milliseconds: implicit, lossless
    assert(last_timeout_ms == 2000);
    wait_for_sample(std::chrono::minutes(1));
    assert(last_timeout_ms == 60000);
    // What does NOT compile, stated as a comment because a refusal cannot be
    // asserted: a function taking std::chrono::seconds handed 250ms - the
    // lossy direction needs an explicit duration_cast, and the compiler
    // refuses the silent truncation.
    return 0;
}
