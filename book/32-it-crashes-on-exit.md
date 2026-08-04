## Chapter 32 — It Crashes on Exit

Every exercise so far told you what it trains before you wrote a line. This
chapter — the first of the ticket-shaped ones — works the way the job does:
you get a symptom, the code it happened to, and no chapter title spoiling
the diagnosis. Work it cold, then open the fold; the thing it trains gets
its name only after you have felt the need for it, on the far side of the
diagnosis.

### The ticket

> **#4712 — Crash on exit (since 2.4.1).** The app crashes when it is
> closed. Not every install: support cannot reproduce it, and neither can
> the developer who shipped 2.4.1. The customer's crash report shows a
> segmentation fault *after* `exit`, in `__cxa_finalize` — after `main` has
> already returned. 2.4.1 changed one thing in this area: an audit line is
> now written when the session closes.

A crash after `main` returns is a category of its own. Your code has
finished; every line you wrote has executed and none of it is on the screen.
And yet something of yours is still running — which is the first thing the
diagnosis has to explain.

### The code it happened to

Four files. The logger is a session log — one heap block, appended to with
Recipe 5's `snprintf`, freed when the logger dies. It is a namespace-scope
global, the way loggers usually are:

```cpp
// logger.h (2.4.1)
#pragma once
#include <cstddef>

class Logger {
public:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void write(const char* line);
    std::size_t lines() const;

private:
    char* buffer_;          // the session log, one heap block
    std::size_t used_ = 0;
    std::size_t lines_ = 0;
};

extern Logger g_logger;    // one logger for the whole program
```

```cpp
// logger.cpp (2.4.1) - the implementation, and the global itself
Logger g_logger;

Logger::Logger() : buffer_(new char[kCapacity]) { buffer_[0] = '\0'; }
Logger::~Logger() { delete[] buffer_; }
```

And 2.4.1's one change: an auditor object, also a global, that writes the
closing line from its destructor —

```cpp
// audit.cpp (2.4.1)
#include "logger.h"

class Auditor {
public:
    ~Auditor() { g_logger.write("audit: session closed"); }    // added in 2.4.1
};

Auditor g_auditor;
```

`main` writes one line of ordinary work and exits. That is the whole
program. It compiles clean under `-Wall -Wextra`. It runs clean on the
developer's machine, and on every machine support tried.

### Try it — before reading on

The ticket card is `exercises/exitlab/TASK.md`, with the full broken
listings to recreate in a scratch directory of your own (the files beside
the card are the fixed reference — no peeking). The steps, and do them in
this order:

1. Build plain, run: exit 0. You have now reproduced support's experience.
2. Build under the handbook's flags with the sources in this order:
   `logger.cpp audit.cpp main.cpp`. Run it.
3. Build again with `audit.cpp logger.cpp main.cpp` — nothing changed but
   the order on the command line. Run that.
4. Read what you get the Chapter 31 way: which report shape, whose stacks,
   and what the frames *below* your own code are telling you.
5. Fix it so **both** link orders run clean. That is the acceptance test —
   not "it stopped crashing here".

### The diagnosis, walked through

<details>
<summary>Show the walkthrough — work the ticket cold first</summary>

Plain build: exit 0 in both link orders, on this machine (macOS/arm64). The
customer still crashes. Keep that pair of facts; the theory section owes you
an explanation for it.

Under `-fsanitize=address,undefined`, the first link order also runs clean.
The second — `audit.cpp logger.cpp main.cpp` — does not:

```text
ERROR: AddressSanitizer: heap-use-after-free ...
WRITE of size 21 ...
    #6 in Logger::write(char const*) logger.cpp:17
    #7 in Auditor::~Auditor() audit.cpp:5
    #9 in __cxa_finalize_ranges (libsystem_c.dylib)
    #10 in exit (libsystem_c.dylib)
freed by thread T0 here:
    #1 in Logger::~Logger() logger.cpp:11
    #3 in __cxa_finalize_ranges (libsystem_c.dylib)
previously allocated by thread T0 here:
    #1 in Logger::Logger() logger.cpp:10
```

Read it with Chapter 31's index. Three stacks — a use-after-free. The
*write* is `Logger::write`, called from `Auditor::~Auditor`. The *freed by*
is `Logger::~Logger`. And the tell this chapter adds to your index: both of
those stacks sit on `__cxa_finalize_ranges`, under `exit`. The write is a
destructor. The free is a destructor. **Two destructors, wrong order** — the
report is not describing a memory bug so much as a scheduling bug, and the
schedule is the exit sequence itself.

So: the logger died before the auditor, the auditor's last words went into
freed memory, and the only thing that decided who died first was the order
of two object files on a link line. Which nobody chose, and nobody tests.

</details>

### What the standard actually promises

You have spent years never once scheduling a global. In C#, a static
constructor runs before the type's first use — lazily, thread-safely, in an
order the runtime works out; teardown belongs to the runtime too, which is
exactly why the one thing every C# developer knows about finalizers is *do
not touch other objects from them*. The runtime made initialization order a
non-topic, and made destruction order somebody else's problem. C++ hands
you both.

> [!NOTE]
> **Surprise for C# devs:** there is no runtime sequencing your globals. Within one translation unit they construct top to bottom — and across translation units the order is **unspecified**: the standard refuses to say, and in practice the link line decides.

The rules, in full:

- Every object with static storage duration is **zero-initialized** first,
  before any code runs (Chapter 3's zero-init, doing quiet work here).
- Then **dynamic initialization** — the constructors — runs. Within a
  translation unit: top to bottom, in declaration order. Across translation
  units: no order at all. Whatever your linker did, that is the order, and
  relinking can change it. This is the **static initialization order
  fiasco**, and it has carried that name for decades.
- Destruction is the exact **reverse of construction**. Which sounds like a
  guarantee, and within one TU it is — but across TUs it inherits the same
  coin flip. Whoever constructed *last* dies *first*.

Now the ticket assembles itself. `~Auditor` needs `g_logger` alive, so the
logger must die later, so the logger must have constructed *earlier* — and
nothing anywhere enforces that. One link order satisfies it by luck; the
other builds a program that writes 21 bytes into freed memory every time it
exits. And the plain build's silence is Chapter 3's lesson wearing a new
coat: the freed block is usually still mapped, so the write lands quietly —
here. On the customer's machine, with the customer's allocator, it is a
segmentation fault in `__cxa_finalize`. Undefined behavior does not owe you
the same symptom twice, and it certainly does not owe you one on the
machine you debug on. Chapter 27's diamond was decided by link order too —
this is the same silent coin, flipped at exit.

### The fix

The global moves behind a function, and the function's local static *is*
the logger:

```cpp
Logger& TheLogger() {
    static Logger logger;    // constructed the first time anyone asks
    return logger;
}
```

`logger.h` now ends by declaring the accessor instead of the object:

```cpp
Logger& TheLogger();    // construct on first use - the fix
```

A function-local static constructs the first time control passes over it —
lazily, like the C# static constructor you never had to think about, and
since C++11 thread-safely too. It cannot be used before it exists, *by
construction*: the only way to reach it is through the function that
creates it. That kills the startup half of the fiasco outright. Chapter
28's test registry already used this shape — its comment "a namespace-scope
vector would be a bet on initialization order" was this chapter, in eight
words.

The exit half needs one more move, and it is the load-bearing one:

```cpp
#include "logger.h"

class Auditor {
public:
    Auditor()  { TheLogger().write("audit: session opened"); }    // pins the order
    ~Auditor() { TheLogger().write("audit: session closed"); }
};

Auditor g_auditor;
```

The constructor's call is not logging for logging's sake. First use
constructs the logger, so the logger finishes constructing *before*
`g_auditor` does — and reverse-order destruction, the same rule that broke
the program, now guarantees the logger outlives the auditor. **Touch your
dependencies in your constructor**: it turns destruction order from a bet
into a consequence.

The acceptance test is the one from the ticket card: both link orders,
clean. This repository runs exactly that on every push —
`build_all.sh` builds `exercises/exitlab/` twice with the translation units
reversed and runs both, because the fix's whole claim is that the order
stopped mattering, and one build cannot prove a claim about two.

### The same fiasco runs forwards

Make the auditor's *constructor* log too — realistic; 2.4.2 would have —
and the bad link order now fails before `main`: the auditor constructs
first, and `g_logger.write` runs on a logger that is zero-initialized but
not yet constructed. `buffer_` is null (Chapter 3 again — zero-init is why
this is a null write and not garbage), and the program crashes at startup.
AddressSanitizer has a detector for exactly this, with the fiasco's name on
it:

```text
$ ASAN_OPTIONS=check_initialization_order=1:strict_init_order=1 ./app
ERROR: AddressSanitizer: initialization-order-fiasco ...
SUMMARY: ... logger.cpp:14 in Logger::write(char const*)
```

One habit covers both directions: a crash before `main` or after it puts
namespace-scope constructors at the top of the suspect list. `grep` for
globals with constructors; the list is usually short and one of them is
usually your bug.

### Pitfalls

- **A clean sanitizer run proved less than you think — again.** The first
  draft of this lab used `std::vector<std::string>` as the log. The exit
  write went into freed memory and ASan said *nothing*: libc++'s container
  annotations un-poison the slot on `push_back` before constructing into it
  (observed on macOS/AppleClang — the freed block reads as poisoned before
  the call and clean after). Finding 10 of Chapter 25 keeps collecting
  examples.
- **The local static alone is half a fix.** Without the constructor touch,
  the logger's construction order — and so its destruction slot — is still
  set by whoever happens to call first. Legal, and back to gambling.
- **The immortal variant.** For something written to by *everyone's*
  destructors, the honest shape is `static Logger* logger = new Logger;` —
  constructed on first use, never destroyed, so there is no destruction
  slot to lose. The block stays reachable, so LeakSanitizer's default stays
  quiet about it. Deliberate immortality is a tool; accidental immortality
  is a leak — write the comment.
- **Plug-ins pay double.** A shared library's globals construct at load and
  destroy at unload, on the host's schedule (Chapter 30's boundary rules
  exist partly for this). If your plug-in needs setup and teardown, want
  them to be *entry points the host calls* — the `X_Init`/`X_DeInit` pair
  of Chapter 16's Bestiary — not side effects of the loader.

> [!TIP]
> **Key principle:** "A namespace-scope object with a constructor is a bet on link order. I construct on first use, and I touch my dependencies in my constructor — so teardown unwinds in the order I chose, not the order the linker did."

### In the wild

The fiasco is old enough that the ecosystem is full of armour against it,
once you know what you are looking at. `std::cout` is safe to use from
another TU's constructor only because `<iostream>` plants a small counter
object in every file that includes it, forcing the stream's setup ahead of
yours. Qt ships `Q_GLOBAL_STATIC` — construct-on-first-use as a macro. And
Chapter 16's Shape 4 warned that embedded HALs treat some resources as
"singletons whose ownership *is* initialization order" — the vendors'
`X_Init`/`X_DeInit` pairs are this chapter's lesson, shipped as API:
initialization order made explicit, because the implicit kind cannot be
trusted. When you author your own boundary (Chapter 30), do the same.

<!-- nav:begin -->
[← Chapter 31 — Reading What the Tools Tell You](31-reading-what-the-tools-tell-you.md) · [Contents](README.md) · [Chapter 33 — Here Is the Report →](33-here-is-the-report.md)
<!-- nav:end -->
