#include "meter.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <new>

// The judge this ticket needed: a heap-allocation counter. Replacing the
// global operator new and operator delete is legal and program-wide - it is
// what a memory profiler does, with more ceremony. The default operator
// new[] and the sized and array deletes all forward to these two, so
// counting here counts everything a std:: container allocates.
namespace {
long g_heap_allocs = 0;
}

void* operator new(std::size_t size) {
    ++g_heap_allocs;
    if (void* p = std::malloc(size)) {
        return p;
    }
    throw std::bad_alloc{};
}

void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

int main(int argc, char** argv) {
    // Tick count. build_all.sh runs this at 50 AND at 1000: the fix's claim
    // is that the deadline path is allocation-free - zero is the one
    // per-tick number a longer session cannot scale - and one session
    // length cannot prove independence from session length.
    const long ticks = argc > 1 ? std::atol(argv[1]) : 1000;

    constexpr std::size_t kChannels = 16;
    constexpr std::size_t kSamples  = 1024;   // 21.3 ms of audio at 48 kHz

    // Setup may allocate freely; the session is what is on trial.
    std::vector<Block> inputs(kChannels);
    for (std::size_t ch = 0; ch < kChannels; ++ch) {
        inputs[ch].samples.resize(kSamples);
        for (std::size_t i = 0; i < kSamples; ++i) {
            const float sign = (i % 2 == 0) ? 1.0f : -1.0f;
            inputs[ch].samples[i] = sign * static_cast<float>(i % 100) / 200.0f;
        }
        inputs[ch].samples[ch * 3 + 7] = 0.75f;   // one planted peak, exactly
    }                                             // representable in a float

    Meter meter(kChannels);

    const long before = g_heap_allocs;
    const auto t0 = std::chrono::steady_clock::now();
    for (long t = 0; t < ticks; ++t) {
        meter.Tick(inputs);
    }
    const auto t1 = std::chrono::steady_clock::now();
    const long during = g_heap_allocs - before;

    // Correctness first: a faster meter that meters wrong is not a fix.
    for (std::size_t ch = 0; ch < kChannels; ++ch) {
        if (meter.Peak(ch) != 0.75f) {
            std::printf("FAILED: channel %zu peak %.3f, expected 0.750\n",
                        ch, static_cast<double>(meter.Peak(ch)));
            return 1;
        }
    }
    if (during != 0) {
        std::printf("FAILED: %ld heap allocations across %ld ticks - the"
                    " deadline path is copying\n", during, ticks);
        return 1;
    }

    // Printed to watch the fix move; MEASURE at -O2 without sanitizers -
    // under the canonical flags this number is not a benchmark.
    const auto ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    std::printf("meter ok: %zu channels x %zu samples, %ld ticks, 0 heap"
                " allocations, %.1f us/tick\n",
                kChannels, kSamples, ticks,
                ticks > 0 ? static_cast<double>(ns) / 1000.0
                                / static_cast<double>(ticks)
                          : 0.0);
    return 0;
}
