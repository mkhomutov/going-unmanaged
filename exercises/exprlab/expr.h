// Chapter 42's lab - the formula field: user-typed text evaluated against the
// host's objects, through a provider the caller injects. This header is
// included WHOLE by book/42-the-formula-field.md from below this banner: edit
// here and the page follows. expr.cpp is included by excerpt, between section
// markers; main.cpp is the judge.
// --8<-- [start:listing]
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
// walked per row. Both are bounded by max_depth - the nesting the parser
// recurses through and the height of the tree the evaluator walks - because
// the text is the user's and the stack is the host's.
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
// --8<-- [end:listing]
