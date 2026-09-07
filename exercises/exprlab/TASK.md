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
`*` and `/` bind tighter than `+` and `-`; `+ - * /` are left-associative,
so `8 - 3 - 2` is 3; a comparison takes exactly two operands, so
`1 < 2 < 3` is refused at the second `<`. A number is the longest run of
digits and dots and must parse whole — `1..2` is refused at 0 — and there
is no exponent: `1e5` is a number followed by a name.

## Tasks

1. **The header.** `Error{pos, what}` — a value, never a throw, with the
   byte offset the caret goes at. `ISymbols` — `Value(name)` and
   `Call(name, args)`, each returning `std::optional<double>`, and the dot
   in `wall.width` belongs to the provider, not to you. The names are
   `std::string_view`. `Formula` — parsed once with
   `static std::variant<Formula, Error> Parse(std::string_view, int max_depth)`,
   evaluated per row with `std::variant<double, Error> Evaluate(const
   ISymbols&) const` — the answer first, then `Error`. Decide, and write down in a comment, why the tree
   keeps no pointer to the provider.
2. **The tokenizer.** A `std::variant` of token kinds, each token carrying
   the offset it began at, names owning their text (not `string_view` — ask
   yourself what happens when the caller passes a temporary). Numbers
   through `std::from_chars`, never `strtod` — and where your library has
   no `double` overload (Apple's libc++ before a macOS 26 deployment
   target), `strtod_l` with a `"C"` locale of your own. Refuse the first
   byte that is nothing, with its position.
3. **The parser.** One function per precedence level, each returning
   value-or-error. Make `8 - 3 - 2` come out 3 *before* you add
   comparisons or calls — the natural right-recursive spelling of the
   grammar gives 7 and passes every one-operator test.
4. **The tree and the evaluator.** Node kinds as a variant (a closed set —
   why not a class hierarchy?), children behind `unique_ptr` (why must
   they be?), one `std::visit` to evaluate. Division by zero is an
   `Error` at the `/`, an unknown name at the name, an unknown function or
   a wrong arity at the function's name.
5. **The depth guard.** Bound the recursion with `max_depth`; refuse with a
   position. Depth is one per nested `(` or unary `-` plus one for the
   value inside, so 63 parentheses round a number is depth 64. Paste a
   thousand `(` and confirm you get an `Error`, not a signal. Then remove
   the guard once and run the same input under the canonical flags — on a
   thread with a 512 KB stack, since the main thread's 8 MB swallows a
   thousand — and read what the sanitizer does and does not say. Then the
   other hostile input: `1+1+…` never nests in the parser and builds a tree
   as tall as the text, which the evaluator walks; bound the height too.
6. **The judge.** A `CHECK` that counts failures and sets the exit code
   (not `assert`, which Release compiles away). A table of formulas with
   values worked out **by hand**, each row chosen for a mistake it catches;
   error *positions*, not error presence; the depth limit accepted at N and
   refused at N + 1, nested and flat; and the locale: after
   `std::setlocale(LC_NUMERIC, "de_DE.UTF-8")`, where the machine has it,
   `1,5` must still be refused at position 1 and `1.5 * 2` must still be 3
   — the second row is the one that catches a `strtod`, since the scan
   never hands it a comma.
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
and watch fail. Three of them pass the sanitizers and fail only the table;
the fourth passes them on every ordinary input and is named
`stack-overflow` only once the hostile one arrives, on a thread that
cannot afford it.
