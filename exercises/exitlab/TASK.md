# The Exit Crash — ticket card (Chapter 32)

This lab is a **ticket, not a task**: you get a symptom and the code it
happened to, and the job is the diagnosis. Chapter 32 states everything in
full and walks the diagnosis behind a spoiler fold — work the ticket cold
first.

> **#4712 — Crash on exit (since 2.4.1).** The app crashes when it is
> closed. Not every install: support cannot reproduce it, and neither can
> the developer who shipped 2.4.1. The customer's crash report shows a
> segmentation fault *after* `exit`, in `__cxa_finalize` — after `main` has
> already returned. 2.4.1 changed one thing in this area: an audit line is
> now written when the session closes.

**The files beside this card are the FIXED reference** — the lab's green
state, built by `build_all.sh` in two different link orders on every push.
Do not start from them. Recreate the broken 2.4.1 code below in a scratch
directory of your own, and work from there.

## The code as 2.4.1 shipped it

`logger.h` — as below, except the last line reads:

```cpp
extern Logger g_logger;    // one logger for the whole program
```

(and there is no `TheLogger` declaration). `logger.cpp` defines the global
at namespace scope — `Logger g_logger;` — and has no `TheLogger()` function.
`audit.cpp` is:

```cpp
#include "logger.h"

class Auditor {
public:
    ~Auditor() { g_logger.write("audit: session closed"); }    // added in 2.4.1
};

Auditor g_auditor;
```

`main.cpp` calls `g_logger.write("main: doing the day's work")` and prints
`g_logger.lines()`. Everything else is identical to the files beside this
card.

## Work the ticket

1. **Reproduce what support saw.** Build the broken version plain
   (`g++ -std=c++17 -Wall -Wextra -g`) and run it. It exits 0. So did every
   machine support tried.
2. **Now the handbook's flags — twice.** The one-line judge builds the
   sources in the order you write them, and that order is the link order:
   `../../scripts/check.sh logger.cpp audit.cpp main.cpp`. Then again as
   `../../scripts/check.sh audit.cpp logger.cpp main.cpp` — only the order
   changed.
3. **Read the report, Chapter 31 style.** Which shape is it? Whose stacks
   are the *write*, the *freed by*, the *allocated by* — and what do the
   frames below your own code say about *when* this is happening?
4. **Fix it so that BOTH link orders run clean.** That is the acceptance
   test — not "it stopped crashing on my machine".
5. **Stretch:** make the Auditor's *constructor* log too, rebuild broken in
   the bad order, and run with
   `ASAN_OPTIONS=check_initialization_order=1:strict_init_order=1`. The
   fiasco has a first act, and the sanitizer knows its name.
