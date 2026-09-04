# The Template Lab — task card

The working subset of templates a plug-in author writes: **Chapter 41** of
the book. ~2 h. Do it cold; `session.h`, `policies.h`, `util.h` and
`main.cpp` beside this card are the reference — no peeking until yours runs.

The lab links `../fakedevice/`'s vendor code (Chapter 18) rather than
copying it: one of the two policies is the real device.

## Tasks

1. **The seam as a template parameter.** Write `Session<Sdk>`: a class that
   opens a handle in its constructor through `Sdk::Open`, closes it in the
   destructor through `Sdk::Close`, is move-only (Chapter 6 — it owns a
   handle), and has a `Pump()` that asks `Sdk::Poll` for samples and keeps
   them. The policy is a type with a `Handle` typedef and three static
   functions; `Session` names it and nothing else.
2. **Two policies.** `FakeDeviceSdk` over `FakeDevice.h` — the trampoline
   and its `void*` now live inside the policy's `Poll` — and `RecordingSdk`,
   a double with a scripted table of samples and two counters, opens and
   closes. Compile the same `Session` against both.
3. **Judge it.** Under the canonical flags: open, pump, move, close against
   both policies; assert the samples, the double's counts, and
   `FakeDevice_OpenHandles() == 0`. Then the compile-time claims:
   `static_assert` that `Session<...>` is not polymorphic and moves without
   throwing.
4. **Break a policy and read the novel.** Delete `Poll` from a policy and
   build. Read the error from the *bottom*: the last `requested here` is
   your line. Then write `HasSdkShape<T>` — the detection idiom, a
   `std::void_t` partial specialization over the three calls — and a
   `static_assert` in `Session` that names the missing function. Build the
   broken policy again behind `#ifdef TEMPLATELAB_BROKEN_POLICY`: the build
   must be refused with *your* sentence as the first error.
5. **The three utilities.** `Describe(value)` with `if constexpr` (numbers
   through `to_string`, strings through `string_view`, anything else a
   `static_assert`); `Join(sep, parts...)` with a comma fold; `Ring<T, N>`
   over a `std::array`. Assert each, including `Join("-") == ""`.
6. **Stretch.** On a C++20 toolchain, replace the detection idiom with a
   `concept` and compare the error text for the broken policy.

Build (from this directory):

```bash
../../scripts/check.sh main.cpp fakedevice
```

The judge is `scripts/build_all.sh`: it builds and runs `main.cpp` against
FakeDevice under the canonical flags, then builds it once more with
`-DTEMPLATELAB_BROKEN_POLICY` and asserts that the build is refused *and*
that the first error line is the `static_assert`'s own text — a judge that
only ever compiles things could not check the claim step 4 makes.
