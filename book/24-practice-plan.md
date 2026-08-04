## Chapter 24 — Practice Plan

A one-week hands-on plan (compress or stretch as needed). Each day's exercise now has a full worked chapter: Day 1 → Chapter 14 (Tracer), Day 2 → Chapter 15 (Buffer), Day 3 → Chapter 19 (Word Counter), plus the SDK track — Chapter 16 (the Bestiary, read first), Chapter 17 (FakeSDK) and Chapter 18 (Device SDK) — and the standalone labs: Chapter 20 (Slicing), Chapter 21 (Invalidation), Chapter 22 (Lambdas), Chapter 23 (Build Model). Do exercises cold first; read the chapter's solution and pitfalls after. The rule that makes it work: **do the exercises cold** — compiler, debugger, sanitizer, and offline docs as your only feedback loops. That trains the self-sufficiency the job requires.

### The week

- **Day 0 — Setup.** IDE with /W4 (or -Wall -Wextra) and C++17 — plus a C++20 switch for Day 3, whose `std::erase_if` needs it. On Windows that means Visual Studio and its Developer PowerShell, where `scripts\check.ps1` is the labs' one-line judge (`scripts/check.sh` everywhere else — same flags, same verdict). Offline cppreference archive installed. If you are targeting a specific host application or device, download its SDK now. Create notes.md — your permanent gotcha file; first entry: today's setup steps.
- **Day 1 — Hands.** From scratch, no lookups until stuck: a class printing from ctor/dtor (watch RAII fire under the debugger); pass it by value / by ref / by move and predict output before running. Then the FileHandle RAII wrapper from memory.
- **Day 2 — Buffer + sanitizer.** Rule of Five Buffer cold, from memory. Then break it deliberately three ways (remove the null-out in move; non-virtual base dtor; erase-during-iteration) and note which tool catches which: the double-free is a loud ASan report, erase-during-iteration is a `container-overflow`, and the non-virtual destructor is a `-Wall` warning at compile time *only when the base has some other virtual function* — give your toy base one, because deleting through a non-polymorphic base compiles without a word (MSVC's `/W4` stays quiet either way) — and then a *leak*, which your platform may simply never report (Chapter 31). Read the reports until they make sense, and write down the break that said nothing: that is Finding 10 in your own terminal. Notes: what each report looks like, and which break was silent.
- **Day 3 — STL fluency.** Word-frequency exercise: file → vector → unordered_map counts → sort by count with a lambda → erase_if filter → top 10. cppreference-only rule in force. Then step through it in the debugger inspecting containers.
- **Day 4 — Your real SDK (or the FakeSDK/FakeDevice labs).** If you have a target SDK: build its example plug-ins, load one into the host, and — the key skill — attach the debugger to the host process and hit a breakpoint inside your code. Expect friction; friction is the curriculum. If not, do Chapters 17 and 18 back to back. Notes: exact attach steps, build config gotchas.
- **Day 5 — First real build (docs + examples allowed freely).** Against your target SDK: an aggregator in the FakeSDK shape — iterate the SDK's elements/devices, check every error code, present a computed result. Use SDK examples as templates — that IS the legitimate at-work workflow.
- **Day 6 — Second build (cold).** Delete yesterday's code. Rebuild using only docs, examples, and your notes. The gap between the two builds tells you exactly what to add to the notes file. If fast: extend with an owned-payload read behind your RAII guard, or whatever transactional/undo mechanism your SDK offers.
- **Day 7 — Consolidate, then stop.** Re-read Appendix B and your notes; rewrite the Buffer one last time from memory; tidy notes into sections (C++ gotchas / API recipes / toolchain steps). Rest. Walk in curious, not depleted.

### Standing drills (repeatable any time)

- **Bug hunt:** write each classic bug on purpose, watch it misbehave, fix it: missing virtual destructor; auto-without-& loop; dangling lambda capture; map operator[] silent insert; erase during iteration; use-after-move; shallow-copy double-free; double-close of an SDK handle.
- **Predict-then-run:** before every run, say out loud what will print and where each object lives (stack/heap) and dies. Wrong predictions go in the notes file.
- **Cold rewrites:** FileHandle, ThingDataGuard, and the Rule of Five Buffer — until each takes under ten minutes without references.
- **Narrate:** explain your reasoning out loud while coding. You'll be explaining decisions to colleagues; the habit transfers directly to code reviews and pair sessions.

---

---


<!-- nav:begin -->
[← Chapter 23 — Exercise: The Build-Model Lab](23-exercise-the-build-model-lab.md) · [Contents](README.md) · [Chapter 25 — Findings from Practice: a Living Log →](25-findings-from-practice.md)
<!-- nav:end -->
