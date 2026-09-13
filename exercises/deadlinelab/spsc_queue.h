// spsc_queue.h - the hand-off to a deadline thread: one producer, one
// consumer, a fixed ring, two atomic indices, and no operation that can
// wait. Chapter 36 said what a deadline thread must not do; this is what it
// does instead.
//
// Included IN FULL by Chapter 43 ("The hand-off") from below this banner:
// edit here and the page follows. Write your own first - the lab's task card
// asks you to build it from the chapter's constraints, and comparing
// afterwards is the point.
// --8<-- [start:listing]
#pragma once
#include <atomic>
#include <cstddef>
#include <type_traits>

// The distance two counters must keep so that two cores do not fight over
// one cache line. Appendix L owns the argument about the number: it is the
// distance this code CHOSE, not a fact about every machine, and 128 covers
// the common 64-byte line twice over and Apple silicon's once.
inline constexpr std::size_t kSeparation = 128;

// A bounded single-producer, single-consumer queue. Exactly one thread ever
// calls TryPush, exactly one ever calls TryPop, and that is the whole reason
// it needs no lock: each index has ONE writer, so the only question is when
// the other thread gets to see it - which is what the memory orders answer.
// One slot is always kept empty, so "full" and "empty" are told apart by the
// indices alone: Capacity is N - 1.
template <class T, std::size_t N>
class SpscQueue {
    static_assert(N > 1, "an SpscQueue needs at least two slots: one is always kept empty");
    // The deadline side copies a slot out. A T whose copy allocates or
    // throws would put the allocator back on the path this queue exists to
    // keep it off - and in interrupt context there is no path to put it on.
    static_assert(std::is_trivially_copyable_v<T>, "slots are copied on the deadline path: the element type must be trivially copyable");
    // An atomic that is not lock-free is a mutex in disguise, and a mutex is
    // what this whole file exists to avoid. On every desktop target a size_t
    // is lock-free; the assert is for the target where it is not.
    static_assert(std::atomic<std::size_t>::is_always_lock_free, "the indices must be lock-free, or the queue takes a lock on the deadline path");

public:
    static constexpr std::size_t Capacity = N - 1;

    // Producer thread only. Returns false when the ring is full: the caller
    // decides whether to wait, retry or drop - this side may be allowed to
    // wait, and this function never does it on the caller's behalf.
    bool TryPush(const T& value) noexcept {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);   // mine: nobody else writes it
        const std::size_t next = tail + 1 == N ? 0 : tail + 1;
        if (next == head_.load(std::memory_order_acquire)) {              // the consumer's: acquire, so
            return false;                                                  // the slot it freed is really free
        }
        slots_[tail] = value;                                              // write the slot...
        tail_.store(next, std::memory_order_release);                      // ...THEN publish it: release
        return true;                                                       // orders the slot before the index
    }

    // Consumer thread only - the deadline thread, or the interrupt handler.
    // Returns false when the ring is empty. Never waits, never allocates,
    // never blocks: every line here is bounded.
    bool TryPop(T& out) noexcept {
        const std::size_t head = head_.load(std::memory_order_relaxed);   // mine
        if (head == tail_.load(std::memory_order_acquire)) {              // the producer's: acquire pairs
            return false;                                                  // with its release, so the slot
        }                                                                  // is visible before the index is
        out = slots_[head];
        head_.store(head + 1 == N ? 0 : head + 1, std::memory_order_release);   // the slot is free: release,
        return true;                                                             // so the producer's acquire sees it
    }

private:
    // Each index on its own line: the producer writes tail_ and the consumer
    // writes head_, and two cores writing one line take turns owning it.
    alignas(kSeparation) std::atomic<std::size_t> head_{0};
    alignas(kSeparation) std::atomic<std::size_t> tail_{0};
    alignas(kSeparation) T slots_[N] = {};
};
// --8<-- [end:listing]
