## Appendix C — Learning With (and Without) AI Assistants

Two situations share this page: the reader whose workplace bans AI tools, and — far more common now — the reader wondering why they should work through a handbook at all when an assistant explains anything on demand. The second question deserves the straight answer first.

### Why this book, when the tutor is free

An assistant is better than this book at explaining — more patient, endlessly rephrasable, tuned to your exact confusion of the moment. Use it for that. What it cannot do is the part of learning that was never explanation:

- **It cannot make you retrieve.** The exercises here are cold on purpose: what you produce unaided is the only honest measure of what you own, and the retrieval attempt itself is what writes the material in. An answer-on-demand loop skips exactly that step every time — fluency that evaporates when the window closes.
- **It cannot attach the evidence.** The ticket chapters hand you a sanitizer report, a hex capture, a vendor's migration notes — pre-broken code with the crime scene intact, and CI keeping it intact. An assistant can improvise an exercise, but it cannot guarantee the bug is real, the report authentic, or the fix's acceptance test meaningful. Here all three are mechanical.
- **It does not know when it is wrong — and in C++ the plausible-but-wrong answer compiles.** This book's claims are pinned by build scripts and CI jobs precisely because remembered C++ drifts; hold the assistant to the same standard rather than a higher opinion.

### The one rule for using an assistant with the exercises

Put it in **review mode**: *critique my attempt against the chapter's pitfalls — do not write the solution.* An assistant that writes the solution has done the retrieval for you, which is to say it has done the learning for you, which is to say nobody did. After your attempt has met its real judges — the compiler, the sanitizers, the reference solution — the assistant is the ideal explainer of whatever gap remains.

The standing habit that goes with it: treat AI-generated C++ as code from a very confident colleague who does not test. It goes through `scripts/check.sh` under the canonical flags before it is believed — exactly the scrutiny Chapter 28 taught you to apply to your own green runs.

### Working without one

If your workplace doesn't permit AI tools, the skill to build is **self-sufficiency**: answering your own questions with docs, a debugger, and memory. It is slower but not weaker — retention runs deeper precisely because every answer costs effort. The lookup-and-reason loop is a muscle; it comes back.

Your offline lifelines:

- **cppreference.com** — the canonical C++ reference; a downloadable offline archive exists. Practice navigating it: its style is terse and standards-flavored, and reading it fluently is itself a skill. Look up vector::erase (find the invalidation notes) and string::c_str (find the lifetime rules) as training.
- **The vendor SDK documentation** — usually ships with the SDK as local HTML; your most-opened window. Learn its structure: functions grouped by subsystem, each with requirements and error codes.
- **The SDK's example projects** — the best teacher for "how do I even...". The move when stuck: find the example doing something similar, read it, adapt.
- **Your own notes file** — every gotcha, every conversion snippet (vendor-string/UTF-8), every "how do I attach the debugger again". One searchable file. Months of accumulated snippets are what experienced add-on developers actually run on.
- **This book** — the chapters for re-learning, Appendix B for the morning re-read, Chapter 13 for toolchain commands.

### The escalation ladder when stuck

Practice this order deliberately; most "stuck" moments dissolve at steps 1–3 if you don't skip them:

1. **Read the error properly.** Compile vs link vs runtime (Chapter 12) tells you where to look before you look anywhere.
2. **cppreference / API docs.**
3. **SDK examples** — grep them for the function name you're fighting.
4. **Debugger and AddressSanitizer** — let the tools tell you the truth.
5. **Your notes file.**
6. **The vendor's developer forum/community** — asking is normal and accepted; answers take days, not seconds, so ask early and keep working meanwhile.
7. **A colleague** — with the error, what you tried, and what you ruled out. That framing earns respect and faster help.

(With an assistant permitted, it slots in at step 2 as the faster spelling of "docs" — the ladder's order, error first, still holds, because a question asked before reading the error is a question the error already answered.)

### What must live in your head vs what may live in the docs

**In your head (Appendix B material):** the Rule of Five shape, const auto& reflex, catch-by-const-reference, the erase-during-iteration fix, member initializer lists, checking every SDK error code, virtual destructors on polymorphic bases. **In the docs, guilt-free:** exact signatures, container method names, algorithm spellings, API struct fields, format specifiers. Knowing which is which removes both cramming anxiety and lookup shame — and it is the same split whether "the docs" answer in fifteen seconds or an assistant does.

> [!TIP]
> **Habit:** Every surprise goes into the notes file the moment it happens — not "later". The file is only as good as its worst day.

---


<!-- nav:begin -->
[← Appendix B — Core Principles (Cheat Sheet)](B-core-principles.md) · [Contents](README.md) · [Appendix D — Resources, Further Reading, and First-Week Tips →](D-resources.md)
<!-- nav:end -->
