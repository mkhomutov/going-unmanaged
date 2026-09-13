// main.cpp - the judge for Chapter 43's hand-off, and the harness the lab is
// checked with. Two phases, one per direction the chapter teaches: a worker
// thread handing samples to a deadline thread, and an interrupt handler
// handing them to the loop it interrupted. POSIX only: the interrupt is a
// timer signal, which is what a desktop has instead of an ISR.
//
// Chapter 43 includes three pieces of this file between section markers:
// the counter ("The judge"), the worker phase and the interrupt phase. Edit
// here and the page follows.
#include "spsc_queue.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <thread>

#include <sys/time.h>

// --8<-- [start:counter]
// Chapter 36's instrument, given a thread. That harness counted the whole
// process, which was right for a process with one thread of interest; here
// the worker is ALLOWED to allocate and the deadline thread is not, so the
// counter asks which thread it is on. thread_local is the only way it can:
// nothing else identifies the caller inside a replaced operator new.
namespace {
thread_local bool t_on_deadline_path = false;
long g_deadline_allocs = 0;              // written by the deadline thread only

void* CountedAlloc(std::size_t size) {
    if (t_on_deadline_path) {
        ++g_deadline_allocs;
    }
    if (void* p = std::malloc(size)) {
        return p;
    }
    throw std::bad_alloc{};
}
}   // namespace

// Both forms, the lesson Recipe 49 paid for: under ASan new[] does not route
// through the scalar replacement, and a heap copy made by an array new
// would pass a counter that replaced only one.
void* operator new(std::size_t size) { return CountedAlloc(size); }
void* operator new[](std::size_t size) { return CountedAlloc(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
// --8<-- [end:counter]

namespace {

// The judge is CHECK, not assert: assert compiles to nothing under -DNDEBUG,
// and a harness that vanishes in Release while still printing its success
// line is worse than none (exercises/choosing/'s reason).
int g_failures = 0;
void Check(bool ok, const char* what) {
    if (!ok) {
        std::printf("FAILED: %s\n", what);
        ++g_failures;
    }
}

struct Sample {
    std::uint32_t seq;
    float value;
};

// Every wait in this file has a deadline - bridgelab's rule. A hand-off
// that stops delivering would otherwise stop CI rather than fail it.
bool Expired(std::chrono::steady_clock::time_point deadline) {
    return std::chrono::steady_clock::now() > deadline;
}

// --8<-- [start:worker]
// Phase 1: a worker thread produces, the deadline thread consumes. The
// worker may wait when the ring is full - it is the thread that CAN. The
// consumer drains a bounded number of samples per tick, which is what a
// deadline callback does: bounded work, then return to the host.
void WorkerToDeadlineThread() {
    constexpr std::uint32_t kItems = 200000;
    constexpr int kPerTick = 32;
    SpscQueue<Sample, 64> queue;

    // The bound, single-threaded first, because it is a claim about the
    // structure and not about timing: Capacity pushes succeed, the next is
    // refused, one pop makes room for exactly one more.
    for (std::uint32_t i = 0; i < queue.Capacity; ++i) {
        Check(queue.TryPush(Sample{i, 0.0f}), "the ring accepts Capacity samples");
    }
    Check(!queue.TryPush(Sample{99, 0.0f}), "the ring refuses the sample past Capacity");
    Sample popped{};
    Check(queue.TryPop(popped) && popped.seq == 0, "TryPop hands back the oldest sample");
    Check(queue.TryPush(Sample{99, 0.0f}), "one pop makes room for exactly one push");
    while (queue.TryPop(popped)) {
    }

    std::atomic<long> retries{0};                   // full-ring waits, on the worker's side
    std::thread worker([&] {
        for (std::uint32_t i = 0; i < kItems; ++i) {
            const Sample s{i, static_cast<float>(i) * 0.5f};
            while (!queue.TryPush(s)) {             // full: the worker waits, the
                retries.fetch_add(1, std::memory_order_relaxed);   // deadline side never does
                std::this_thread::yield();
            }
        }
    });

    t_on_deadline_path = true;                      // from here to the join, this thread is the deadline thread
    const long allocs_before = g_deadline_allocs;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    std::uint32_t expect = 0;
    long ticks = 0;
    long empty_ticks = 0;
    bool in_order = true;
    while (expect < kItems && !Expired(deadline)) {
        int drained = 0;
        Sample s{};
        while (drained < kPerTick && queue.TryPop(s)) {   // bounded work per tick
            if (s.seq != expect) {
                in_order = false;
            }
            ++expect;
            ++drained;
        }
        if (drained == 0) {
            ++empty_ticks;                          // nothing to do is a valid tick
        }
        ++ticks;
    }
    const long allocs_during = g_deadline_allocs - allocs_before;
    t_on_deadline_path = false;
    worker.join();

    Check(expect == kItems, "every sample the worker produced arrived (no deadline expired)");
    Check(in_order, "every sample arrived in the order it was produced, none twice");
    Check(allocs_during == 0, "the deadline thread allocated nothing across the whole session");
    std::printf("worker -> deadline thread: %u samples, %ld ticks (%ld empty), %ld full-ring retries by the worker, %ld allocations on the deadline thread\n",
                kItems, ticks, empty_ticks, retries.load(), allocs_during);
}
// --8<-- [end:worker]

// --8<-- [start:interrupt]
// Phase 2: the producer is an interrupt. A timer signal interrupts the main
// loop wherever it happens to be, the handler runs to completion on the
// SAME thread, and the loop resumes - which is exactly an ISR's relationship
// to the code it preempts. Everything the handler touches is at namespace
// scope, because a handler, like an ISR, takes no context pointer: it can
// only reach what exists before main runs.
SpscQueue<Sample, 16> g_from_interrupt;           // static storage: exists before main
std::atomic<std::uint32_t> g_interrupts{0};       // lock-free, so it is safe here
std::atomic<std::uint32_t> g_interrupt_drops{0};

void OnTimerInterrupt(int) {
    // Bounded, lock-free, allocation-free, and it never waits: if the ring
    // is full, the sample is dropped and the drop is counted. An interrupt
    // that waits for the loop it interrupted waits forever.
    const std::uint32_t n = g_interrupts.fetch_add(1, std::memory_order_relaxed);
    if (!g_from_interrupt.TryPush(Sample{n, 1.0f})) {
        g_interrupt_drops.fetch_add(1, std::memory_order_relaxed);
    }
}

void InterruptToMainLoop() {
    constexpr std::uint32_t kWanted = 200;

    struct sigaction action = {};
    action.sa_handler = &OnTimerInterrupt;
    sigemptyset(&action.sa_mask);
    sigaction(SIGALRM, &action, nullptr);

    struct itimerval every_ms = {};
    every_ms.it_value.tv_usec = 1000;
    every_ms.it_interval.tv_usec = 1000;
    setitimer(ITIMER_REAL, &every_ms, nullptr);    // from here, the handler can run between any two lines

    t_on_deadline_path = true;                      // the handler runs on THIS thread, so it is counted too
    const long allocs_before = g_deadline_allocs;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    std::uint32_t received = 0;
    std::uint32_t last_seq = 0;
    bool in_order = true;
    Sample s{};
    while (received < kWanted && !Expired(deadline)) {
        while (g_from_interrupt.TryPop(s)) {        // a dropped sample leaves a gap in seq,
            if (received > 0 && s.seq <= last_seq) {   // never a repeat and never a step back
                in_order = false;
            }
            last_seq = s.seq;
            ++received;
        }
    }
    const long allocs_during = g_deadline_allocs - allocs_before;
    t_on_deadline_path = false;

    struct itimerval off = {};
    setitimer(ITIMER_REAL, &off, nullptr);          // stop the interrupts...
    action.sa_handler = SIG_DFL;
    sigaction(SIGALRM, &action, nullptr);           // ...then uninstall: the order matters
    while (g_from_interrupt.TryPop(s)) {            // what the last interrupts left in the ring
        ++received;
    }

    const std::uint32_t fired = g_interrupts.load();
    const std::uint32_t dropped = g_interrupt_drops.load();
    Check(received >= kWanted, "the main loop received the samples the interrupts produced (no deadline expired)");
    Check(in_order, "every interrupt's sample arrived in order, none twice");
    Check(received + dropped == fired, "every interrupt either delivered its sample or counted a drop");
    Check(allocs_during == 0, "neither the handler nor the loop it interrupted allocated");
    std::printf("interrupt -> main loop: %u interrupts, %u samples received, %u dropped on a full ring, %ld allocations\n",
                fired, received, dropped, allocs_during);
}
// --8<-- [end:interrupt]

}   // namespace

int main() {
    WorkerToDeadlineThread();
    InterruptToMainLoop();
    if (g_failures != 0) {
        std::printf("deadlinelab: %d FAILED\n", g_failures);
        return 1;
    }
    std::printf("deadlinelab: the hand-off holds in both directions\n");
    return 0;
}
