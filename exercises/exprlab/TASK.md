# Exercise: The Formula Field (user-typed text, evaluated against injected objects)

A formula box for a computed column: **Chapter 42** of the book. ~3 h. Do
it cold in a directory of your own — `expr.h`, `expr.cpp` and `main.cpp`
beside this card are the reference, no peeking until yours runs.

*Trains: Chapter 42 — tokens as a closed set (Chapter 10), a tree behind
`unique_ptr` (Appendix H), recursive descent with a value-or-error at every
level (Chapter 8), the provider seam (Chapter 28), a bounded recursion
(Chapter 3), and a judge whose oracle is a table worked out by hand
(Chapter 34).*

Standard library only. No FakeSDK: the "host" is a provider you write
yourself, which is the point.

## The formula language

Numbers (`1`, `1.5`, `.5`), dotted names (`wall.width`), `+ - * /`, unary
`-`, parentheses, calls with comma-separated arguments (`max(a, b)`), and
the comparisons `< > <= >= == !=`, which yield 1 or 0. Whitespace anywhere.
`*` and `/` bind tighter than `+` and `-`; every binary operator is
left-associative, so `8 - 3 - 2` is 3.

## Tasks

1. **The header.** `Error{pos, what}` — a value, never a throw, with the
   byte offset the caret goes at. `ISymbols` — `Value(name)` and
   `Call(name, args)`, each returning `std::optional<double>`, and the dot
   in `wall.width` belongs to the provider, not to you. `Formula` — parsed
   once with `Parse(text, max_depth)`, evaluated per row with
   `Evaluate(const ISymbols&)`, both returning a `std::variant` of the
   answer and `Error`. Decide, and write down in a comment, why the tree
   keeps no pointer to the provider.
2. **The tokenizer.** A `std::variant` of token kinds, each token carrying
   the offset it began at, names owning their text (not `string_view` — ask
   yourself what happens when the caller passes a temporary). Numbers
   through `std::from_chars`, never `strtod`. Refuse the first byte that is
   nothing, with its position.
3. **The parser.** One function per precedence level, each returning
   value-or-error. Make `8 - 3 - 2` come out 3 *before* you add
   comparisons or calls — the natural right-recursive spelling of the
   grammar gives 7 and passes every one-operator test.
4. **The tree and the evaluator.** Node kinds as a variant (a closed set —
   why not a class hierarchy?), children behind `unique_ptr` (why must
   they be?), one `std::visit` to evaluate. Division by zero, an unknown
   name and an unknown function or wrong arity are all `Error`s at the
   position of the culprit.
5. **The depth guard.** Bound the recursion with `max_depth`; refuse with a
   position. Paste a thousand `(` and confirm you get an `Error`, not a
   signal. Then remove the guard once, run the same input under the
   canonical flags, and read what the sanitizer does and does not say.
6. **The judge.** A `CHECK` that counts failures and sets the exit code
   (not `assert`, which Release compiles away). A table of formulas with
   values worked out **by hand**, each row chosen for a mistake it catches;
   error *positions*, not error presence; the depth limit accepted at N and
   refused at N + 1; and the locale: `1,5` must be refused at position 1
   before and after `std::setlocale(LC_NUMERIC, "de_DE.UTF-8")`, where the
   machine has it.
7. **Stretch.** String literals and a `contains(a, b)` over them; a
   `Formula::Names()` listing the identifiers a formula references, so the
   host can subscribe to exactly those; the seam as a template policy
   (Chapter 41), and a sentence on why the run-time provider was the right
   default here.

Build (from this directory):

```bash
../../scripts/check.sh expr.cpp main.cpp
```

`build_all.sh` builds the reference under the canonical flags and runs
its judge on every push. Nothing here is broken on purpose: the shapes
that fail (right recursion, `strtod`, a token that is a view, no depth
limit) are described in the chapter's Pitfalls and left for you to build
and watch fail, since each passes the sanitizers and fails only the table.
