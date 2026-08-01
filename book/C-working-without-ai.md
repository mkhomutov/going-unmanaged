## Appendix C — Working Without AI Assistants

If your workplace doesn't permit AI tools, the skill to build is **self-sufficiency**: answering your own questions with docs, a debugger, and memory. It is slower but not weaker — retention runs deeper precisely because every answer costs effort. The lookup-and-reason loop is a muscle; it comes back.

### Your offline lifelines

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

### What must live in your head vs what may live in the docs

**In your head (Appendix B material):** the Rule of Five shape, const auto& reflex, catch-by-const-reference, the erase-during-iteration fix, member initializer lists, checking every SDK error code, virtual destructors on polymorphic bases. **In the docs, guilt-free:** exact signatures, container method names, algorithm spellings, API struct fields, format specifiers. Knowing which is which removes both cramming anxiety and lookup shame.

> [!IMPORTANT]
> **Habit:** Every surprise goes into the notes file the moment it happens — not "later". The file is only as good as its worst day.

---

---


<!-- nav:begin -->
[← Appendix B — Core Principles (Cheat Sheet)](B-core-principles.md) · [Contents](README.md) · [Appendix D — Resources, Further Reading, and First-Week Tips →](D-resources.md)
<!-- nav:end -->
