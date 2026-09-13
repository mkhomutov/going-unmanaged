# The Deadline Lab — task card (Chapter 43)

Chapter 29 hands a callback on a foreign thread a mutex and a `push_back`;
Chapter 36 forbids both on a thread with a deadline. This lab is what goes
in their place: a bounded ring between one producer and one consumer, two
atomic indices, and the two memory orders that make it correct — first
between a worker thread and a deadline thread, then between an interrupt
handler and the loop it interrupted. Chapter 43 states everything in full.

**The files beside this card are the worked result** — `spsc_queue.h` (the
ring) and `main.cpp` (the harness that judges it: order, completeness, zero
allocations on the deadline thread, every wait bounded), kept green by
`build_all.sh` under ASan/UBSan and again under TSan on every push. Do not
start from them. Build your own in a scratch directory and compare after.

## Build the hand-off

1. **The ring, from the constraints.** One producer thread, one consumer
   thread, a fixed array of `N` slots, two `std::atomic<std::size_t>`
   indices. `TryPush(const T&)` returns `false` when full; `TryPop(T&)`
   returns `false` when empty; neither ever waits. Decide how full and
   empty are told apart from the two indices alone. Then place the memory
   orders: for each load and store, write down which thread reads it and
   what must be visible by the time it is. Each index has exactly one
   writer — that sentence is the whole design.
2. **The harness.** Number every sample. A worker thread pushes two hundred
   thousand of them, waiting (yielding) when the ring is full; the main
   thread, playing the deadline thread, drains at most thirty-two per tick
   and returns. Assert that every sample arrived, in order, once — and
   that the deadline thread allocated **nothing**: replace `operator new`
   (both forms) with a counter that counts only on a `thread_local` flag.
   Give the whole run a deadline. Build and run under
   `scripts/check.sh main.cpp`, then `SAN=thread scripts/check.sh main.cpp`.
3. **Weaken it.** Replace every `acquire` and `release` with `relaxed` and
   run twenty times. Record what you see, and on which machine. Restore
   the orders.
4. **Put Chapter 29's fix on the deadline thread.** A struct with a
   `std::mutex` and a `std::vector<Sample>`, the worker locking and
   pushing, the deadline thread locking and draining. Run the harness and
   read the allocation count. Say what the counter cannot see.
5. **Make the producer an interrupt.** Install a `SIGALRM` handler with
   `sigaction`, start a one-millisecond `setitimer`, and push from the
   handler into a ring at namespace scope — the handler takes no context
   pointer, so nothing it touches can be a local. Count drops instead of
   waiting. Drain in the main loop until two hundred samples have arrived,
   stop the timer, restore the handler, and assert order and zero
   allocations on the thread — which now covers the handler too.
6. **Break it** — the lock-in-a-handler listing at the end of Chapter 43's
   interrupt-context section — and watch what happens under
   `scripts/check.sh`, then `SAN=thread`, then `SAN=none`.

## The shapes that fail

Three shapes that exist to fail. None is in the lab directory: two are
here and in Chapter 43, identically, and the third is compiled by
`check_platform_claims.sh` and shown by the chapter from there.

**The relaxed ring** — the listing in `spsc_queue.h` with every
`std::memory_order_acquire` and `std::memory_order_release` replaced by
`std::memory_order_relaxed`. It passes every run on x86-64. On Apple
silicon, a harness that numbers its samples printed, in six runs of eight:

```text
stale: got seq 46983, expected 47047
```

Sixty-four behind: the sample that occupied that slot one lap ago.

**Chapter 29's fix on the deadline thread** — correct for a device
callback, disqualified here, and invisible to the allocation counter:

```cpp
struct Sink {
    std::mutex m;
    std::vector<Sample> samples;     // grows on the worker's side
};

// The deadline thread's tick. The counter reads zero: nothing here
// allocates on THIS thread. The lock is the bug, and no counter sees it.
void Tick(Sink& sink) {
    std::lock_guard<std::mutex> g(sink.m);     // held by a worker the scheduler
    for (const Sample& s : sink.samples) {     // has just preempted: the
        Consume(s);                            // deadline thread waits for it
    }
    sink.samples.clear();
}
```

**A lock in interrupt context** — the handler takes the mutex the loop it
interrupted is holding, and `raise` delivers the signal while the lock is
held. One thread, and it waits for itself. The listing is the last one in
Chapter 43's interrupt-context section, and the only one of the three that
is compiled anywhere: `check_platform_claims.sh` generates it and asserts
that the plain build never returns, bounded, because a demonstration whose only outcome
is silence needs a script that treats silence as the answer. It never
prints. Plain, under ASan/UBSan and under TSan, the process sits there —
every sanitizer is waiting too.
