## Chapter 18 — Exercise: The Device SDK

*Trains: Chapter 1 (RAII), Chapter 6 (move-only types — with a twist), Chapter 10 (lambdas/std::function), Chapter 16 Shape 2. Time: ~2 h. This is the peripheral-SDK idiom: after this exercise, libusb, PortAudio, HIDAPI, and serial-port APIs will all look familiar.*

### The vendor code

`FakeDevice.h` / `FakeDevice.cpp` — vendor code, do not edit. Three idioms live in this header, each worth reading twice:

```cpp
--8<-- "exercises/fakedevice/FakeDevice.h"
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
--8<-- "solutions/device_solution.cpp"
```

</details>

### The pitfalls, and what they generalize to

**The trampoline pattern is the whole chapter.** A C API can store only a function pointer — no captures, no state. The trick: register a *static* function whose only job is to cast the `void*` back to your object and forward the call. The context pointer is the closure's state, threaded through the C API by hand. Every callback-based C SDK — every one — is wrapped this way; write it once here and you will recognize it forever. (One nicety for a *real* C SDK, whose callback type has C language linkage: a static member function cannot be given C linkage, so the strictly conforming landing pad is a free `extern "C"` function that forwards into the class. Every mainstream ABI accepts the static member anyway, which is why you will see it everywhere.)

**The move twist: the context pointer aliases `this`.** The SDK stores the address of your session object as the callback context. Move the session, and the SDK still holds the *old* address — the moved-from husk. The next poll delivers a sample into a gutted object: at best a silent miss, at worst use-after-free when the husk is destroyed first. The reference's `Rebind()` in both move operations re-registers with the new `this`. The general lesson is bigger than this exercise: **any type that hands out pointers to itself (to an SDK, a callback registry, an observer list) must re-register on move — or delete its moves.** `std::function` members, timers, and observer patterns all carry this trap.

**Callback lifetime is a contract with the SDK.** The destructor closes the device, which (per the header) clears the callback — so the SDK can never call into a dead object *in this synchronous design*. Real device SDKs call back from driver threads, which adds two requirements the exercise deliberately excludes: unregister-then-join semantics in the destructor (ensure no callback is mid-flight when the object dies) and synchronization around everything the callback touches. When you meet a real SDK, ask its docs the Chapter 16 question: *what thread calls me back?* — and treat a missing answer as "a thread that isn't yours."

**Exceptions must not escape the trampoline.** The stack above the trampoline is C code (and in real SDKs, a driver). A throwing C++ callback unwinding into C is undefined behavior. Production trampolines wrap the forward in `try/catch(...)` and convert to a stored error or a log — the Chapter 8 boundary rule in its sharpest form. (The reference omits the guard for clarity; adding it is a worthy stretch goal.)

**Double-close prevention is the wrapper's reason to exist.** The SDK punishes double-close with an error; the wrapper makes it structurally impossible — `std::exchange` nulls the handle on move, the destructor tolerates null, and there is no public `Close` to call twice (add one as a stretch goal, and make it idempotent).

### Stretch goals

Add the `try/catch(...)` guard to the trampoline with a `LastError()` accessor. Add an idempotent public `Close()`. Store several sessions in a `std::vector<DeviceSession>` and verify callbacks survive the vector's reallocation (they will — because your move operations rebind; remove `Rebind()` and watch ASan report the `heap-use-after-free` — the SDK's stored context still points into the vector's freed old block — or, in an unsanitized build, watch the callbacks silently die instead; then explain the mechanism). Hardest: simulate the threaded case — call `Device_Poll` from a `std::thread` and make the sample collection race-free with a mutex, then explain why the destructor now needs more than it has.

**The wrapper you just wrote is not finished.** If your SDK calls back from a driver thread — most do — the missing part is not a mutex. [Chapter 29](29-concurrency.md#chapter-29--concurrency) works that case in full: why unregistering does not stop a callback already in flight, the weak reference and the alive flag that make a late one harmless, and why no ordering you can write makes freeing the context safe. `exercises/threadlab/` is this lab again with a driver thread in front of it. Stop here and read that chapter before shipping this shape against a real device — the synchronous version above is correct only because `FakeDevice` promised to be.
