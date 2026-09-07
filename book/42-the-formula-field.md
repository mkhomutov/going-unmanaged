## Chapter 42 — The Formula Field

Somewhere in month three a request arrives that is not about the SDK at all. The dashboard wants a *computed column*: the user types `wall.width * 2 > door.offset + 1` into a box, and every row shows the result. Or the export filter wants an expression. Or the alarm threshold wants to be a formula over the sensor's own properties rather than a number. The request is always described as small, and in C# it was: `DataTable.Compute` took a string, `System.Linq.Expressions` built a tree the runtime compiled, Roslyn scripting ran a line of C# against your objects, and if none of those fit there was a NuGet package for it. Someone else parsed it.

Here nobody does. There is no compiler in the process, no reflection to bind `wall.width` to a property, and a parsing library is a [Chapter 27](27-dependency-management.md#chapter-27--dependency-management) decision with a licence attached and a build to keep green. So the formula field is, more often than a C# developer expects, code you write — and it is worth writing once with care, because it is the one place in a plug-in where *user-typed text* decides what the program does next. Every pitfall in it is a pitfall this book has already taught, arriving together: a view into a string that died ([Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency)), a recursion the user's text sizes ([Chapter 3](03-stack-heap-and-undefined-behavior.md#chapter-3--stack-heap-and-undefined-behavior)), a number that parses differently on a German machine, and a seam between the formula and the objects it names that decides whether any of it can be tested without the host ([Chapter 28](28-testing.md#chapter-28--testing)). The lab is `exercises/exprlab/`: a tokenizer, a recursive-descent parser, a tree, an evaluator, and a judge that holds all four to a table of values worked out by hand.

### The shape of the job

Text in, a number out, and in between the objects the caller injects. That last clause is the design: the formula never knows what `wall.width` *is*, only who to ask. The header is the whole contract:

```cpp
#pragma once
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace formula {

// Where the text went wrong, and why - a VALUE (Chapter 8): a formula the
// user mistyped is the ordinary case of this parser, not an event.
struct Error {
    std::size_t pos;      // byte offset into the text: the caret goes here
    std::string what;
};

// The seam (Chapters 28 and 41): the host's objects, as the formula sees
// them. "wall.width" is one name to this interface - the dot belongs to the
// provider, which is what lets the same formula run against a live host and
// against a map in a test. nullopt means "no such name" and "no such
// function, or wrong arity": both are errors the formula reports with the
// position of the name.
class ISymbols {
public:
    virtual ~ISymbols() = default;
    virtual std::optional<double> Value(std::string_view name) const = 0;
    virtual std::optional<double> Call(std::string_view name, const std::vector<double>& args) const = 0;
};

// A formula parsed once and evaluated many times - the shape of a computed
// column: the text is checked when the user types it, and the tree is
// walked per row. Parsing is bounded (max_depth) because the text is the
// user's and the stack is the host's.
class Formula {
public:
    static std::variant<Formula, Error> Parse(std::string_view text, int max_depth = 64);
    std::variant<double, Error> Evaluate(const ISymbols& symbols) const;

    Formula(Formula&&) noexcept;
    Formula& operator=(Formula&&) noexcept;
    ~Formula();

    struct Node;                                  // the tree: a closed set of node kinds behind one pointer
private:
    explicit Formula(std::unique_ptr<Node> root);
    std::unique_ptr<Node> root_;
};

// One call for the one-off case: parse and evaluate, errors from either step.
std::variant<double, Error> Evaluate(std::string_view text, const ISymbols& symbols, int max_depth = 64);

}  // namespace formula
```

Four decisions live in those forty lines, and each is a chapter's. **A mistyped formula is a value, not an event** — [Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes)'s decision, made the way its drill made it for the malformed row: the ordinary outcome of a box the user types into is a typo, and a parser that throws once per keystroke has filed the common case under exceptional. The value carries a *position*, because a parser that says "syntax error" is one nobody can use; the caret goes at `pos`. **The provider is an interface** — [Chapter 28](28-testing.md#chapter-28--testing)'s seam, designed first: the vendor's document is one implementation, a map is another, and the whole lab runs against the map on a machine with nothing installed. [Chapter 41](41-templates-you-will-write.md#chapter-41--templates-you-will-write) offered the other seam, a template parameter, and its own question decides against it here: the provider is chosen at run time, by which document is open, so it stays a virtual base. **The formula is parsed once** and evaluated per row, which is why `Formula` is a type and not a function — the text is checked when the user types it, the tree is walked when the data changes, and `Formula::Node` is declared and never defined in the header, [Chapter 30](30-authoring-an-abi-boundary.md#chapter-30--authoring-an-abi-boundary)'s PIMPL rule with the destructor and the moves defined where `Node` is complete. And **parsing is bounded**: `max_depth` exists because the text is the user's and the stack is the host's, which is the subject of the depth guard below.

Notice also what the header does not promise: `Evaluate` takes the provider on every call and the tree keeps no pointer to it. A formula that stored `const ISymbols*` at parse time would be [Chapter 33](33-here-is-the-report.md#chapter-33--here-is-the-report)'s loan with no term — the document it pointed at closes, the tree outlives it, the next row is a use-after-free — and the fix is the same as that chapter's: borrow at the point of use.

### Tokens: the closed set

The tokenizer turns bytes into a vector of tokens, each a closed set of kinds and each carrying the byte offset it began at:

```cpp
struct Number { double value; };
struct Ident  { std::string name; };      // "wall.width": the dot is part of the name
struct Op     { char c; };                // + - * / < > = ! , and the two-character forms below
struct Compare { std::string op; };       // "<=", ">=", "==", "!="
struct LParen {};
struct RParen {};
struct End {};

struct Token {
    std::size_t pos;
    std::variant<Number, Ident, Op, Compare, LParen, RParen, End> kind;
};
```

A `std::variant`, because the set is closed and the compiler can then refuse a `visit` that forgets a kind ([Chapter 10](10-modern-cpp-fluency.md#chapter-10--modern-c-fluency)); a `std::string` inside `Ident` rather than a `string_view`, because the tokens outlive the call that tokenized — they sit in the parser while the caller's text may not — and a view into a temporary is the Chapter 10 trap in its most common disguise. `Ident` also settles the injected-object question at the cheapest point: `wall.width` is one identifier, dot included, and what the dot means is the provider's decision, not the parser's. The parser has no idea there are objects. The provider does.

The number scan is one function, and its one decision is the chapter's second trap:

```cpp
// The number scan. from_chars, never strtod: strtod reads the C locale's
// decimal separator, so on a German machine "1,5" parses as one and a half
// and "1.5" stops at the dot - the bug that works on the bench. from_chars
// knows one spelling, the wire's, on every machine.
Token ScanNumber(std::string_view text, std::size_t& i) {
    const std::size_t start = i;
    while (i < text.size() && (IsDigit(text[i]) || text[i] == '.')) ++i;
    double value = 0.0;
    const auto [end, ec] = std::from_chars(text.data() + start, text.data() + i, value);
    if (ec != std::errc{} || end != text.data() + i) {
        i = start;                                // let the caller report "not a number" at start
        return Token{start, End{}};
    }
    return Token{start, Number{value}};
}
```

`std::strtod`, `std::stod` and `atof` read the decimal separator from the process's C locale, and a plug-in does not own its process's locale: the host, or a library the host loaded, may have called `setlocale` for its own reasons. On such a machine `1,5` is one and a half and `1.5` is one — silently, with the formula's own tests green on every developer's laptop, because every developer's laptop is in the C locale. `std::from_chars` (Recipe 19 in [Appendix F](F-rosetta-cookbook.md#appendix-f--the-rosetta-cookbook)) knows one spelling and reads the same bytes the same way everywhere, which is [Chapter 34](34-parse-this-capture.md#chapter-34--parse-this-capture)'s wire discipline applied to a number in a text box. The judge switches the process to `de_DE` where the machine has it and asserts nothing changed.

> [!WARNING]
> **Trap:** `std::stod("1,5")` returns one on the bench and one and a half on a customer's machine whose host set a German locale — no error, no warning, and a formula that "works" for every developer; parse numbers with `from_chars`, which asks no locale.

### The tree: a closed set of node kinds, behind one pointer

```cpp
struct Formula::Node {
    struct Literal  { double value; };
    struct Name     { std::string name; };
    struct Negate   { std::unique_ptr<Node> operand; };
    struct Binary   { std::string op; std::unique_ptr<Node> lhs, rhs; };
    struct CallExpr { std::string name; std::vector<std::unique_ptr<Node>> args; };

    std::size_t pos;                              // where this node's text began: the error's caret
    std::variant<Literal, Name, Negate, Binary, CallExpr> kind;
};
```

Two of [Appendix H](H-choosing.md#appendix-h--choosing-signatures-containers-and-storage)'s decisions in one struct. The node *kinds* are a closed set nobody extends from outside, so they are a variant by value — no base class, no virtual `Eval`, and a `visit` that will not compile if a kind is missing. The *children* sit behind `unique_ptr`, and here the appendix's fourth procedure has its one structural answer rather than a preference: a `Node` cannot hold a `Node` by value, because the type would contain itself, so a tree boxes its edges whether or not anything else argues for it. The C# reflex — a `Node` class hierarchy with a virtual `Evaluate` — is not wrong here; it is the open-set shape, and the set is closed. A reader who wants the hierarchy anyway has Chapter 5's rules to obey and gains nothing for it.

### The parser: one function per precedence level

Recursive descent is the technique of writing the grammar down as functions, one per precedence level, each calling the level below it:

```cpp
//   comparison := additive ( ('<' | '>' | '<=' | '>=' | '==' | '!=') additive )?
//   additive   := term ( ('+' | '-') term )*
//   term       := unary ( ('*' | '/') unary )*
//   unary      := '-' unary | primary
//   primary    := NUMBER | NAME | NAME '(' args ')' | '(' comparison ')'
//
// Left-associative by construction: additive LOOPS over its operands, so
// 8 - 3 - 2 is (8 - 3) - 2. A version that recursed on the right instead
// would give 8 - (3 - 2) = 7, and pass every test with one operator in it.
```

The whole of precedence is that `Term` is called *from* `Additive`, so a `*` is consumed before the `+` around it ever sees its operands. And the whole of associativity is in the shape of one function:

```cpp
    std::variant<NodePtr, Error> Additive() {
        auto lhs = Term();
        if (std::holds_alternative<Error>(lhs)) return lhs;
        for (;;) {
            const std::size_t pos = Peek().pos;
            char c = 0;
            if (TakeOp('+')) c = '+'; else if (TakeOp('-')) c = '-'; else break;
            auto rhs = Term();
            if (std::holds_alternative<Error>(rhs)) return rhs;
            lhs = std::make_unique<Node>(Node{pos, Node::Binary{std::string(1, c), std::move(std::get<NodePtr>(lhs)), std::move(std::get<NodePtr>(rhs))}});
        }
        return lhs;
    }
```

A loop, folding each new operand into the tree on the left. The version that reads more naturally from the grammar — `additive := term ('+' additive)?`, recursing on the right — is the chapter's wrong-that-looks-like-working: it parses `8 - 3 - 2` as `8 - (3 - 2)` and returns 7, and every test with one operator in it passes, and so does every test with two operators of different precedence. Only `a - b - c` with the same operator twice tells the two apart, which is why the judge's table has that row and the `16 / 4 / 2` row beside it. This is [Chapter 34](34-parse-this-capture.md#chapter-34--parse-this-capture)'s lesson about oracles: no sanitizer knows that 7 is wrong. A value table worked out by hand is the only judge there is, and the rows in it are chosen for the mistakes they can catch.

The error handling is [Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes)'s value pole in every function: each level returns a `variant<NodePtr, Error>`, and a failure below is handed up unchanged, position and all, until `Parse` returns it. It is the error-code shape — a check after every call — and the chapter chose it over a `throw` caught in `Parse` for the reason that chapter gives: the failure here is the common case, and a formula being retyped fires it on every keystroke.

### The depth guard

Every `(` and every unary `-` recurses one level deeper, which means the user's text sizes the host's stack:

```cpp
    // The depth guard. Every parenthesis and every unary minus recurses, so
    // "((((((((1" is a stack frame per byte - the user's text sized the
    // host's stack. Refuse past max_depth with a position, before the stack
    // does it with a signal (Recipe 34's crash, none of Chapter 31's shapes).
    struct Depth {
        Depth(int& d, int limit, bool& over) : depth_(d) { over = ++depth_ > limit; }
        ~Depth() { --depth_; }
        int& depth_;
    };
```

```cpp
    std::variant<NodePtr, Error> Unary() {
        bool over = false;
        Depth guard(depth_, max_depth_, over);
        if (over) return Error{Peek().pos, "expression nests too deeply"};
        const std::size_t pos = Peek().pos;
        if (TakeOp('-')) {
            auto operand = Unary();
            if (std::holds_alternative<Error>(operand)) return operand;
            return std::make_unique<Node>(Node{pos, Node::Negate{std::move(std::get<NodePtr>(operand))}});
        }
        return Primary();
    }
```

A thousand opening parentheses pasted into the box is a thousand nested calls, and each is several frames deep through `Comparison`, `Additive`, `Term` and `Unary`; on [Chapter 3](03-stack-heap-and-undefined-behavior.md#chapter-3--stack-heap-and-undefined-behavior)'s 512 KB worker-thread stack that is a crash on some frame's entry, with none of [Chapter 31](31-reading-what-the-tools-tell-you.md#chapter-31--reading-what-the-tools-tell-you)'s report shapes and no line of the formula's code in the report — Recipe 34's failure, delivered by a text box. The guard is [Chapter 1](01-ownership-and-raii.md#chapter-1--ownership-and-raii)'s RAII counting depth on the way in and out, and the refusal is a value with the position where the level too deep begins. Sixty-four is a limit no human formula reaches and every hostile one does; the judge accepts sixty-four levels, refuses sixty-five with the position, and refuses a thousand parentheses and a thousand minus signs in microseconds.

### The evaluator: the injected object answers

```cpp
        } else if constexpr (std::is_same_v<K, Node::Name>) {
            // The injected object answers, or the formula reports the name -
            // the formula never knows what "wall.width" is, only who to ask.
            if (const auto v = symbols.Value(k.name)) return *v;
            return Error{node.pos, "unknown name '" + k.name + "'"};
```

`Eval` is one `std::visit` over the node variant, with `if constexpr` sorting the kinds ([Chapter 41](41-templates-you-will-write.md#chapter-41--templates-you-will-write)'s `Describe`, over a tree), and the `Name` branch is where the objects come in: the provider is asked, and the answer or the refusal comes back with the node's position, so an unknown property is reported at the character the user typed it. The same branch for `CallExpr` hands the provider a name and the evaluated arguments, and a `nullopt` there means either no such function or the wrong number of arguments — the provider decides, since it is the provider that knows what `max` takes. Division by zero is an `Error` at the `/`, not a NaN the row would display as `nan`; comparisons yield 1 or 0, the way the box's users read a filter.

The lab's provider is a map of objects, each a map of properties, and three functions:

```cpp
    std::optional<double> Value(std::string_view name) const override {
        const auto dot = name.find('.');
        if (dot == std::string_view::npos) return std::nullopt;
        const auto obj = objects_.find(std::string(name.substr(0, dot)));
        if (obj == objects_.end()) return std::nullopt;
        const auto prop = obj->second.find(std::string(name.substr(dot + 1)));
        if (prop == obj->second.end()) return std::nullopt;
        return prop->second;
    }
```

That is [Chapter 28](28-testing.md#chapter-28--testing)'s fake you own, and the reason the whole lab runs on a laptop: the host adapter that resolves `wall.width` against a live document — on the main thread, at the safe point, through [Chapter 38](38-the-bridge-out.md#chapter-38--the-bridge-out)'s queue if the formula is evaluated for a client — is one more `ISymbols`, and the tree, the parser and the judge never change.

### The judge

Four things are asserted, because four things could be wrong in a way no tool reports:

```cpp
    const std::vector<std::pair<const char*, double>> table = {
        {"1 + 2 * 3", 7},                        // precedence: * binds tighter
        {"(1 + 2) * 3", 9},                      // parentheses override it
        {"8 - 3 - 2", 3},                        // LEFT-associative: (8-3)-2, not 8-(3-2)=7
        {"16 / 4 / 2", 2},                       // same for division
        {"-2 * -3", 6},                          // unary minus, twice
        {"--4", 4},                              // stacked unary
        {"wall.width * 2", 8},                   // the injected object
        {"wall.width * wall.height", 10},
        {"max(wall.width, door.offset) + 1", 5}, // a call with two arguments
        {"abs(-wall.height)", 2.5},
        {"wall.width > 3", 1},                   // comparisons yield 1 or 0
        {"wall.width * 2 >= door.offset + 7", 0},
        {"1.5 + .5", 2},                         // a leading-dot fraction
        {" 2  +  2 ", 4},                        // whitespace anywhere
        {"min(1, 2) == 1", 1},
    };
```

**Values, worked out by hand** — the oracle of [Chapter 34](34-parse-this-capture.md#chapter-34--parse-this-capture), and every row chosen for a mistake it can catch: the third row is the associativity bug, the fifth and sixth the unary minus, the seventh the injection. **Error positions**, not error presence — `1 + * 2` fails at 4, `wall.depth` at 0, `1 / (2 - 2)` at 2 — because a parser judged on "returned an error" would pass with `pos` always zero. **The depth limit** at exactly N and N + 1, and then at a thousand. And **the locale**:

```cpp
    CHECK(ErrorAt(Evaluate("1,5", host), 1));
    if (std::setlocale(LC_NUMERIC, "de_DE.UTF-8") != nullptr) {
        CHECK(ErrorAt(Evaluate("1,5", host), 1));
        CHECK(ValueIs(Evaluate("1.5 * 2", host), 3));
        std::setlocale(LC_NUMERIC, "C");
    }
```

The judge is a `CHECK` that counts and sets the exit code, never `assert` — [Appendix H](H-choosing.md#appendix-h--choosing-signatures-containers-and-storage)'s rule for a harness, since a Release build compiles `assert` away and a judge that vanishes is worse than none. `build_all.sh` builds the two translation units under the canonical flags and runs it on every push; the sanitizers watch the tree's pointers, and the table watches the arithmetic, and neither could do the other's job — Finding 10 of [Chapter 25](25-findings-from-practice.md#chapter-25--findings-from-practice-a-living-log), one more time.

> [!TIP]
> **Key principle:** "User-typed text is data until my parser says so — tokens I own, a tree of a closed set of kinds, evaluation through a provider the caller injects, a bounded depth, and an error that carries a position rather than a throw."

### In the wild

- **Formula fields are everywhere in host applications** — the expression bar of a spreadsheet-shaped panel, the filter box of a query tool, a parametric dimension in a CAD package, an automation curve in a DAW. The vendor's SDK frequently exposes its *own* expression language for those, evaluated by the host; check for it before writing one, the way [Appendix G](G-the-bridge-catalogue.md#appendix-g--the-bridge-catalogue) says to check for the host's own automation channel. When the host has none, the lab's shape is what you build.
- **The libraries** are a [Chapter 27](27-dependency-management.md#chapter-27--dependency-management) decision: header-only expression evaluators exist and are widely used, and their price is the usual one — a large header in every translation unit that includes it, and a grammar you do not own. When the formula language must grow variables, strings, control flow and user-defined functions, it has stopped being a formula and become a scripting language, and Appendix G's embedded interpreter is the honest next step.
- **Objects are injected on the main thread.** A provider that resolves `wall.width` against a live document is an SDK call, and [Chapter 38](38-the-bridge-out.md#chapter-38--the-bridge-out)'s invariant applies to it: evaluated from a transport thread, the formula is a job posted to the main-thread queue, and the provider is the stub adapter's method with the thread assert on its first line.
- **The error position is the product.** A formula box that underlines the character the parser stopped at is the difference between a feature users adopt and one they abandon; `Error::pos` exists for that caret, and every branch that returns an error carries the position of the token it was looking at.

### Pitfalls

- **Tokens as views into the text.** A `string_view` in `Ident` is zero copies until the caller passes a temporary — `Evaluate(line.substr(5), host)` — and then the parser reads a string that died at the semicolon. The lab's tokens own their names; the price is a few short-string copies per parse, and parsing happens once per formula, not once per row.
- **Right recursion for a left-associative operator.** The grammar reads more naturally that way, every one-operator test passes, and `8 - 3 - 2` is 7. The loop in `Additive` is the fix and the `8 - 3 - 2` row is the judge.
- **`std::stod`.** Locale-dependent, and the locale is the host's. `from_chars`, always; and a test that switches the locale, or the bug ships.
- **No depth limit.** The parser is correct for every formula a human types and crashes the host on the first hostile one — from a pasted file, a fuzzer, or a customer's macro that generates formulas. Bound the recursion and refuse with a position.
- **Throwing for a parse error.** [Chapter 8](08-error-handling.md#chapter-8--error-handling-exceptions-and-error-codes)'s hot-loop trap in its purest form: the box is retyped a hundred times a minute, and each keystroke is a throw. The error is the common case here; it is a value.
- **Storing the provider in the tree.** A `const ISymbols*` member of `Formula` reads as convenience and is [Chapter 33](33-here-is-the-report.md#chapter-33--here-is-the-report)'s loan with no term: the document closes, the column is still bound to the formula, and the next refresh dereferences a dead adapter. The provider is a parameter of `Evaluate`, per call.
- **Evaluating against the host off the main thread.** The parser is pure and thread-agnostic; the provider is not. A formula evaluated for a client on a transport thread must ride [Chapter 38](38-the-bridge-out.md#chapter-38--the-bridge-out)'s queue.

### Try it

The lab is `exercises/exprlab/` — the task card walks the same road as this chapter, cold; `expr.h`, `expr.cpp` and `main.cpp` are the reference, and the card asks you not to open them until yours runs. In outline:

1. **Write the tokenizer** for numbers, dotted names, the operators, parentheses and the comma, each token carrying its byte offset; refuse the first byte that is nothing, with its position. Parse numbers with `from_chars`.
2. **Write the parser** as one function per precedence level, returning a value-or-error at every level, and make `8 - 3 - 2` come out 3 before adding anything else.
3. **Write the provider seam** — `Value` and `Call` over a `string_view` name — and a fake over a map of objects; make `wall.width * 2` evaluate.
4. **Add the depth guard**, then paste a thousand parentheses and confirm you get a position, not a signal. Remove the guard once, run the same input under the canonical flags, and read what the sanitizer does and does not say.
5. **Write the judge**: the value table by hand first, the error positions, the depth limit at N and N + 1, and the locale switch. `scripts/check.sh expr.cpp main.cpp` is the one-line build.
6. **Stretch:** string literals and a `contains` function over them; a `Formula::Names()` that lists the identifiers a formula references, so the host can subscribe to exactly those properties; the seam as a template policy ([Chapter 41](41-templates-you-will-write.md#chapter-41--templates-you-will-write)), and a sentence on why the run-time provider was the right default.

---

<!-- nav:begin -->
[← Chapter 41 — Templates You Will Write](41-templates-you-will-write.md) · [Contents](README.md) · [Appendix A — Fundamentals Refresher →](A-fundamentals-refresher.md)
<!-- nav:end -->
