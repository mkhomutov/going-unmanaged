## Chapter 36 — The Host Stutters

The first four tickets ended with a confession: a sanitizer named the crime, a counter reached zero, a hand decode agreed with the wire. This one — the fifth — arrives with evidence that appears to *acquit* you. The attached profile is genuine, professionally taken, correctly summarized by support — and the summary is wrong anyway, because the two numbers it leans on measure the one thing this ticket was never about. Performance work is where C++ reputations are made and lost in an SDK shop, and it starts here: not with making code fast, but with reading cost evidence without being lied to by an average. Same rule as [Chapter 33](33-here-is-the-report.md#chapter-33--here-is-the-report): **no compiler until your diagnosis is written down.** The two attachments and the listings below are sufficient.

### The ticket

> **#5788 — Audio dropouts with our meter loaded (since 2.1.0).** A
> mastering studio reports intermittent dropouts — a click every few
> minutes during long sessions, worse on their 16-channel template, gone
> the moment our meter plug-in is disabled. 2.1.0 is the release that
> added multi-channel support. Support profiled a five-hour reproduction
> on the studio's machine and closed the ticket as *not our bug*: "the
> plug-in is 6.7% of the render thread's busy time and 0.2% of the wall
> clock." The host vendor's engineer reopened it the same day with one
> sentence — *"Look under your own frame."* Both attachments are below.

A ticket with two attachments that disagree about your guilt is a gift, though it rarely feels like one. The skill this chapter trains is making them testify together.

### Attachment 1 — the profile

A sampling profile of the host's render (audio) thread: 300 seconds, one sample every millisecond, counts are samples. A sampler interrupts the thread on a clock and writes down the call stack it caught; a symbol's count is how often it was on the stack, which is a measure of *where the time went on average*. Total time for a frame includes its children; `[self]` is the frame alone. The host's own subtree is elided — the vendor notes, pointedly, that it contains no allocator symbols:

```text
9120  AudioEngine::RenderCycle(unsigned int)  (in HostCore)
  8349  [ the host's own processing - elided ]
   618  PluginHost::Process(PluginInstance&)  (in HostCore)
   + 452  Meter::Tick(std::vector<Block>)  (in meter.plugin)
   + !  341  Meter::Tick(std::vector<Block>)  [self]
   + !   57  _platform_memmove  (in libsystem_platform.dylib)
   + !   38  free  (in libsystem_malloc.dylib)
   + !   16  operator new(unsigned long)  (in libc++abi.dylib)
   +  161  std::vector<Block>::vector(std::vector<Block> const&)  (in meter.plugin)
   + !  129  _platform_memmove  (in libsystem_platform.dylib)
   + !   26  operator new(unsigned long)  (in libc++abi.dylib)
   + !    6  free  (in libsystem_malloc.dylib)
   +    5  PluginHost::Process(PluginInstance&)  [self]
   153  AudioEngine::RenderCycle(unsigned int)  [self]
```

Support's arithmetic checks out: our two frames total 613 samples of the thread's 9,120 busy ones — 6.7% — and 613 milliseconds of a 300-second capture is 0.2% of the wall clock. The render thread spends most of its life asleep, waiting for the audio device; nothing here is *slow*.

### Attachment 2 — the host's engine log

```text
[engine] render budget: 21.3 ms (1024 frames @ 48 kHz)
[engine] 00:41:02.114  overrun: cycle 24.9 ms  (miss 1)
[engine] 01:12:37.821  overrun: cycle 23.4 ms  (miss 2)
[engine] 02:03:11.006  overrun: cycle 26.1 ms  (miss 3)
        ...
[engine] session summary: 41 deadline misses in 05:04:56, max 26.1 ms
[engine] misses uncorrelated with UI, disk, or automation activity
```

The budget line is arithmetic, not policy: at 48,000 samples per second, a 1,024-sample buffer *is* 21.3 milliseconds — the thread must produce the next buffer before the hardware finishes playing this one, every time, or the listener hears the gap. Forty-one times in five hours, it didn't.

### The code it happened to

Reduced to the two files the profile points into, plus the session that drives them — the shape 2.1.0 shipped. The meter keeps a running peak per channel; the host calls `Tick` once per buffer, on the render thread, with one `Block` per channel.

`meter.h`'s class, as shipped — the declaration the profile's biggest frame belongs to:

```cpp
    // Called once per tick on the host's audio thread.
    void Tick(std::vector<Block> inputs);
```

`meter.cpp`'s `Tick`, as shipped:

```cpp
void Meter::Tick(std::vector<Block> inputs) {
    assert(inputs.size() == peaks_.size());
    std::size_t ch = 0;
    for (auto block : inputs) {
        float peak = peaks_[ch];
        for (auto s : block.samples) {
            peak = std::max(peak, std::fabs(s));
        }
        peaks_[ch] = peak;
        ++ch;
    }
}
```

It compiles clean under `-Wall -Wextra`. It runs clean under every sanitizer in [Chapter 13](13-toolchain-quick-reference.md#chapter-13--toolchain-quick-reference)'s canonical flags. Its output is correct on every buffer of every session. The code review that approved it read exactly what you just read.

### Try it — before reading on

The ticket card is `exercises/perflab/TASK.md`, with both attachments and the broken listings together (the files beside the card are the fixed reference — no peeking). Pencil before compiler:

1. **Read the profile with one question:** what does a peak meter's subtree have any *right* to contain? Write down every symbol under our two frames that is not arithmetic, and what it is doing there.
2. **Pair the attachments.** The budget is 21.3 ms; the worst miss is 26.1 ms. Of the symbols you listed, which have an *unbounded* worst case? What do they wait on — and who else in the process can be holding it?
3. **Grade support's arithmetic.** Both numbers are correct. Write one sentence on what kind of claim a mean can acquit, and why this ticket is not that kind of claim.
4. **Name the guilty characters.** There are two, one line each — and they are *absent*, not present.
5. **Only now, reproduce.** Recreate the broken `meter.h`/`meter.cpp` from the card next to the committed `main.cpp`, predict the allocation count per tick — the profile shows where every one comes from — then build and run the two translation units under the canonical flags: `scripts/check.sh meter.cpp main.cpp`. The harness is the acceptance test, and it fails with the number.
6. **Fix it and prove it.** Zero allocations at 50 ticks and at 1000 (the tick count is a run argument: `scripts/check.sh meter.cpp main.cpp 50`, then `1000`).
7. **Stretch:** build both versions `-O2` *without* sanitizers, time them, and see how little the mean moved. Then write the sentence support needed: why the ticket was real anyway.

### The diagnosis, walked through

<details>
<summary>Show the walkthrough — write your own diagnosis down first</summary>

Start where the vendor pointed: under your own frame. A peak meter's job is `fabs` and `max` over floats it does not own. Its profile subtree has a right to contain arithmetic — `[self]` — and nothing else. Now read what is actually there: `memmove`, `free`, `operator new` under `Tick`, and a whole sibling frame, `std::vector<Block>::vector(std::vector<Block> const&)`, that is nothing *but* `memmove` and allocator calls. A copy constructor, with our plug-in's name on it, running on the host's render thread. The question is no longer *is the plug-in slow*. It is *who asked for a copy?*

Nobody did — which is the C++ answer. Two declarations did, by omission. `void Tick(std::vector<Block> inputs)` takes its argument **by value**, so the host's call site constructs a complete copy of the session's blocks — outer vector, sixteen inner vectors, every sample — before `Tick` runs a single instruction. That is the sibling frame, and its position is a lesson in itself: the copy for a by-value argument is built *by the caller*, so it lands **outside** your function in the tree, under a `std::` name most eyes skip. And `for (auto block : inputs)` copies **again**, one whole `Block` per channel per tick, this time inside `Tick` — that is the `memmove`/`free`/`new` under your own frame. [Chapter 2](02-value-semantics.md#chapter-2--value-semantics) said every C++ type behaves like a C# struct on assignment; [Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency) turned that into the reflex *`const auto&` in loops — `auto` alone copies*. Here is the invoice: two missing ampersands, thirty-three trips through the allocator per tick — one outer vector plus sixteen blocks at the call site, sixteen more in the loop — at forty-seven ticks a second, for five hours.

Now pair that with the log. Thirty-three allocations per tick is not slow *on average* — `malloc` in the common case is a few dozen nanoseconds, and the profile says so: all the allocator frames together are a sliver. But the render thread's contract is not about the average. It must finish every 21.3 ms window, and the allocator is a **shared service with a lock and an unbounded worst case**: another thread holds the lock at the wrong moment, the allocator decides this call is the one that grows a zone or takes a page fault, and one `malloc` out of the day's four million takes milliseconds instead of nanoseconds. The budget is 21.3; the misses are 23–26. A sampler firing every millisecond will essentially never land inside an event that rare — the profile *structurally cannot see* the crime, only the criminal's name. That is why the percentages acquit while the symbols convict, and why the misses correlate with nothing the customer can watch: the trigger is another thread's allocation pattern.

Every part of the ticket now testifies. *Worse on the 16-channel template* — the copies scale with channel count. *Long sessions* — more draws from the lottery. *Gone when the plug-in is disabled* — the host's own subtree contains no allocator symbols, because the host's engineers wrote their render path under the rule you are about to adopt. *Support could not reproduce it* — an eight-channel rig on an idle machine buys half the tickets and a calm allocator.

One more honest number, because this chapter is about not being lied to by numbers: on this machine, the broken build averages ~25 µs per tick at `-O2` and the fixed one ~21 µs. The *mean* barely blinked — copying 128 KB through a warm cache is genuinely cheap — which is exactly why profile-by-averages let this ship, twice. The mean was never the crime. The worst case was.

</details>

### What the contract actually says

A name for the notes file: **the deadline path** — a real-time thread's callback, where the contract is not "be fast" but "be *bounded*". The host's documentation will state it somewhere, in words this blunt or blunter: on the audio thread, do not allocate, do not take locks, do not touch files or sockets, do not block for any reason — because the thread's product is not throughput, it is a deadline met every time. Every mechanism on that list shares one property: a worst case you do not control. The allocator is the one that hides best, because in C++ nothing in the *syntax* says "this line allocates" — a missing ampersand allocates, a by-value parameter allocates, `push_back` allocates on its schedule and not yours.

> [!NOTE]
> **Surprise for C# devs:** you already own this discipline — you called it *avoiding GC pressure*. Low-latency C# is a decade of exactly this: no allocations per frame, `ArrayPool<T>` and `stackalloc` and `struct` reuse, because gen-0 collections stop the world at the worst moment. The transfer is direct, with one inversion: in C# an allocation is spelled `new` and the collector's pause is the tail risk; here the allocation is spelled *nothing at all* — it hides in a signature, in an `auto` without `&` — and the allocator's lock is the tail risk. Same discipline, harder to see the trigger.

The second half of the contract is about evidence: **a mean can only acquit a mean.** A sampling profile answers "where does the time go, on average" — the right tool for throughput work, and support used it correctly. A deadline is a claim about the maximum, and the maximum of a rare event is invisible to a sampler by construction. When the complaint is a stutter, a dropout, a spike — read the profile not for its percentages but for its *names*: anything with an unbounded worst case, sitting on a thread with a deadline, is guilty regardless of its weight. The percentage column measures cost. It cannot measure risk.

### The fix

Two ampersands and a `const` — the cheapest fix in this book, in the two files the signature lives in ([Chapter 12](12-the-compilation-model.md#chapter-12--the-compilation-model): a declaration changes in the header *and* the definition). The fixed `meter.h`:

```cpp
#pragma once
#include <cstddef>
#include <vector>

// One channel's samples for one tick of the host's meter clock.
struct Block {
    std::vector<float> samples;
};

class Meter {
public:
    explicit Meter(std::size_t channels);

    // Called once per tick on the host's audio thread - the deadline path.
    // Borrows the blocks for the duration of the call: no copy, no
    // allocation, nothing that can block.
    void Tick(const std::vector<Block>& inputs);

    // Running peak for one channel, linear [0, 1].
    float Peak(std::size_t channel) const;

private:
    std::vector<float> peaks_;
};
```

And the fixed `meter.cpp`:

```cpp
#include "meter.h"
#include <algorithm>
#include <cassert>
#include <cmath>

Meter::Meter(std::size_t channels) : peaks_(channels, 0.0f) {}

void Meter::Tick(const std::vector<Block>& inputs) {
    assert(inputs.size() == peaks_.size());
    std::size_t ch = 0;
    for (const auto& block : inputs) {          // borrow - the second &
        float peak = peaks_[ch];
        for (const float s : block.samples) {   // a float: by value on purpose
            peak = std::max(peak, std::fabs(s));
        }
        peaks_[ch] = peak;
        ++ch;
    }
}

float Meter::Peak(std::size_t channel) const {
    assert(channel < peaks_.size());
    return peaks_[channel];
}
```

Note what the inner loop kept: `const float s`, by value, on purpose. A `float` *is* a register; copying it is the fast path, and an `&` there would buy nothing. The reflex is not "never copy" — it is "know what the thing weighs". [Chapter 14](14-exercise-the-lifetime-tracer.md#chapter-14--exercise-the-lifetime-tracer) counted constructor calls; this ticket is what they cost when the type is sixteen heap buffers deep.

The acceptance test is the part the first shipping missed, so it is the part the lab mechanizes. The harness — the whole file, because the counter *is* the chapter:

```cpp
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
```

Run against the broken meter, this harness fails with `33 heap allocations` per tick multiplied out; against the fix it prints zero. That number — not a timing — is the acceptance test, for the same reason the diagnosis could not come from the profile's percentages: a timing asserts about the mean, and the ticket was about the worst case. **Zero allocations is a claim about every tick at once**, including the four-millionth one on the studio machine, and it is immune to how fast the CI runner happens to be. `build_all.sh` runs it at 50 ticks and at 1000 because zero-per-tick is a claim of session-length independence, and one length cannot prove it. This is a real industry practice with a name — allocation tests — and it is the same idea as [Chapter 35](35-still-live-at-unload.md#chapter-35--still-live-at-unload)'s counter judging what the sanitizers cannot: pick the judge that can actually see the crime.

### Pitfalls

- **Measuring the Debug or sanitized build.** Under the canonical flags this harness reports ~450 µs per tick on this machine; at `-O2` plain, ~21. That is a factor of twenty, and it is not uniform across code shapes — sanitizers tax memory traffic hardest, so the *ratio* between your candidates lies too. Two builds, two questions, always: the sanitized build answers "is it correct", the `-O2` build answers "what does it cost". A benchmark of a Debug build is a rumour about a stranger.
- **"Take it by value and `std::move` it in."** Genuine advice — for *sinks*, functions that keep their argument (Chapter 6). Applied to a per-tick callback it fixes nothing: the callee's move is cheap, but the caller still builds the copy to move *from* — the profile's sibling frame, unmoved. A callback that only reads wants `const&`; by-value-plus-move is for functions that store.
- **Reaching for an object pool first.** Pools, arenas, and preallocation are the real tools for deadline code that *must* create things. But the first question is not "how do I allocate safely here" — it is "why does this path allocate at all?" This ticket's answer was "it never needed to"; a pool would have industrialized a copy nobody wanted.
- **Trusting the percentage column on a deadline thread.** "0.2% of wall clock" is a true statement about the mean and an empty statement about the tail. On a throughput workload, weight is guilt; on a deadline workload, *presence* is — any unbounded-worst-case symbol on the thread is a finding, at any percentage.
- **Fixing the floats too.** The reflex that removes the two heavy copies will itch to make the inner loop `const auto&` as well. Resist on principle: a `float` by reference saves nothing and costs an indirection the optimizer must then remove. The habit is *know the weight*, not *fear the copy* — [Chapter 2](02-value-semantics.md#chapter-2--value-semantics)'s point, full circle.

> [!TIP]
> **Key principle:** "On a deadline thread I treat the allocator as I/O: the hot path allocates nothing, locks nothing, and blocks on nothing — and I prove it with a counter, because a profiler's mean flatters while a deadline punishes the worst case."

### In the wild

The rule this chapter derives is written, in almost these words, in every real-time SDK's documentation — audio plug-in APIs state "never allocate or lock in the process callback" as flatly as a license term, and game engines run the same discipline per frame: a 60 Hz title has a 16.7 ms budget, and engine teams budget allocations per frame at *zero* outside explicit arenas for exactly this chapter's reason. The tooling has caught up, too: recent Clang ships **RealtimeSanitizer** (`-fsanitize=realtime`) — mark a function `[[clang::nonblocking]]` and the run aborts the moment anything on that path allocates, locks, or blocks. That is this chapter's counter, promoted into the sanitizer family; it is not in this book's canonical flags because it needs a newer toolchain than the baseline, but know it exists, because it turns the deadline contract from documentation into a build failure. And your C# past has the same institution: the low-latency .NET world runs allocation-count assertions in CI — the harness above with more ceremony — because "no allocations on this path" rots the day someone adds an innocent-looking line, in any language. The counter is how the rule survives its authors.

### Reproduce it cold

A week or two from now, closed book: write the allocation-counting harness from memory — the replaced `operator new` and `delete`, a counter, a before/after around the code on trial — and point it at any loop you believe is clean. Then re-state, in one sentence each: what a sampling profile can prove, what it structurally cannot, and what zero allocations proves that a fast timing does not. If you can also name where a by-value argument's copy shows up in a call tree — whose frame, whose name — the profile will never acquit the wrong party in front of you again. The schedule is in [Chapter 24](24-practice-plan.md#chapter-24--practice-plan).

<!-- nav:begin -->
[← Chapter 35 — Still Live at Unload](35-still-live-at-unload.md) · [Contents](README.md) · [Chapter 37 — No Repro, Dump Attached →](37-no-repro-dump-attached.md)
<!-- nav:end -->
