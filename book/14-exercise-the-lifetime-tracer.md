# Part V — Learning by Doing

Work each exercise **cold**: compiler, debugger, sanitizer, and offline docs as your only feedback loops, opening a chapter's reference solution only after your own attempt. The repository's `exercises/` directory carries a task card for every exercise (plus the vendor code for Chapters 17 and 18), so you can attempt each one without the solution on the next screen.

---

## Chapter 14 — Exercise: The Lifetime Tracer

One small class makes every lifetime rule in this book visible: a **Tracer** that prints from every special member function it has — destructor, copy and move constructors, copy and move assignment — plus the ordinary constructor that names it, each stamped with an instance ID and its own address. Build it once, keep it forever — it is a diagnostic instrument, not a toy. When container or call behavior is mysterious, drop a Tracer in and the output replaces guesswork. It needs no tools a locked-down work machine lacks.

### The complete instrument

```cpp
--8<-- "solutions/tracer.cpp"
```

### Design choices worth stealing

- **Instance IDs separate identity from name.** Copies share a name; the `#id` makes each object unambiguous: `a#2 copy-CONSTRUCTED from a#1`.
- **Addresses turn claims into proofs.** Same address across "constructed" and "destroyed" lines = same object. Stack addresses vs heap addresses are visibly different ranges.
- **Moves steal in the initializer list** — `name_(std::move(other.name_))` — then log. Note the move constructor prints its *own* `name_`, because by the time the body runs the theft has already happened.
- **Husk marking** (`t.name_ = "husk"`) is tracing sugar: it makes the teardown roll-call show which objects ended life gutted. Real moved-from strings are simply left empty.
- **The sink-parameter constructor** — `explicit Tracer(std::string n) : name_(std::move(n))` — takes by value and moves in: one overload optimally handles both copies and moves (Chapter 6).
- **`inline static` counters** (C++17) — static members defined in-class, no separate .cpp definition needed; the modern fix for the Chapter 4 annoyance.
- **The alive counter is a leak detector**: every construction increments, every destruction decrements; a nonzero count at exit means RAII bookkeeping is broken somewhere.

### Annotated output of a real run

```text
--- singles ---
a#1 @0x7fff...d90  constructed
a#2 @0x7fff...dc0  copy-CONSTRUCTED from a#1 @0x7fff...d90
a#3 @0x7fff...df0  move-CONSTRUCTED, gutting #1 @0x7fff...d90
```

`b = a` created a new object (#2) at a new address. `std::move(a)` let #3 steal from #1 — note the gutted label prints an *empty name*: honestly stolen before printing. `a` (#1) still exists as a husk; it gets a normal destructor at the very end.

```text
--- calls ---
a#4 ...  copy-CONSTRUCTED from a#2 ...
a#4 ...  destroyed
a#5 ...  move-CONSTRUCTED, gutting #2 ...
a#5 ...  destroyed
```

`ByValue(b)`: a copy lives exactly for the call. **`ByRef(b)` printed nothing at all** — `const T&` binds directly, no object created; that silence is why it is the default parameter idiom. `ByValue(std::move(b))`: same function, but permission was granted, so the parameter *moved* — and b is now a husk.

```text
--- RVO ---
temp#6 @0x7fff...e20  constructed
```

That is the **entire** output of `Tracer d = MakeTracer()` on clang and GCC. One construction; no copy, no move. The compiler built "temp" directly in d's memory — the local, the return value, and d are one object (compare this address with d's destructor line at teardown: identical).

This is copy elision, and it is worth knowing which flavour, because not every build above produces it. `MakeTracer` returns a *named* local, which makes this **NRVO** — the optional kind. What C++17 made mandatory is narrower: elision is guaranteed only when the returned operand is a temporary outright, `return Tracer("temp");`. clang and GCC perform NRVO anyway, even at `-O0`. MSVC does not unless asked: `/Zc:nrvo` is off by default and switched on by `/O2`, `/permissive-`, or `/std:c++20` — none of which the `cl /std:c++17 /W4 /EHsc` line above passes. Build it that way and an extra move-construction appears here, and every `#id` after it shifts by one. (To see that output on any compiler, add `-fno-elide-constructors`.) Either way the cost is one move at worst, which is why returning objects by value is the right default in modern C++ — but "free" is a property of your compiler and flags, not a promise of the language.

```text
--- vector ---
v1#7 @0x7fff...e50  constructed          <- temporary, on the STACK
v1#8 @0x5622...2c0  move-CONSTRUCTED...  <- moved into the vector: HEAP address!
husk#7 ...  destroyed                    <- temporary husk dies
v2#9 ...   constructed
v2#10 @0x5622...398  move-CONSTRUCTED...
v1#11 @0x5622...370  move-CONSTRUCTED, gutting #8 @0x5622...2c0   <- REALLOCATION
husk#8 ...  destroyed
husk#9 ...  destroyed
```

The address ranges expose stack vs heap directly (Chapter 3). And the second `push_back` triggered **reallocation**: v1 was picked up and move-constructed *again* at a new heap address — you can watch the vector carry its contents to a bigger block. Delete `noexcept` from the move constructor and this line becomes `copy-CONSTRUCTED`: the vector falls back to copying when the move might throw (Chapter 6, Finding 3 of Chapter 25).

```text
---- 6 object(s) still alive, 11 ever created ----
--- teardown (reverse order per scope) ---
v1#11 destroyed   v2#10 destroyed        <- vector's elements first
temp#6 destroyed                          <- d (which IS temp: RVO)
a#3 destroyed
husk#2 destroyed  husk#1 destroyed        <- the gutted b and a, normal destruction
```

Eleven constructions, eleven destructions — balanced books, no leaks. Destruction runs in reverse construction order within each scope — a guarantee for the *named locals*; the order of the two element lines inside the vector is the standard library's business (libstdc++ destroys front-to-back as shown here, libc++ on a Mac back-to-front, and the standard blesses both). The husks in the roll-call are visual proof of which objects were genuinely emptied.

### Four experiments to run

1. **Delete `noexcept`** from the move constructor. The reallocation line flips from move to copy — vector's exception-safety rule, observed live.
2. **Add `v.reserve(4);`** before the push_backs. The entire reallocation block vanishes — no growth, no transfer.
3. **Add `Tracer x("x"); x = x;`** — self-copy-assignment. The copy assignment here has no self-check and survives only because `std::string::operator=` tolerates it. Ask yourself what happens when the member is a raw pointer: that question is the doorway to the Buffer worked example in Chapter 15.
4. **Make `a` const** — `const Tracer a("a");` — and watch the third line of the singles section. `Tracer c = std::move(a);` now prints `copy-CONSTRUCTED`, and `a` never becomes a husk: the cast asked for a steal, the const forbade it, and the copy constructor answered without a word from the compiler. Chapter 6's value-category table is the reason, and the trap it names second.
