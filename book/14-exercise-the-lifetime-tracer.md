# Part V — Learning by Doing

Before starting, skim [Chapter 24](24-practice-plan.md#chapter-24--practice-plan) — the practice plan — which sequences these chapters into a one-week schedule. Then work each exercise **cold**: compiler, debugger, sanitizer, and offline docs as your only feedback loops, opening a chapter's reference solution only after your own attempt. The repository's `exercises/` directory carries a task card for every exercise (plus the vendor code for Chapters 17 and 18), so you can attempt each one without the solution on the next screen.

---

## Chapter 14 — Exercise: The Lifetime Tracer

One small class makes every lifetime rule in this book visible: a **Tracer** that prints from all six special member functions, stamped with an instance ID and its own address. Build it once, keep it forever — it is a diagnostic instrument, not a toy. When container or call behavior is mysterious, drop a Tracer in and the output replaces guesswork. It needs no tools a locked-down work machine lacks.

### The complete instrument

```cpp
// Tracer v2 - makes object identity, lifetime, and moves maximally visible.
// Build:  clang++ -std=c++17 -Wall -Wextra tracer.cpp -o tracer
//    or:  cl /std:c++17 /W4 /EHsc tracer.cpp

#include <iostream>
#include <string>
#include <utility>
#include <vector>

class Tracer {
public:
    explicit Tracer(std::string n)
        : name_(std::move(n)), id_(++counter_)
    {
        ++alive_;
        Log("constructed");
    }

    ~Tracer() {
        --alive_;
        Log("destroyed");
    }

    // ---- copy: duplicates, source untouched -------------------------------
    Tracer(const Tracer& other)
        : name_(other.name_), id_(++counter_)
    {
        ++alive_;
        Log("copy-CONSTRUCTED from " + other.Label());
    }

    Tracer& operator=(const Tracer& other) {
        // std::string::operator= releases our old buffer internally.
        // With a raw resource, WE would have to release it here. (Finding 4)
        name_ = other.name_;
        Log("copy-ASSIGNED from " + other.Label());
        return *this;
    }

    // ---- move: steals, source becomes a husk ------------------------------
    Tracer(Tracer&& other) noexcept
        : name_(std::move(other.name_)), id_(++counter_)   // steal FIRST (Finding 1)
    {
        ++alive_;
        Log("move-CONSTRUCTED, gutting " + other.MarkHusk());
    }

    Tracer& operator=(Tracer&& other) noexcept {
        if (this != &other) {                              // self-move guard (Finding 5)
            name_ = std::move(other.name_);                // steal
            Log("move-ASSIGNED, gutting " + other.MarkHusk());
        }
        return *this;
    }

    // ---- bookkeeping ------------------------------------------------------
    static void Report() {
        std::cout << "---- " << alive_ << " object(s) still alive, "
                  << counter_ << " ever created ----\n";
    }

private:
    // "name#id @address" - id distinguishes objects sharing a name;
    // address proves identity (RVO: 'temp' and 'd' print the SAME address).
    std::string Label() const {
        return name_ + "#" + std::to_string(id_) + " @" + Addr();
    }

    std::string Addr() const {
        char buf[32];
        std::snprintf(buf, sizeof buf, "%p", static_cast<const void*>(this));
        return buf;
    }

    std::string MarkHusk() {
        std::string was = Label();      // capture identity BEFORE overwriting
        name_ = "husk";                 // tracing sugar only - real moved-from
        return was;                     // strings are simply left empty
    }

    void Log(const std::string& what) const {
        std::cout << Label() << "  " << what << '\n';
    }

    std::string name_;
    int id_;
    inline static int counter_ = 0;     // total ever created
    inline static int alive_   = 0;     // live right now (leak detector)
};

// ---------------------------------------------------------------------------

Tracer MakeTracer() {
    Tracer t("temp");
    return t;                           // watch: NO copy, NO move (RVO)
}

void ByValue(Tracer t) { (void)t; }
void ByRef(const Tracer& t) { (void)t; }

int main() {
    std::cout << "--- singles ---\n";
    Tracer a("a");
    Tracer b = a;                       // copy: b is a NEW object (new id)
    Tracer c = std::move(a);            // move: a becomes a husk

    std::cout << "--- calls ---\n";
    ByValue(b);                         // copy in, destroyed at return
    ByRef(b);                           // SILENCE - binds directly, no object
    ByValue(std::move(b));              // move in - b gutted

    std::cout << "--- RVO ---\n";
    Tracer d = MakeTracer();            // one construction; compare the
                                        // address with d's destructor line!
    std::cout << "--- vector ---\n";
    std::vector<Tracer> v;
    v.push_back(Tracer("v1"));          // temp constructed, moved in, temp dies
    v.push_back(Tracer("v2"));          // + REALLOCATION: v1 moves to new block
                                        // (delete noexcept above -> it COPIES)
    Tracer::Report();                   // everything from main still alive

    std::cout << "--- teardown (reverse order per scope) ---\n";
    return 0;
}   // vector first (its elements), then d, c, b, a

// After main, statics persist; a final Report() via atexit would show 0 alive.
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

That is the **entire** output of `Tracer d = MakeTracer()`. One construction; no copy, no move. The compiler built "temp" directly in d's memory — the local, the return value, and d are one object (compare this address with d's destructor line at teardown: identical). This is RVO/copy elision, and it is why returning objects by value in modern C++ is free.

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

Eleven constructions, eleven destructions — balanced books, no leaks. Destruction runs in reverse construction order within each scope. The husks in the roll-call are visual proof of which objects were genuinely emptied.

### Three experiments to run

1. **Delete `noexcept`** from the move constructor. The reallocation line flips from move to copy — vector's exception-safety rule, observed live.
2. **Add `v.reserve(4);`** before the push_backs. The entire reallocation block vanishes — no growth, no transfer.
3. **Add `Tracer x("x"); x = x;`** — self-copy-assignment. The copy assignment here has no self-check and survives only because `std::string::operator=` tolerates it. Ask yourself what happens when the member is a raw pointer: that question is the doorway to the Buffer worked example in Chapter 15.

---

