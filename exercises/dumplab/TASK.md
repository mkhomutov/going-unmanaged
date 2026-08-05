# No Repro, Dump Attached — ticket card (Chapter 37)

This lab is a **ticket, not a task** — the sixth, and the first with
nothing to run: the program already ran, on a machine you will never see,
and died. What you have is the paperwork. Chapter 37 states everything in
full and walks the diagnosis behind a spoiler fold — work the ticket cold
first, and work it **on paper first**: the report, the source below, and
arithmetic are sufficient to name the guilty line.

> **#6117 — Crash at session close, field units only (3.4.0).** Three
> crash reports from two customer sites in ten days, all at session
> close, all on base-model field units. The bench cannot reproduce it —
> support has closed hundreds of sessions on every rig in the building,
> and every rig is green. No customer can share their machine; one
> shared the host's crash report, attached below. Our 3.4.0 binary is
> the shipped, stripped release build; the release job archived its
> symbol file next to the tag, as it always does.

**The files beside this card are the FIXED reference** — the lab's green
state, run by `build_all.sh` under both device configurations on every
push. Do not start from them. Recreate the broken 3.4.0 `session.cpp`
below in a scratch directory of your own, and work from there.

## The attachment — the host's crash report

Trimmed to the crashed thread and the two images that matter:

```text
Process:           HostShell [8412]
Identifier:        com.hostvendor.hostshell
Version:           5.3 (5310)

Exception Type:    EXC_BAD_ACCESS (SIGSEGV)
Exception Codes:   KERN_INVALID_ADDRESS at 0x0000000000000010

Thread 0 Crashed:: Main Thread
0   monitor.plugin      0x000000010bd0e654   0x10bd0e000 + 1620
1   monitor.plugin      0x000000010bd0e710   0x10bd0e000 + 1808
2   HostShell           0x0000000104a81f3c   Document::CloseSession() + 188
3   HostShell           0x0000000104a7c118   Document::Close() + 96
4   HostShell           0x00000001049f08a4   -[AppDelegate closeDocument:] + 72

Binary Images:
    0x10bd0e000 - 0x10bd0ffff  monitor.plugin  <7c3a9f2e-...>  ours, 3.4.0
    0x1049e8000 - 0x104ce3fff  HostShell       <91d47b0c-...>  the host
```

Note the asymmetry before anything else: the host's frames have names,
ours are `load address + offset`. That is not the reporter playing
favourites — it is what a stripped release binary looks like from the
outside, and it is why the release job archives the symbol file.

## The code as 3.4.0 shipped it

`session.h` and `main.cpp` — as beside this card. `session.cpp` is:

```cpp
#include "session.h"

Session::Session(bool calibrated) {
    if (calibrated) {
        cal_ = std::make_unique<Calibration>();
        cal_->scale  = 0.5;     // the capability's factory constants
        cal_->offset = 1.0;
    }
}

void Session::Ingest(double reading) {
    sum_ += reading;
    ++count_;
}

double Session::FoldedMean() const {
    const double mean = count_ > 0 ? sum_ / static_cast<double>(count_) : 0.0;
    cal_->folded += count_;                // bookkeeping the pack expects
    return mean * cal_->scale + cal_->offset;
}

double Session::Report() const {
    return FoldedMean();
}
```

## Work the ticket

1. **Read the exception line first — no source yet.** What kind of
   address is `0x10`? Chapter 3 named the pattern: it is not a random
   address, it is *an offset from somewhere*. From where?
2. **Do the arithmetic against `session.h`.** Two `double`s, then a
   `long`. Which member of which struct lives sixteen bytes in? Name the
   member the dead program was touching, from the address alone.
3. **Symbolicate frame 0 on paper.** `0x10bd0e654 − 0x10bd0e000 = 0x654`
   — an offset into *your* binary, answerable by *your* archived symbol
   file, no customer required. (You will do it for real in step 6.)
4. **Correlate the metadata.** All field units; every bench rig is green.
   What is different about a base-model device in this code? Which
   pointer does that difference control?
5. **Name the guilty function and the missing line.** Write both down —
   and predict which function the *naive* symbolication of frame 0 will
   name instead, and why.
6. **Only now, reproduce.** Recreate the broken `session.cpp` above next
   to the committed `session.h`/`main.cpp`, build the shipped shape, and
   die the customer's death:
   `g++ -std=c++17 -O2 -g session.cpp main.cpp -o monitor && ./monitor 0`
   Then hold the post-mortem your platform supports: run it under the
   debugger (`lldb -- ./monitor 0`, or `gdb --args`), read the stop
   reason and the backtrace, and compare against the report — same fault
   address, same physical frames. Symbolicate your own crash PC with
   `atos -i` (macOS), `addr2line -Cfie` (Linux), or the debugger itself,
   first *without* the inline flag, then with it. The difference is the
   chapter.
7. **The run the matrix never had.** Build the broken shape under the
   canonical flags and run `./monitor 0`: UBSan names the file, line and
   column on the spot. The tool was never the gap; the configuration
   matrix was.
8. **Fix it and prove it.** Both configurations green under the flags:
   `./monitor 1` and `./monitor 0`. One configuration cannot prove both
   — that is this lab's two-run claim, and `build_all.sh` holds it.
9. **Stretch: the institutional half.** `strip` a copy of your `-O2 -g`
   build, crash it, and symbolicate the stripped binary's addresses
   against the *unstripped original* — the whole dSYM/PDB discipline in
   one exercise: names are not in the corpse; they are in what you kept.
