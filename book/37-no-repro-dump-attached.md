## Chapter 37 — No Repro, Dump Attached

The sixth ticket, and the first with nothing to run. Every previous ticket let you reproduce: build the broken shape, watch it fail, interrogate it alive. This one already ran — on a machine you will never see, under a configuration you will have to deduce — and died. What arrives is the corpse's paperwork: a crash report, a core file, a minidump, depending on the platform, and they are all the same document wearing different clothes — a fault, a stack of raw addresses, and a list of what was loaded where. SDK work receives this artifact constantly, because your code runs inside other people's processes on other people's machines. [Chapter 31](31-reading-what-the-tools-tell-you.md#chapter-31--reading-what-the-tools-tell-you) taught you reports a sanitizer wrote *for* you, with names and line numbers pre-attached; this chapter's report attaches nothing — the names are your job, and they live in a file that had better exist. Same rule as ever: **no compiler until your diagnosis is written down.** The report, the source, and arithmetic are sufficient.

### The ticket

> **#6117 — Crash at session close, field units only (3.4.0).** Three
> crash reports from two customer sites in ten days, all at session
> close, all on base-model field units. The bench cannot reproduce it —
> support has closed hundreds of sessions on every rig in the building,
> and every rig is green. No customer can share their machine; one
> shared the host's crash report, attached below. Our 3.4.0 binary is
> the shipped, stripped release build; the release job archived its
> symbol file next to the tag, as it always does.

That last clause is doing more work than the whole rest of the ticket. Hold it.

### The attachment — the host's crash report

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

Read the asymmetry before anything else: the host's frames have names, ours are `load address + offset`. The reporter is not playing favourites — this is what a stripped release binary looks like from the outside. A compiled function keeps no memory of what it was called; the name lives in the symbol information, and a shipped binary carries as little of that as its build allows. The `Binary Images` section is the decoder ring the report *can* give you: where each module happened to be loaded this time, so that an absolute address can be turned back into a stable offset into a file you possess.

The report on your own platform will dress differently — a Linux core file opened with `gdb` or `lldb`, a Windows minidump opened with WinDbg, this text file from a Mac — but the three organs are always these: the exception, the thread's addresses, the module table. The reading below transfers whole.

### The code it happened to

Reduced to the plug-in's session accounting — the two files frame 0 will turn out to live in, plus the driver. The device's optional *calibration pack* exists only on units that report the capability; `session.h` says so in the place every reader passes:

```cpp
#pragma once
#include <memory>

// The optional calibration pack: present only when the device reports the
// capability. THE INVARIANT: cal_ may be null for an entire session, and
// every path that touches it owns the check.
struct Calibration {
    double scale  = 1.0;
    double offset = 0.0;
    long   folded = 0;     // readings folded through this pack so far
};

class Session {
public:
    explicit Session(bool calibrated);

    void Ingest(double reading);
    double Report() const;                 // session close: the mean, calibrated
    bool Calibrated() const { return cal_ != nullptr; }

private:
    double FoldedMean() const;             // applies the pack to the mean

    double sum_   = 0.0;
    long   count_ = 0;
    std::unique_ptr<Calibration> cal_;     // null when the capability is absent
};
```

And `session.cpp` as 3.4.0 shipped it:

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

It compiles clean under `-Wall -Wextra`. Every bench rig runs it green, plain or sanitized, hundreds of sessions a day.

### Try it — before reading on

The ticket card is `exercises/dumplab/TASK.md`, with the report and the broken listing together (the files beside the card are the fixed reference — no peeking). Pencil before compiler:

1. **Read the exception line first — no source yet.** What kind of address is `0x10`? [Chapter 3](03-stack-heap-and-undefined-behavior.md#chapter-3--stack-heap-and-undefined-behavior) named the pattern: it is not a random address, it is an *offset from somewhere*. From where?
2. **Do the arithmetic against `session.h`.** Two `double`s, then a `long`. Which member of which struct lives sixteen bytes in? Name what the dead program was touching, from the address alone.
3. **Symbolicate frame 0 on paper.** `0x10bd0e654 − 0x10bd0e000 = 0x654` — an offset into *your* binary, answerable by *your* archived symbol file, no customer required.
4. **Correlate the metadata.** All field units; every bench rig green. What is different about a base-model device *in this code*, and which pointer does that difference control?
5. **Name the guilty function and the missing line** — and predict which function the *naive* symbolication of frame 0 will name instead, and why.
6. **Only now, reproduce** — the card's steps 6–9: die the customer's death at `-O2`, hold the post-mortem with your platform's debugger, symbolicate your own crash PC with and without inline expansion, run the configuration the matrix never had, then fix and prove both configurations.

### The diagnosis, walked through

<details>
<summary>Show the walkthrough — write your own diagnosis down first</summary>

`KERN_INVALID_ADDRESS at 0x0000000000000010`. Sixteen. A program with a heap in the terabyte range does not stumble onto address sixteen by chance; an address that small is almost always **null plus an offset** — some `p->member` where `p` was null and `member` sits that far into its struct. Which struct? The report cannot say, but the source can: `Calibration` is two `double`s and then `long folded` — offsets 0, 8, **16**. The dead program was touching `cal_->folded` through a null `cal_`. One line in the plug-in does that. You have the guilty line before symbolicating a single frame — this is [Chapter 33](33-here-is-the-report.md#chapter-33--here-is-the-report)'s region arithmetic with the sign flipped: there an offset *into a block* named the victim; here an offset *from nothing* names the member.

Now make the stack confess anyway, because next time the struct will not have a unique member at that offset. Frame 0 is `0x10bd0e654`, and the Binary Images table says our module loaded at `0x10bd0e000`: offset `0x654`. Feed that to the symbol file the release job archived — `atos` on a Mac, `addr2line` or `llvm-symbolizer` on Linux, the debugger with the PDB on Windows — and the naive answer comes back: **`Session::Report() const, session.cpp:23`**. Go look at line 23. It reads `return FoldedMean();`. It dereferences nothing. It *cannot fault*.

This is the moment post-mortem work is about, so slow down on it. The line is not lying and the tool is not broken: at `-O2` the compiler **inlined** `FoldedMean` into `Report`, so the faulting instruction physically lives inside `Report`'s code range, and an address-to-symbol lookup dutifully says so. The frame that did it does not exist on the stack — it was compiled out of existence, [Chapter 31](31-reading-what-the-tools-tell-you.md#chapter-31--reading-what-the-tools-tell-you)'s `-O2` lesson escalated from *missing frames in a trace* to *the culprit has no frame at all*. The debug information remembers what the stack cannot: ask with inline expansion — `atos -i`, `addr2line -i`, or a debugger, which shows it as a bracketed extra frame — and the same address answers twice: `FoldedMean() const, session.cpp:18` **inlined into** `Report() const, session.cpp:23`. Line 18 is `cal_->folded += count_;`. The arithmetic and the symbols now agree.

On this machine, reproducing the shipped shape (`-O2 -g`, the bare configuration) and holding the post-mortem in `lldb` prints the whole story in three lines:

```text
* thread #1, stop reason = EXC_BAD_ACCESS (code=1, address=0x10)
    frame #0: 0x0000000100000654 monitor`Session::FoldedMean(this=0x...) const at session.cpp:18:18 [opt] [inlined]
    frame #1: 0x0000000100000630 monitor`Session::Report(this=0x...) const at session.cpp:23:12 [opt]
    frame #2: 0x0000000100000710 monitor`main at main.cpp:15:29 [opt]
```

Same fault address as the customer's. Frames #0 and #1 are the *same physical frame* — look at the program counters, `0x654` sits 36 bytes into the function that starts at `0x630` — pulled apart by the debug info's inline records. Strip the binary, crash it again, and the reconstruction disappears: two anonymous app frames, `+36` and done, which is exactly the customer's view. Names are not in the corpse. They are in what you kept.

Which leaves *why* `cal_` was null, and here the report reaches its honest limit: a stripped crash report carries no heap, no locals, no history — it cannot tell you why a pointer held what it held. The ticket's metadata can. All three crashes are base-model field units; `Session`'s constructor allocates the pack only `if (calibrated)`; the bench's rigs are the full-featured units support always gets. `FoldedMean` touches the pack without the check that `Ingest`'s author — the same header's own capital-letters INVARIANT — knew to state. The bench was never running this function's null branch, because the bench never *has* a null. "Cannot reproduce" was never a mystery: the repro instruction was printed in the report's metadata, the way [Chapter 33](33-here-is-the-report.md#chapter-33--here-is-the-report)'s workaround was.

And the run the matrix never had: build the broken shape under this book's canonical flags and run the bare configuration once. UBSan stops it on the spot — `session.cpp:18:11: runtime error: member access within null pointer of type 'Calibration'` — file, line, column, before the crash ever happens. The tools were never the gap. The configuration matrix was: every sanitized run the project ever did was a calibrated run. A tool can only judge the runs you give it — Finding 10 of [Chapter 25](25-findings-from-practice.md#chapter-25--findings-from-practice-a-living-log) has been collecting this family all book.

</details>

### What the contract actually says

Two names for the notes file. **Near-null fault address = member offset.** A crash at `0x10`, `0x18`, `0x40` is a null pointer plus `offsetof` arithmetic, and the struct definition converts it to a member name before any debugger opens — the cheapest diagnosis in this book, and the field artifact it decodes arrives weekly in SDK work. (Its cousins: a fault at exactly 0 is a null deref of the first member or the pointer itself; garbage-high addresses are wild pointers, not null ones — different chapter, different crime.)

**Symbols are yours to keep.** The shipped binary does not carry its names; the *symbol file* does — dSYM on Apple platforms, PDB on Windows, the unstripped build or split debug info on Linux — and it is only useful if it matches the customer's binary byte for byte, which means one archived per release, filed with the tag, forever. A crash report is a claim check against that archive. Teams that keep symbols read field crashes in minutes; teams that don't get a page of numbers and a shrug. And when you read them, symbolicate **with inline expansion** — the optimizer compiles frames out of existence, so the function a naive lookup names is merely the *host* of the instruction, not necessarily its author.

> [!NOTE]
> **Surprise for C# devs:** you have had this ticket before — and never noticed how much machinery softened it. A `NullReferenceException` hands you the exact throw site, a full stack with names, and it does so *on the customer's machine, in the release build, every time*, because the runtime carries its own metadata and checks every dereference. Here the same mistake is UB ([Chapter 3](03-stack-heap-and-undefined-behavior.md#chapter-3--stack-heap-and-undefined-behavior)): what you get is a hardware fault, an address, and a stack of numbers — and the name-restoring metadata is a separate artifact that exists only if your release process kept it. The diagnosis is entirely recoverable. It is just no longer free.

### The fix

One check, in the one function that skipped it — the fixed `session.cpp`:

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
    if (cal_ == nullptr) {                 // the field configuration: no pack,
        return mean;                       // so the mean ships uncalibrated
    }
    cal_->folded += count_;                // bookkeeping the pack expects
    return mean * cal_->scale + cal_->offset;
}

double Session::Report() const {
    return FoldedMean();
}
```

Worth saying plainly: this is the *minimal* fix, and the chapter chose it because it is what the ticket gets on the day. The design debt it leaves is the invariant itself — "cal_ may be null and every path owns the check" is a rule enforced by vigilance, and vigilance is what just failed. The structural cures both remove the null instead of guarding it: an always-present *identity pack* (`scale 1.0, offset 0.0` — calibration that changes nothing), or absence folded into one place so no second path can ever forget. When the same null needs its third guard, stop counting guards and pick one of those.

The acceptance test is the configuration matrix the project was missing, and the driver states it at the top:

```cpp
#include "session.h"
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    // Device configuration under test. build_all.sh runs BOTH: 1 is the
    // bench's calibrated unit, 0 the field's base model - the crash lived
    // only in the second, and one configuration cannot prove both.
    const bool calibrated = !(argc > 1 && std::atoi(argv[1]) == 0);

    Session s(calibrated);
    for (int i = 1; i <= 8; ++i) {
        s.Ingest(static_cast<double>(i));    // mean 4.5, exactly
    }
    const double report = s.Report();

    const double expected = calibrated ? 4.5 * 0.5 + 1.0 : 4.5;
    if (report != expected) {
        std::printf("FAILED: report %.3f, expected %.3f (calibrated=%d)\n",
                    report, expected, calibrated ? 1 : 0);
        return 1;
    }
    std::printf("session ok: %s device, report %.2f\n",
                calibrated ? "calibrated" : "base-model", report);
    return 0;
}
```

`build_all.sh` runs it both ways under the canonical flags — the calibrated bench and the bare field unit — because the crash lived only in the configuration the matrix never had, and one configuration cannot prove a claim about both. The other half of the fix is institutional and was already in place, which is the only reason this ticket took an afternoon instead of a week: *the release job archived the symbol file next to the tag.* If your project's release process does not do that today, that is the actual action item of this chapter.

### Pitfalls

- **Guarding the symptom line and closing the ticket.** A null check slapped at whatever line the naive symbolication named — line 23, remember, which cannot fault — fixes nothing; even placed correctly, ask *whose job the check is* before adding it. Here the invariant's own text ("every path that touches it owns the check") assigns it to the helper. When the answer is "everyone's job", that is not an answer — it is the design debt above.
- **Expecting the shipped stack to look like your dev stack.** At `-O0` the crash has three tidy app frames and `FoldedMean` appears by name; at `-O2` it has two, and the culprit exists only in the debug info's inline records. Both are true pictures of different programs. Diagnose the customer's build with the customer's build's symbols, then reproduce at `-O0` for the comfortable view — [Chapter 31](31-reading-what-the-tools-tell-you.md#chapter-31--reading-what-the-tools-tell-you)'s advice, now with a corpse.
- **Shipping without archiving symbols.** Strip the binary, lose the dSYM/PDB, and every field crash you will ever receive is permanently a page of hex. The archive costs megabytes; its absence costs the ticket. One symbol file per shipped build, matched by build id, kept as long as the build is in the field.
- **Trusting a symbolicated line without inline expansion.** The naive lookup interrogates an innocent function — and an afternoon dies proving line 23 cannot do what the report says it did. `-i` on `atos` and `addr2line`; debuggers show it as the bracketed extra frame. Make it the reflex, not the fallback.
- **Reading "cannot reproduce" as "cannot be understood".** The report's metadata — which units, which sites, what they share — is evidence with the same standing as the stack. Here it *was* the repro instruction. The bench's rigs being better-equipped than the field's is not bad luck; it is the standing bias of every bench, and worth one standing question in every triage: *what does the field have that the bench does not — or the reverse?*

> [!TIP]
> **Key principle:** "A fault address just past null is a member offset — I read it against the struct definition before I read a line of code."

> [!TIP]
> **Key principle:** "A crash report is only as good as the symbol files I archived on release day — I keep the dSYM or PDB for every shipped build, and I symbolicate with inline expansion, because the frame that did it may live inside the frame that is named."

### In the wild

This chapter's artifact has an industry around it. Hosts and large applications embed a crash-reporting pipeline — Breakpad and its successor Crashpad are the open-source engines under many of them — that catches the fault, writes a minidump, and ships it home; services then aggregate thousands of field crashes by stack signature, so "three reports from two sites" arrives pre-counted on a dashboard. The Windows half of the world runs on exactly this discipline at OS scale: Windows Error Reporting minidumps, WinDbg's `!analyze`, and *symbol servers* — infrastructure whose entire job is this chapter's second principle, PDBs archived for every build and fetched by build id on demand. Linux has the same organs under different names: `coredumpctl`, `gdb` against a core, build-id-stamped debug info split into its own packages. And the capability-shaped bug that filled this ticket has a standing name in device work — the *bench bias*: support's rigs are the full-featured units, so the base model's code paths are the least-executed in the building while being the most numerous in the field. The fix is the one this lab wired: the configuration matrix in CI, with the cheapest unit in the matrix — because the field's most common machine should never be your least-tested one.

### Reproduce it cold

A week or two from now, closed book: take any struct from your own code, write down its members' offsets by hand, and say what fault address a null-pointer access to each member would produce — then check yourself with `offsetof`. Re-state the two principles from memory: what a near-null address converts to, and what a crash report is worth without the symbol file. If you can also explain why a symbolicated line can be innocent — whose code range an inlined instruction lives in — you will never again lose an afternoon interrogating `return FoldedMean();`. The schedule is in [Chapter 24](24-practice-plan.md#chapter-24--practice-plan).

<!-- nav:begin -->
[← Chapter 36 — The Host Stutters](36-the-host-stutters.md) · [Contents](README.md) · [Chapter 38 — The Bridge Out →](38-the-bridge-out.md)
<!-- nav:end -->
