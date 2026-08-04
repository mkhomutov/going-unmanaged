# Exercise: The Threaded Callback (the driver thread, finally)

Full statement, the design it arrives at, and the pitfalls: **Chapter 29** of
the book, *Try it*. This is Chapter 18's hardest stretch goal, and the chapter
answers it — so do it COLD, all the way to a clean run, before reading. ~2 h.

*Trains: Chapter 29 (data races, `std::thread`'s join obligation, RAII locks,
ThreadSanitizer); Chapter 16's fourth question — *what thread calls me back?* —
answered rather than asked; callback lifetime across a thread boundary, which is
Chapter 22's dangling capture with timing on top.*

Vendor code: `../fakedevice/FakeDevice.h` / `.cpp` — the same device as
Chapter 18, unchanged. Read, compile, link, never edit. Nothing in it needs to
change: you build the driver thread yourself, out of `Device_Poll`.

## The task

1. **Make it threaded.** Call `Device_Poll` from a `std::thread` while the main
   thread reads the samples. Build under `-fsanitize=thread` and read the
   report. You may need several runs, or more load, before the race is
   *observable* — TSan reports it either way, which is the point.
2. **Fix problem one — shared state.** A mutex around the sample collection, on
   both sides. TSan goes quiet; the samples stay right.
3. **Fix problem two — lifetime.** `FakeDevice.h` says nothing about calling the
   API from two threads at once, so by Chapter 16's rule it is not thread-safe:
   confine every `Device_*` call to the polling thread, and post registration
   and unregistration to it as jobs on a mutex-guarded queue. Notice what that
   buys you — unregistering is now **deferred**, so a callback really can arrive
   after the session is gone. Then restructure the callback around a weak
   reference plus an alive flag, and destroy sessions in a loop on the main
   thread while the poller runs.
4. **Break it deliberately, one change at a time.** Put `delete ctx` back at the
   end of the destructor and run the ASan build ten or twenty times rather than
   once — some runs pass, and only one of the two outcomes is the truth about
   the code. Then restore it and remove only the alive flag, predicting the
   failure before you run it.
5. **Explain it out loud** (the Chapter 24 narration drill): why the flag is
   published *before* the unregister, why the callback takes a weak reference
   rather than a strong one, and why no ordering you can write makes deleting
   the context safe.

Step 4 exists to fail, so keep it out of any script you wire up.

## Build

Two builds, not one flag list — ThreadSanitizer cannot be combined with
AddressSanitizer, and they answer different questions:

```
g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -g \
    ../fakedevice/FakeDevice.cpp your.cpp -I ../fakedevice -o threaded
g++ -std=c++17 -Wall -Wextra -fsanitize=thread -g \
    ../fakedevice/FakeDevice.cpp your.cpp -I ../fakedevice -o threaded-tsan
```

or, from this directory:

```
../../scripts/check.sh your.cpp fakedevice              # ASan + UBSan
SAN=thread ../../scripts/check.sh your.cpp fakedevice   # ThreadSanitizer
# Windows: ..\..\scripts\check.ps1 your.cpp fakedevice covers the ASan half;
# MSVC has no TSan, so run the SAN=thread build under clang/gcc (WSL works).
```

## Done means

- **Clean under `-fsanitize=thread`** and **clean under
  `-fsanitize=address,undefined`**, from separate builds of the same source.
- **Clean on every run, not on a run.** Races are timing-dependent: repeat each
  build a dozen times or more before believing it. One quiet run proves nothing
  — that is the whole argument of Chapter 29.
- **`FakeDevice_OpenHandles() == 0`** at the end, as in Chapter 18. A driver
  thread repeals no obligation.
- **Assertions about values, not about survival** (Chapter 25, Finding 10). "It
  did not crash" is not a result. Assert the exact samples where the run is
  deterministic, and a property that holds regardless where it is not.
- **Termination is decided, not hoped for.** A stop flag and a join; no
  unbounded loop, and no sleep standing in for synchronization. (A sleep to
  *widen* a race window is a different thing and is fine.)

Reference solution: [`solutions/device_threaded_solution.cpp`](../../solutions/device_threaded_solution.cpp)
— it implements step 3 in full, including the file-scope context registry the
chapter mentions in passing, which is what lets the program be clean under
LeakSanitizer as well. Read it after your own runs, not before.
