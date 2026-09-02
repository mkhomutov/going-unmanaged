# Exercise: The Interop Lab

Publishing a C surface for a managed caller to P/Invoke: **Chapter 39** of the
book. ~2 h.

You have already built a C boundary in Chapter 30. This one adds the four
things that only start to matter when the caller is a *runtime* rather than a
compiler — because the declaration on the other side is written **by hand, in
another language, and nothing checks that the two agree.**

## What is here

`plugin.h` is the surface; `plugin.cpp` is behind it; `marshal.h` stands in
for the marshaller; `main.cpp` is the judge. Write your own surface first —
the files here are the answer.

**`marshal.h` is not a CLR.** It models what the marshaller does to a struct,
a string and a delegate, the same way `FakeSDK` models a vendor. That is
enough, because every mistake this chapter is about is observable from the
native side. What it deliberately does not model is the garbage collector:
a collected delegate is a use-after-free, and this repository does not commit
programs whose bug is the point. The sink carries an `alive` flag instead, so
the harness can *assert* the documented window is honoured.

## The task

Write a C surface a managed caller could bind, and make each of these true.

1. **A blittable options struct with a leading size field.** Fixed-width
   integers only — no `bool`, no `char`, nothing whose size the two sides
   might disagree about. The caller sets `size = sizeof(Options)`; you check
   it and refuse a mismatch.
2. **A string out-parameter the caller allocates.** Called with a null
   buffer it reports the required size; called with a small one it says so
   rather than writing. Nothing you allocated ever crosses, so *which heap
   frees this* never has to be answered.
3. **Name the encoding in the header.** UTF-8, on every platform, stated
   where the caller will read it. "The platform's `char`" is not an
   agreement.
4. **Write down the callback window.** One sentence saying exactly how long
   you hold the pointer. The managed author cannot keep their delegate
   rooted for the right span unless you tell them what that span is.

## The judge

`scripts/build_all.sh` builds and runs it. Two of its five assertions are
worth stealing:

- **A deliberately misdeclared struct is passed in**, exactly as a caller who
  read an older header would produce, and the surface must return
  `PLUGIN_VERSION_MISMATCH` rather than a plausible wrong answer.
- **The sink must not be called after `Plugin_ClearSink` returns**, which is
  the header's promise asserted rather than trusted.

Run your own attempt with:

```bash
scripts/check.sh plugin.cpp main.cpp
```

## Try this before reading the fix

Delete the `options->size != sizeof(PluginOptions)` check and run it again.
Predict first: a wrong gain, or something louder?

<details>
<summary>What actually happens — try it before opening</summary>

`AddressSanitizer: stack-buffer-overflow`. The misdeclared struct is eight
bytes and the surface reads twelve, so `channels` is read from four bytes
past the end of the caller's object. The size field is not politeness; it is
the difference between a clear result code and a buffer overrun in a frame
you do not own — and across a real P/Invoke boundary, nobody is running a
sanitizer on the managed caller's stack.

</details>
