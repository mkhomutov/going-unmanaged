# The Host Stutters — ticket card (Chapter 36)

This lab is a **ticket, not a task** — the fifth one, and the first where
the attached evidence appears to *acquit* you. Chapter 36 states everything
in full and walks the diagnosis behind a spoiler fold — work the ticket
cold first, and work it **on paper first**: no compiler until your
diagnosis is written down.

> **#5788 — Audio dropouts with our meter loaded (since 2.1.0).** A
> mastering studio reports intermittent dropouts — a click every few
> minutes during long sessions, worse on their 16-channel template, gone
> the moment our meter plug-in is disabled. 2.1.0 is the release that
> added multi-channel support. Support profiled a five-hour reproduction
> on the studio's machine and closed the ticket as *not our bug*: "the
> plug-in is 6.7% of the render thread's busy time and 0.2% of the wall
> clock." The host vendor's engineer reopened it the same day with one
> sentence — *"Look under your own frame."* Both attachments are below.

**The files beside this card are the FIXED reference** — the lab's green
state, run by `build_all.sh` at two different session lengths on every
push. Do not start from them. Recreate the broken 2.1.0 code below in a
scratch directory of your own, and work from there.

## Attachment 1 — the profile

A sampling profile of the host's render (audio) thread, 300 seconds at
1 ms, from the studio machine. Counts are samples. The host's own subtree
is elided — the vendor notes, pointedly, that it contains no allocator
symbols:

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

## Attachment 2 — the host's engine log

```text
[engine] render budget: 21.3 ms (1024 frames @ 48 kHz)
[engine] 00:41:02.114  overrun: cycle 24.9 ms  (miss 1)
[engine] 01:12:37.821  overrun: cycle 23.4 ms  (miss 2)
[engine] 02:03:11.006  overrun: cycle 26.1 ms  (miss 3)
        ...
[engine] session summary: 41 deadline misses in 05:04:56, max 26.1 ms
[engine] misses uncorrelated with UI, disk, or automation activity
```

## The code as 2.1.0 shipped it

`meter.h` and `meter.cpp` — as beside this card, except the header's
`Tick` declaration and its comment read:

```cpp
    // Called once per tick on the host's audio thread.
    void Tick(std::vector<Block> inputs);
```

and `meter.cpp`'s `Tick` is:

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

`main.cpp` — the simulated session and the lab's judge — is unchanged:
use the one beside this card as-is.

## Work the ticket

1. **Read the profile with one question — no compiler.** What does a peak
   meter's subtree have any right to contain? Write down every symbol
   under our two frames that is not arithmetic, and what each one is
   doing there.
2. **Pair the attachments.** The budget is 21.3 ms and the worst miss is
   26.1 ms. Of the symbols you just listed, which have an *unbounded*
   worst case? What do they wait on, and who else in the process can be
   holding it?
3. **Grade support's arithmetic.** 6.7% of the thread, 0.2% of the wall
   clock — both numbers are correct. Write one sentence on what kind of
   claim they can acquit, and why this ticket is not that kind of claim.
4. **Name the guilty characters.** There are two, one line each — and
   they are *absent*, not present. Write the two lines as they should
   have been.
5. **Only now, reproduce.** Recreate the broken `meter.h`/`meter.cpp`
   from the listings above, next to the committed `main.cpp`. Predict the
   allocation count per tick before you run — the profile shows where all
   of them come from — then:
   `../../scripts/check.sh meter.cpp main.cpp`
   The harness is the acceptance test, and it fails with the number.
6. **Fix it and prove it.** Zero allocations, at 50 ticks and at 1000
   (the tick count is a run argument:
   `../../scripts/check.sh meter.cpp main.cpp 50`, then `1000`) — the
   claim is that the deadline path stopped allocating, and one session
   length cannot prove independence from session length.
7. **Stretch: measure what the fix did to the mean.** Build both versions
   `-O2` *without* sanitizers, time a long run of each, and see how
   little the average moved — then write one sentence on why the ticket
   was real anyway. Profile the fixed build with your platform's sampler
   and find the allocator symbols gone.
