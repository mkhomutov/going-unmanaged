## Chapter 18 — Exercise: The Device SDK

*Trains: Chapter 1 (RAII), Chapter 6 (move-only types — with a twist), Chapter 10 (lambdas/std::function), Chapter 16 Shape 2. Time: ~2 h. This is the peripheral-SDK idiom: after this exercise, libusb, PortAudio, HIDAPI, and serial-port APIs will all look familiar.*

### The vendor code

`FakeDevice.h` / `FakeDevice.cpp` — vendor code, do not edit. Three idioms live in this header, each worth reading twice:

```cpp
// ============================================================================
// FakeDevice.h - a miniature peripheral-device SDK in the classic C idiom:
// opaque handles, open/close lifecycle, and callbacks with a void* context.
// This is the shape of libusb, HIDAPI, PortAudio, serial-port and most
// vendor device SDKs. DO NOT MODIFY. Read it, wrap it, obey it.
// ============================================================================
#pragma once
#include <cstddef>

using DevErr = int;
constexpr DevErr DevOk        = 0;
constexpr DevErr DevNullParam = 1;
constexpr DevErr DevNotFound  = 2;   // no device with that name
constexpr DevErr DevClosed    = 3;   // operation on a closed/invalid handle
constexpr DevErr DevBusy      = 4;   // open() on an already-open device

// Opaque handle: you get a pointer to a type you cannot see inside.
// The SDK owns the memory behind it; you own the OBLIGATION to Close it.
struct DeviceImpl;
using DeviceHandle = DeviceImpl*;

// The C callback idiom: a plain function pointer plus a caller-supplied
// context pointer, passed back verbatim on every invocation. This pair is
// how C APIs deliver events into YOUR code - no closures exist in C.
using SampleCallback = void(*)(int sample, void* userContext);

// Open a device by name ("sensor0".."sensor3" exist). On success writes a
// handle you MUST eventually pass to Device_Close exactly once.
DevErr Device_Open(const char* name, DeviceHandle* outHandle);

// Close and invalidate the handle. Safe to call with null (*no-op*).
// Double-close of the same handle is an error your wrapper must prevent.
DevErr Device_Close(DeviceHandle h);

// Register (or clear, with nullptr) the sample callback for this device.
// The context pointer is stored verbatim and handed back on every sample.
DevErr Device_SetCallback(DeviceHandle h, SampleCallback cb, void* userContext);

// Ask the device to deliver its pending samples NOW, synchronously, by
// invoking the registered callback once per sample on THIS thread.
// (Real SDKs often call back from a driver thread - see the chapter notes.)
DevErr Device_Poll(DeviceHandle h);

// Test-support: number of handles currently open. Must be 0 when you finish.
size_t FakeDevice_OpenHandles();
// Test-support: preload N pending samples (values 100, 101, ...) on a device.
DevErr FakeDevice_InjectSamples(DeviceHandle h, size_t n);
```

**The opaque handle** — `DeviceHandle` is a pointer to a struct whose definition you never see. You cannot copy the device, inspect it, or free it yourself; the handle is a claim ticket, and `Device_Close` is the only way to redeem it. **The open/close lifecycle** — open hands out the obligation; double-close is an *error*, not a no-op, so your wrapper must guarantee exactly-once. **The C callback pair** — a plain function pointer plus a `void*` context returned to you verbatim: this is how C delivers events into your code, because C has no closures. Bridging it to C++ closures is the heart of the exercise.

### The task

**Part A — `DeviceSession`**: a **move-only** RAII wrapper. Unlike Chapter 17's guard (one struct, one scope, copy and move both deleted), a device session is an ownable resource you may want to store in containers or return from factories — so it gets the full Chapter 6 treatment: deleted copies, real moves, `noexcept`, exactly-once close. Opening can fail, and constructors can't return error codes — design around that (the reference uses a static factory writing into an out-parameter, the SDK's own style; returning `std::optional<DeviceSession>` is an equally defensible alternative).

**Part B — the trampoline**: an `OnSample(std::function<void(int)>)` method letting callers register a real C++ closure, bridged to the SDK's C callback via a static function and the `void*` context.

**Part C — prove it**: open, register a lambda capturing a local vector, inject and poll, assert the exact samples arrived; **move the session and verify callbacks still land** (this is the twist — predict what breaks before testing); exercise the error paths (`DevBusy`, `DevNotFound`); and assert `FakeDevice_OpenHandles() == 0` at the end.

### Reference solution

<details>
<summary><strong>Show the solution — do the exercise cold first</strong></summary>

```cpp
// Device SDK exercise - reference solution.
#include "FakeDevice.h"
#include <cassert>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

// Part A - RAII session. MOVE-ONLY: unlike ThingDataGuard (one struct, one
// scope), a device session is an ownable resource you may want to store in
// containers or return from factories - so it gets the full move treatment.
class DeviceSession {
public:
    DeviceSession() = default;                       // empty session

    static DevErr Open(const char* name, DeviceSession& out) {
        DeviceHandle h = nullptr;
        DevErr err = Device_Open(name, &h);
        if (err != DevOk) return err;
        out = DeviceSession(h);                      // move-assign into caller
        return DevOk;
    }

    ~DeviceSession() { Device_Close(h_); }           // null-safe by contract

    DeviceSession(const DeviceSession&) = delete;    // copying a handle would
    DeviceSession& operator=(const DeviceSession&) = delete;   // double-close

    DeviceSession(DeviceSession&& o) noexcept
        : h_(std::exchange(o.h_, nullptr)),
          onSample_(std::move(o.onSample_)) {
        Rebind();                                    // ctx points at *this* -
    }                                                // it moved, so re-register!

    DeviceSession& operator=(DeviceSession&& o) noexcept {
        if (this != &o) {
            Device_Close(h_);
            h_ = std::exchange(o.h_, nullptr);
            onSample_ = std::move(o.onSample_);
            Rebind();
        }
        return *this;
    }

    bool IsOpen() const { return h_ != nullptr; }

    // Part B - the trampoline: bridge the C callback to std::function.
    DevErr OnSample(std::function<void(int)> fn) {
        onSample_ = std::move(fn);
        return Rebind();
    }

    DevErr Poll() { return h_ ? Device_Poll(h_) : DevClosed; }
    DevErr Inject(size_t n) { return h_ ? FakeDevice_InjectSamples(h_, n) : DevClosed; }

private:
    explicit DeviceSession(DeviceHandle h) : h_(h) {}

    static void Trampoline(int sample, void* ctx) {  // the C-shaped landing pad
        auto* self = static_cast<DeviceSession*>(ctx);
        if (self->onSample_) self->onSample_(sample);
    }

    DevErr Rebind() {
        if (!h_) return DevOk;
        return onSample_
            ? Device_SetCallback(h_, &Trampoline, this)
            : Device_SetCallback(h_, nullptr, nullptr);
    }

    DeviceHandle h_ = nullptr;
    std::function<void(int)> onSample_;
};

int main() {
    {
        DeviceSession s;
        DevErr err = DeviceSession::Open("sensor0", s);
        assert(err == DevOk && s.IsOpen());
        assert(FakeDevice_OpenHandles() == 1);

        std::vector<int> got;
        s.OnSample([&got](int v) { got.push_back(v); });   // a real closure,
        s.Inject(3);                                       // riding a C API
        s.Poll();
        assert((got == std::vector<int>{100, 101, 102}));
        std::cout << "callbacks ok: got " << got.size() << " samples\n";

        // move the session - the trampoline context must follow it
        DeviceSession s2 = std::move(s);
        assert(!s.IsOpen() && s2.IsOpen());
        s2.Inject(1);
        s2.Poll();
        assert(got.size() == 4);                    // still lands in 'got'
        std::cout << "moved session still delivers: " << got.back() << "\n";

        // error paths
        DeviceSession dup;
        assert(DeviceSession::Open("sensor0", dup) == DevBusy);   // already open
        DeviceSession nope;
        assert(DeviceSession::Open("sensor9", nope) == DevNotFound);
    }   // s2's destructor closes; s's destructor closes nothing (null handle)

    assert(FakeDevice_OpenHandles() == 0);          // the leak check
    std::cout << "all handles closed\n";
    return 0;
}
```

</details>

### The pitfalls, and what they generalize to

**The trampoline pattern is the whole chapter.** A C API can store only a function pointer — no captures, no state. The trick: register a *static* function whose only job is to cast the `void*` back to your object and forward the call. The context pointer is the closure's state, threaded through the C API by hand. Every callback-based C SDK — every one — is wrapped this way; write it once here and you will recognize it forever.

**The move twist: the context pointer aliases `this`.** The SDK stores the address of your session object as the callback context. Move the session, and the SDK still holds the *old* address — the moved-from husk. The next poll delivers a sample into a gutted object: at best a silent miss, at worst use-after-free when the husk is destroyed first. The reference's `Rebind()` in both move operations re-registers with the new `this`. The general lesson is bigger than this exercise: **any type that hands out pointers to itself (to an SDK, a callback registry, an observer list) must re-register on move — or delete its moves.** `std::function` members, timers, and observer patterns all carry this trap.

**Callback lifetime is a contract with the SDK.** The destructor closes the device, which (per the header) clears the callback — so the SDK can never call into a dead object *in this synchronous design*. Real device SDKs call back from driver threads, which adds two requirements the exercise deliberately excludes: unregister-then-join semantics in the destructor (ensure no callback is mid-flight when the object dies) and synchronization around everything the callback touches. When you meet a real SDK, ask its docs the Chapter 16 question: *what thread calls me back?* — and treat a missing answer as "a thread that isn't yours."

**Exceptions must not escape the trampoline.** The stack above the trampoline is C code (and in real SDKs, a driver). A throwing C++ callback unwinding into C is undefined behavior. Production trampolines wrap the forward in `try/catch(...)` and convert to a stored error or a log — the Chapter 8 boundary rule in its sharpest form. (The reference omits the guard for clarity; adding it is a worthy stretch goal.)

**Double-close prevention is the wrapper's reason to exist.** The SDK punishes double-close with an error; the wrapper makes it structurally impossible — `std::exchange` nulls the handle on move, the destructor tolerates null, and there is no public `Close` to call twice (add one as a stretch goal, and make it idempotent).

### Stretch goals

Add the `try/catch(...)` guard to the trampoline with a `LastError()` accessor. Add an idempotent public `Close()`. Store several sessions in a `std::vector<DeviceSession>` and verify callbacks survive the vector's reallocation (they will — because your move operations rebind; remove `Rebind()` and watch them silently die, then explain the mechanism). Hardest: simulate the threaded case — call `Device_Poll` from a `std::thread` and make the sample collection race-free with a mutex, then explain why the destructor now needs more than it has.

---


<!-- nav:begin -->
[← Chapter 17 — Exercise: The FakeSDK](17-exercise-the-fakesdk.md) · [Contents](README.md) · [Chapter 19 — Exercise: The Word Counter →](19-exercise-the-word-counter.md)
<!-- nav:end -->
