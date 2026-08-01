## Chapter 8 — Error Handling: Exceptions and Error Codes

C# is exceptions everywhere. C++ is split-brained: exceptions exist, but large parts of the ecosystem — most C-flavored SDKs, OS APIs, and plug-in interfaces — use C-style **error codes**. You need both.

```cpp
// C++ exceptions - familiar, with differences:
try {
    auto w = LoadWidget(path);
} catch (const std::exception& e) {   // catch by CONST REFERENCE, always
    Log(e.what());                    // (by value would slice - Chapter 2!)
    throw;                            // rethrow: plain 'throw', like C#
}
// NO 'finally' in C++. RAII *is* the finally:
// cleanup lives in destructors, which run during stack unwinding.
```

Standard exceptions derive from `std::exception` (`std::runtime_error`, `std::logic_error`, `std::out_of_range`...). Throw by value, catch by const reference — catching by value slices derived exceptions.

The error-code world you'll actually live in:

```cpp
ErrCode err = Thing_GetData(index, &data);      // Chapter 17's SDK idiom
if (err != NoErr)
    return err;                  // check EVERY call. No exception will save you.

err = Thing_SumValues(&data, &sum);
if (err != NoErr)
    return err;
// The tedium is real. Mitigations: early returns (not nested ifs),
// RAII guards so early returns can't leak, small helper functions.
```

Two rules to hold: **(1)** exceptions must never cross a DLL/plug-in boundary into a host application or a C API — catch everything at your entry points and convert to error codes; the code on the other side is not prepared for your exceptions. **(2)** destructors must never throw — one is already running during unwinding; a second exception terminates the program.

Exception-safety guarantees (worth knowing cold): **basic** — no leaks, object in some valid state; **strong** — operation succeeds or has no effect (copy-and-swap from Chapter 6 delivers this); **noexcept** — cannot throw. If a constructor throws, the object never existed — its destructor does NOT run, but already-constructed members ARE destroyed. That is why acquiring resources through RAII members is safe and raw acquisition in ctor bodies is not.

> [!IMPORTANT]
> **Key principle:** "I check every error code, use RAII so early returns can't leak, and never let an exception escape the plug-in boundary — I catch at entry points and translate to the SDK's error codes."

---


<!-- nav:begin -->
[← Chapter 7 — Templates vs C# Generics](07-templates-vs-csharp-generics.md) · [Contents](README.md) · [Chapter 9 — Casts, Conversions, and Strings →](09-casts-conversions-and-strings.md)
<!-- nav:end -->
