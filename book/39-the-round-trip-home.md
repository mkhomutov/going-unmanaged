## Chapter 39 — The Round Trip Home

[Chapter 30](30-authoring-an-abi-boundary.md#chapter-30--authoring-an-abi-boundary) ended on the `extern "C"` façade — the shape that survives crossing a binary boundary between two compilers, and the shape every SDK in this book chose. This chapter is what happens when the thing on the other side of that boundary is the runtime you came from.

It is a near-certain assignment. The native library is working, and now somebody wants a test harness, or an internal tool, or a UI, and the fastest way to write those is the language your team already knows. So you publish a C surface and bind it from C#, and the surface barely changes from the one Chapter 30 taught you to write.

What changes is who reads your header. Not a compiler — a person, transcribing it into a second language by hand.

### The declaration is written twice, and nothing compares them

A `[DllImport]` (or, on modern .NET, a `[LibraryImport]`) declaration is a *claim* about your ABI, written in C#, compiled by a C# compiler that has never seen your header and cannot see it. Your side declares the function once. Their side declares it again. Nothing anywhere checks that the two agree.

That is worth sitting with, because it is almost certainly the first time in this reader's career that a signature is not checked by anything. In C#, calling a method with the wrong argument types is a compile error before you have finished typing. Here both sides compile, both sides link, the program runs, and the disagreement shows up as a wrong number or a corrupted heap somewhere else entirely.

You have seen this exact mechanism before. [Chapter 27](27-dependency-management.md#chapter-27--dependency-management)'s ODR diamond was two declarations of one struct in one program, and the linker silently picked one. This is the same failure with the safety rail removed: there is no linker here, because the two declarations are in different languages and never meet.

> [!NOTE]
> **The big reveal:** P/Invoke is not a feature that connects two type systems. It is a promise you make twice, in two languages, that nothing verifies — and the native half is the half that gets blamed.

Which sets the whole job. You cannot make the managed declaration correct from here. What you *can* do is publish a surface that is hard to transcribe wrongly, and that says so at runtime when somebody has.

### Blittable is the word that matters

A **blittable** type has the same representation in managed and native memory. The marshaller pins it and passes its address: no copy, no conversion, nothing to get wrong. The list is short and worth memorising, because it is the whole of your palette.

| Blittable | Not blittable |
|---|---|
| `int`/`uint`, `short`/`ushort`, `long`/`ulong`, `byte`/`sbyte` | `bool` — one byte native, and the marshaller's default is a 4-byte Windows `BOOL` |
| `float`, `double` | `char` and `string` — encoding is a conversion, and a conversion is a copy |
| `IntPtr`, `UIntPtr`, and any pointer | arrays of non-blittable things, and anything holding a reference |
| a struct whose fields are *all* blittable | a struct with one non-blittable field anywhere in it |

The reason this matters more to you than to them: a blittable struct is passed by address, so an out-parameter you write into is the caller's own memory. A **non-blittable struct is copied into a temporary**, your function fills in the temporary, and whether the values make it back depends on marshalling attributes the C# author has to get right. One `bool` in an options struct is enough to move you from the first case to the second.

So: fixed-width integers only, from `<stdint.h>`. `int32_t`, never `int`; `int32_t` for a flag, never `bool`. It looks pedantic in a header a C++ caller would also use, and it is the difference between a struct that cannot be misdeclared and one that can.

### The size field stops being politeness

Chapter 30 introduced a leading `size` field as a *versioning* device — a way for version two to append fields and still serve old callers. Across a P/Invoke boundary it acquires a second job, and the second one is more urgent.

```cpp
typedef struct {
    uint32_t size;        // caller sets this to sizeof(PluginOptions)
    int32_t  gain;
    int32_t  channels;
} PluginOptions;
```

The managed side declares that again by hand, and must say `[StructLayout(LayoutKind.Sequential)]` when it does — without it the runtime is free to order the fields as it pleases, which for a managed type is a legitimate optimisation and for this one is a bug. Sequential layout, matching pack, matching field order, matching field *widths*. Four things to transcribe, and the compiler on neither side is watching.

Here is what one missed field costs, from `exercises/interoplab/`. A caller that read an older header declares the struct without `size`, so it hands over eight bytes where the surface expects twelve:

```text
ERROR: AddressSanitizer: stack-buffer-overflow
READ of size 4 at 0x00016d1a2188
    #0 Plugin_Create plugin.cpp
```

Not a wrong `gain`. A read four bytes past the end of the caller's object, in the caller's frame — and across a real boundary the caller's frame belongs to a managed process where nobody is running a sanitizer at all. The check that turns that into a returned error code is one line:

```cpp
if (options->size != sizeof(PluginOptions)) return PLUGIN_VERSION_MISMATCH;
```

That is the field paying for itself twice: once for the version-two conversation Chapter 30 described, and once, today, for the transcription error you cannot prevent and can absolutely detect.

### Strings: three lengths and one contract

This is where the tickets come from, and the symptom is specific enough to recognise: text is fine for most customers and mojibake for the German and Russian ones.

The mechanism is [Chapter 9](09-casts-conversions-and-strings.md#chapter-9--casts-conversions-and-strings)'s, arriving at a boundary. A managed string is UTF-16. Your `char*` is bytes. Somebody converts, and the only question is whether both sides agree about to *what*. The historical default for `[DllImport]` is `CharSet.Ansi`, and "Ansi" there does not mean ASCII or UTF-8 — it means the system's active code page, which is why the bug sorts customers by language. Modern .NET spells the choice explicitly, `StringMarshalling.Utf8`, and the older `CharSet.Unicode` means UTF-16.

None of which is yours to fix. Yours is to remove the ambiguity, in the header, where the person transcribing will read it:

> **Encoding is part of the contract: this is UTF-8, always, on every platform.**

And then to notice that a string has three different lengths, all of them correct:

```text
"Zähler-µ"   →   8 characters
                 10 bytes in UTF-8      ← what your buffer must hold
                 8 UTF-16 units         ← what the managed side counts
```

A buffer sized from the wrong one of those three is the second-most-common interop bug after the layout mismatch. It is why the surface reports the size it needs rather than trusting anyone to compute it.

### Who frees it — and the answer that deletes the question

Chapter 30's rule was *whoever allocates must free*. On this boundary it has a sharper edge, because the managed side has several plausible-looking ways to free your memory and none of them is your allocator. `Marshal.FreeHGlobal` and `Marshal.FreeCoTaskMem` release from particular heaps; your `free()` releases from your C runtime's. Hand back a `malloc`'d string and the C# author will free it with something, confidently, and be wrong — with the corruption surfacing later, in an unrelated allocation, which is [Chapter 25](25-findings-from-practice.md#chapter-25--findings-from-practice-a-living-log)'s Finding 10 shape all over again.

Two shapes are safe. The second is better because it does not require anyone to be careful:

1. **You allocate, and you publish the matching `Free`.** Works, and now every consumer must remember to call it — the same contract `Thing_DisposeData` imposes in [Chapter 17](17-exercise-the-fakesdk.md#chapter-17--exercise-the-fakesdk), with a garbage-collected language on the other end that makes forgetting feel harmless.
2. **The caller allocates and you fill.** Called with a null buffer you report the size needed; called with a small one you say so and write nothing.

```cpp
PluginResult Plugin_GetName(PluginHandle h, char* buffer, size_t capacity,
                            size_t* needed);
```

Nothing you own ever crosses, so *which heap frees this* is not answered — it is never asked. On the managed side that call is a `byte[]` the GC already owns, and the interop layer becomes boring, which is the highest compliment an interop layer can be paid.

### Handles, and what `SafeHandle` is for

The opaque handle you have been publishing since Chapter 30 is exactly right here, and it has a counterpart on the managed side worth knowing about, because it changes what you should document.

`SafeHandle` is .NET's RAII for a native handle: a finalizable wrapper that closes the thing even if the process is shutting down, and — the part that matters to you — holds a reference count so the handle cannot be recycled while a call is still using it. It is the guard type of [Chapter 1](01-ownership-and-raii.md#chapter-1--ownership-and-raii), written by somebody else, for your handle.

Your job is to make it possible to use:

- **The handle is opaque and pointer-sized.** `IntPtr` on their side, and nothing they can dereference.
- **`Destroy` tolerates null and is idempotent.** A finaliser may run in circumstances you cannot predict, including twice if somebody is careless.
- **Say whether it is thread-safe**, because a `SafeHandle` protects the *handle*, not the object behind it. Chapter 16's fourth question, now asked of you.

### The delegate that was collected

[Chapter 22](22-exercise-lambda-lifetimes.md#chapter-22--exercise-lambda-lifetimes) taught that a lambda's captures can die before the lambda is called. The managed version of that lesson bites harder, and this is the one that produces crash reports nobody can reproduce.

When C# hands you a function pointer, what it actually creates is a small native thunk owned by a delegate object. That delegate is a managed object like any other. If nothing on the managed side keeps it rooted — and a delegate passed as an argument and then forgotten is not rooted — the collector is entitled to take it whenever it likes. Your stored pointer now aims at nothing, and it fires on the next event.

What makes it vicious is the asymmetry of the evidence. On the managed side there is no warning, no exception and nothing that looks wrong; the code that "leaked" is the code that did not keep a field. The crash is native, in your library, with your name on it.

You cannot fix that from here either. You can write down the one sentence that lets them fix it:

> **We hold this pointer from `Plugin_SetSink` until `Plugin_ClearSink` returns, and not one instruction longer.**

That is the entire contract, and it is what tells a managed author how long their field has to stay alive. It is the same sentence a device SDK owes you about its callback — Chapter 16's fourth question again, and [Chapter 29](29-concurrency.md#chapter-29--concurrency) if the answer involves a thread of yours.

> [!WARNING]
> **Trap:** A delegate marshalled as a function pointer is rooted only while something on the managed side holds it. Passing it straight into your registration call and keeping no field compiles, runs, works for a while, and then does not.

### In the wild: the other direction

Everything above assumes you own the process and the runtime is a guest you invited. The mirror image exists and arrives by a completely different route: a managed or interpreted runtime that already owns the process and has loaded *you* — a JNI library under a JVM, an extension module inside a Python interpreter, an addon in a Node process.

The material transfers almost intact. Layout, encoding, ownership and callback windows are the same four problems with the same four answers, because they are properties of the boundary rather than of who started the process. Two things do change, and they are worth naming so you know to go looking. **You do not choose the thread**, and the runtime may have rules about which of its APIs may be touched from where — a C++ exception reaching a JNI entry point takes the whole VM down, which is Chapter 30's nothing-escapes rule with the stakes raised again. And **you do not choose when you are unloaded**, so the static-teardown material in [Chapter 32](32-it-crashes-on-exit.md#chapter-32--it-crashes-on-exit) applies to a shutdown you did not schedule.

[Appendix G](G-the-bridge-catalogue.md#appendix-g--the-bridge-catalogue) prices the in-process family this belongs to, and [Chapter 38](38-the-bridge-out.md#chapter-38--the-bridge-out) builds the queue for the case where the foreign side must reach a host you do not own.

### Choosing what to publish

| Instead of | Publish | Because |
|---|---|---|
| `bool` | `int32_t` | the marshaller's default width for `bool` is not one byte |
| `int`, `long` | `int32_t`, `int64_t` | `long` is a different size on two of the three platforms you ship to |
| a returned `char*` | a caller-filled buffer plus a `needed` out-parameter | it deletes the which-heap question rather than answering it |
| an enum | `int32_t` plus documented constants | enum underlying type is a compiler decision |
| a struct by value with anything non-blittable in it | a blittable struct by pointer | one copy versus zero, and a marshalling attribute they must get right |
| "we call you back sometimes" | a written lifetime window | it is the only thing that lets them root the delegate correctly |

### Pitfalls

- **A `bool` in an exported struct.** It is one byte to you and, by default, four to the marshaller. Every field after it is then at the wrong offset, and the struct still compiles on both sides.
- **Omitting `[StructLayout(LayoutKind.Sequential)]` on the managed side.** You cannot enforce it, so put a `size` field in front and check it — the runtime's freedom to reorder is a fact about their type system, not a mistake you can forbid.
- **Returning a pointer to a `static` or a member buffer** to dodge the ownership question. It works until two threads call you, or until the second call overwrites what the first returned before the caller finished marshalling it.
- **Assuming a "string" is a length.** Three numbers, and interop bugs live in the gap between them.
- **Letting an exception reach an exported function.** Chapter 30 said this already; it is worse here, because the frame above yours is a runtime that will translate an unknown foreign failure into something unrecognisable, if it survives at all.
- **Testing the boundary only from C++.** A C++ caller shares your compiler, your `sizeof`, your enum widths and your calling convention, so it agrees with you about everything the managed side might get wrong. It is a necessary test and not a sufficient one.

> [!TIP]
> **Key principle:** "The managed declaration of my function is written by hand and checked by nothing — so I publish only blittable types, put a size field in front of every struct, and validate it on entry."

> [!TIP]
> **Key principle:** "Nothing I allocate crosses the boundary — the caller hands me a buffer and I tell them how big it needs to be, so which heap frees it is never asked."

> [!TIP]
> **Key principle:** "I write the callback's lifetime window into the header, because it is the only thing that tells a managed author how long to keep their delegate rooted."

### Try it

The finished surface is `exercises/interoplab/` — write your own first, in a directory of its own.

1. **Publish a blittable options struct** with a leading `size` field, and validate it in your create function. Then delete the validation, pass a struct declared one field short, and run it under `-fsanitize=address,undefined`. Predict before you run: a wrong value, or something louder? The answer is in the lab's task card, and it is worth being wrong about first.
2. **Add a string out-parameter** with the two-call protocol — null buffer for the size, then the real one. Assert all three lengths of a non-ASCII name and satisfy yourself they are all correct.
3. **Add a callback with a written window**, then prove the window: register, pump, clear, mark the target dead, pump again, and assert nothing arrived.
4. **Write the C# side on paper.** You cannot compile it here, and that is the exercise: transcribe your own header into `[LibraryImport]` declarations by hand, then read them back against the header looking for the four things nothing would have caught. Doing that once is what makes the rest of this chapter stick, because you will have been the person who gets it wrong.
5. **Hardest.** Add a second version of the options struct with an appended field, and make one build of the library serve both callers correctly, using the size field alone to tell them apart.

---

<!-- nav:begin -->
[← Chapter 38 — The Bridge Out](38-the-bridge-out.md) · [Contents](README.md) · [Appendix A — Fundamentals Refresher →](A-fundamentals-refresher.md)
<!-- nav:end -->
