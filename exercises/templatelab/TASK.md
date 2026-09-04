# Exercise: The Template Lab (one Session, two policies, and a build that must fail)

The working subset of templates a plug-in author writes: **Chapter 41** of
the book. ~2 h. Do it cold in a file of your own — `session.h`,
`policies.h`, `util.h` and `main.cpp` beside this card are the reference,
no peeking until yours runs.

*Trains: Chapter 41 — the template seam Chapter 28 named, `static_assert` as
the judge, the detection idiom, and reading an instantiation error from the
right end.*

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
   both policies; assert the samples, the double's counts, that a moved-from
   `Session` pumps nothing, and `FakeDevice_OpenHandles() == 0`. Then the
   compile-time claims: `static_assert` that `Session<...>` is not
   polymorphic and moves without throwing.
4. **Break a policy and read the error — twice.** Delete `Poll` from a
   policy and build. First surprise: if nothing calls `Pump()`, the build is
   *green* — a member of a class template is compiled only when used. Call
   `Pump()`, build again, and find the two lines that matter: the innermost
   failure (`no member named 'Poll'`) and the frame in *your* file (clang's
   last `requested here`, GCC's first `required from here`). Then write
   `HasSdkShape<T>` — the detection idiom, a `std::void_t` partial
   specialization over the calls — and a `static_assert` in `Session` that
   names the shape. Put the broken policy behind
   `#ifdef TEMPLATELAB_BROKEN_POLICY`, with a `Pump()` call, and build it:

   ```bash
   c++ -std=c++17 -Wall -Wextra -DTEMPLATELAB_BROKEN_POLICY -I ../fakedevice -c your.cpp
   ```

   The build must be refused with *your* sentence as the first `error:`
   line — whether or not anything calls `Pump()`.
5. **The three utilities.** `Describe(value)` with `if constexpr` (numbers
   through `to_string`, `char` as itself, strings through `string_view`,
   anything else a `static_assert` whose condition depends on `T`);
   `Join(sep, parts...)` with a comma fold; `Ring<T, N>` over a
   `std::array`. Assert each, including `Describe('A') == "A"` and
   `Join("-") == ""`.
6. **Stretch.** On a C++20 toolchain, replace the detection idiom with a
   `concept` and compare the error text for the broken policy: the concept
   names the exact expression that failed; the `static_assert` can only
   list what it asked for.

Build (from this directory):

```bash
../../scripts/check.sh your.cpp fakedevice
```

The judge is `scripts/build_all.sh`: it builds and runs `main.cpp` against
FakeDevice under the canonical flags, then builds it once more with
`-DTEMPLATELAB_BROKEN_POLICY` and asserts that the build is refused *and*
that the first error line is the `static_assert`'s own text — a judge that
only ever compiles things could not check the claim step 4 makes.
