// Chapter 42's lab - the tokenizer, the parser and the evaluator behind
// expr.h. The chapter quotes this file by excerpt: the token type, the
// number scan, the precedence ladder, the depth guard, the height check and
// the evaluator's name lookup. Editing a quoted unit means editing the
// chapter in the same commit (the testlab discipline).
#include "expr.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <system_error>
#include <utility>

// The floating-point from_chars arrived later than the integer one: libstdc++
// 11, MSVC 2019 16.4, libc++ 20 - and Apple's libc++ ships it in the dylib,
// so it exists only for a deployment target of macOS 26 or later. Neither of
// the two macros below is defined where the overload is missing (Appendix K).
#if defined(__cpp_lib_to_chars) || \
    (defined(_LIBCPP_AVAILABILITY_HAS_FROM_CHARS_FLOATING_POINT) && _LIBCPP_AVAILABILITY_HAS_FROM_CHARS_FLOATING_POINT)
#define FORMULA_HAS_FROM_CHARS_DOUBLE 1
#else
#include <cstdlib>
#include <locale.h>
#ifdef __APPLE__
#include <xlocale.h>                              // strtod_l, newlocale
#endif
#endif

namespace formula {

namespace {

// ---- tokens ---------------------------------------------------------------

// A closed set of token kinds: a variant (Chapter 10), not a kind field
// beside a union. Each token carries the byte offset it started at, so an
// error can point at it.
struct Number { double value; };
struct Ident  { std::string name; };      // "wall.width": the dot is part of the name
struct Op     { char c; };                // + - * / < > , (the two-character comparisons are Compare, below)
struct Compare { std::string op; };       // "<=", ">=", "==", "!="
struct LParen {};
struct RParen {};
struct End {};

struct Token {
    std::size_t pos;
    std::variant<Number, Ident, Op, Compare, LParen, RParen, End> kind;
};

bool IsIdentStart(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
bool IsIdentChar(char c)  { return IsIdentStart(c) || (c >= '0' && c <= '9') || c == '.'; }
bool IsDigit(char c)      { return c >= '0' && c <= '9'; }

// The number parse. from_chars, never strtod: strtod reads the C locale's
// decimal separator, so on a German machine "1,5" parses as one and a half
// and "1.5" stops at the dot - the bug that works on the bench. from_chars
// knows one spelling, the wire's, on every machine. Where the library has
// no from_chars for double, the same one spelling comes from strtod handed
// a "C" locale of the parser's own, which asks the process's locale nothing.
// Returns where the number ended, or nullptr if [first, last) is not one.
const char* ParseNumber(const char* first, const char* last, double& value) {
#ifdef FORMULA_HAS_FROM_CHARS_DOUBLE
    const auto [end, ec] = std::from_chars(first, last, value);
    return ec == std::errc{} ? end : nullptr;
#else
    static const locale_t c_locale = newlocale(LC_NUMERIC_MASK, "C", static_cast<locale_t>(nullptr));
    const std::string copy(first, last);          // strtod wants a terminator
    char* stop = nullptr;
    value = strtod_l(copy.c_str(), &stop, c_locale);
    return stop == copy.c_str() ? nullptr : first + (stop - copy.c_str());
#endif
}

// The number scan: the longest run of digits and dots, which must parse
// whole - "1..2" is not a number, and "1e5" is a number followed by a name.
Token ScanNumber(std::string_view text, std::size_t& i) {
    const std::size_t start = i;
    while (i < text.size() && (IsDigit(text[i]) || text[i] == '.')) ++i;
    double value = 0.0;
    const char* end = ParseNumber(text.data() + start, text.data() + i, value);
    if (end != text.data() + i) {
        i = start;                                // let the caller report "not a number" at start
        return Token{start, End{}};
    }
    return Token{start, Number{value}};
}

// Tokenize the whole text up front, or stop at the first byte that is
// nothing: the error carries that byte's position.
std::variant<std::vector<Token>, Error> Tokenize(std::string_view text) {
    std::vector<Token> out;
    std::size_t i = 0;
    while (i < text.size()) {
        const char c = text[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') { ++i; continue; }
        if (IsDigit(c) || (c == '.' && i + 1 < text.size() && IsDigit(text[i + 1]))) {
            Token t = ScanNumber(text, i);
            if (std::holds_alternative<End>(t.kind)) return Error{i, "not a number"};
            out.push_back(std::move(t));
            continue;
        }
        if (IsIdentStart(c)) {
            const std::size_t start = i;
            while (i < text.size() && IsIdentChar(text[i])) ++i;
            out.push_back(Token{start, Ident{std::string(text.substr(start, i - start))}});
            continue;
        }
        if (i + 1 < text.size() && text[i + 1] == '=' && (c == '<' || c == '>' || c == '=' || c == '!')) {
            out.push_back(Token{i, Compare{std::string(text.substr(i, 2))}});
            i += 2;
            continue;
        }
        switch (c) {
            case '+': case '-': case '*': case '/': case '<': case '>': case ',':
                out.push_back(Token{i, Op{c}}); ++i; continue;
            case '(': out.push_back(Token{i, LParen{}}); ++i; continue;
            case ')': out.push_back(Token{i, RParen{}}); ++i; continue;
            default: {
                // Quote the byte only if it is one printable character: the first
                // byte of "é" on its own is not text a message can carry.
                if (c >= 0x20 && c < 0x7f) return Error{i, std::string("unexpected character '") + c + "'"};
                char hex[8];
                std::snprintf(hex, sizeof hex, "0x%02X", static_cast<unsigned>(static_cast<unsigned char>(c)));
                return Error{i, std::string("unexpected byte ") + hex};
            }
        }
    }
    out.push_back(Token{text.size(), End{}});
    return out;
}

}  // namespace

// ---- the tree -------------------------------------------------------------

// A closed set of node kinds, so a variant (Appendix H, procedure 4) - and a
// tree, so the children sit behind unique_ptr: a Node cannot hold a Node by
// value, and the pointer is the one reason to box that the appendix lists
// as structural rather than a preference.
struct Formula::Node {
    struct Literal  { double value; };
    struct Name     { std::string name; };
    struct Negate   { std::unique_ptr<Node> operand; };
    struct Binary   { std::string op; std::unique_ptr<Node> lhs, rhs; };
    struct CallExpr { std::string name; std::vector<std::unique_ptr<Node>> args; };
    using Kind = std::variant<Literal, Name, Negate, Binary, CallExpr>;

    std::size_t pos;                              // where this node's text began: the error's caret
    int height;                                   // of this subtree: what Eval and the destructor recurse through
    Kind kind;
};

namespace {

using Node = Formula::Node;
using NodePtr = std::unique_ptr<Node>;

// One more than the tallest child. Kept on every node because the parser's
// depth guard bounds only the parser's OWN recursion: "1+1+1+..." never
// nests in the parser (Additive loops), yet folds into a left spine as tall
// as the text, and Eval and the destructor recurse through that spine.
int HeightOf(const Node::Kind& kind) {
    return std::visit([](const auto& k) -> int {
        using K = std::decay_t<decltype(k)>;
        if constexpr (std::is_same_v<K, Node::Literal> || std::is_same_v<K, Node::Name>) {
            return 1;
        } else if constexpr (std::is_same_v<K, Node::Negate>) {
            return 1 + k.operand->height;
        } else if constexpr (std::is_same_v<K, Node::Binary>) {
            return 1 + std::max(k.lhs->height, k.rhs->height);
        } else {
            static_assert(std::is_same_v<K, Node::CallExpr>);
            int tallest = 0;
            for (const auto& a : k.args) tallest = std::max(tallest, a->height);
            return 1 + tallest;
        }
    }, kind);
}

// ---- the parser: recursive descent, one function per precedence level ----
//
//   comparison := additive ( ('<' | '>' | '<=' | '>=' | '==' | '!=') additive )?
//   additive   := term ( ('+' | '-') term )*
//   term       := unary ( ('*' | '/') unary )*
//   unary      := '-' unary | primary
//   primary    := NUMBER | NAME | NAME '(' args ')' | '(' comparison ')'
//
// Left-associative by construction: additive LOOPS over its operands, so
// 8 - 3 - 2 is (8 - 3) - 2. A version that recursed on the right instead
// would give 8 - (3 - 2) = 7, and pass every test with one operator in it.
// A comparison takes exactly two operands: 1 < 2 < 3 is refused at the
// second '<', not read as (1 < 2) < 3.
class Parser {
public:
    Parser(std::vector<Token> tokens, int max_depth) : tokens_(std::move(tokens)), max_depth_(max_depth) {}

    std::variant<NodePtr, Error> Run() {
        auto tree = Comparison();
        if (auto* err = std::get_if<Error>(&tree)) return *err;
        if (!std::holds_alternative<End>(Peek().kind)) {
            return Error{Peek().pos, "unexpected text after the expression"};
        }
        return std::move(std::get<NodePtr>(tree));
    }

private:
    const Token& Peek() const { return tokens_[index_]; }
    Token Take() { return tokens_[index_++]; }

    bool TakeOp(char c) {
        if (const auto* op = std::get_if<Op>(&Peek().kind); op && op->c == c) { ++index_; return true; }
        return false;
    }

    // The depth guard. Every parenthesis and every unary minus recurses, so
    // "((((((((1" is a stack frame per byte - the user's text sized the
    // host's stack. Refuse past max_depth with a position, before the stack
    // does it with a signal (Recipe 34's crash, none of Chapter 31's shapes).
    struct Depth {
        Depth(int& d, int limit, bool& over) : depth_(d) { over = ++depth_ > limit; }
        ~Depth() { --depth_; }
        int& depth_;
    };

    // Every node is built here, so the tree's height is checked once for all
    // kinds: the guard above bounds what the parser recurses through, this
    // bounds what the evaluator will. One limit serves both.
    std::variant<NodePtr, Error> Build(std::size_t pos, Node::Kind kind) {
        const int height = HeightOf(kind);
        if (height > max_depth_) return Error{pos, "expression is too deep to evaluate"};
        return std::make_unique<Node>(Node{pos, height, std::move(kind)});
    }

    std::variant<NodePtr, Error> Comparison() {
        auto lhs = Additive();
        if (std::holds_alternative<Error>(lhs)) return lhs;
        std::string op;
        if (const auto* cmp = std::get_if<Compare>(&Peek().kind)) { op = cmp->op; }
        else if (const auto* o = std::get_if<Op>(&Peek().kind); o && (o->c == '<' || o->c == '>')) { op = std::string(1, o->c); }
        if (op.empty()) return lhs;
        const std::size_t pos = Take().pos;
        auto rhs = Additive();
        if (std::holds_alternative<Error>(rhs)) return rhs;
        return Build(pos, Node::Binary{op, std::move(std::get<NodePtr>(lhs)), std::move(std::get<NodePtr>(rhs))});
    }

    std::variant<NodePtr, Error> Additive() {
        auto lhs = Term();
        if (std::holds_alternative<Error>(lhs)) return lhs;
        for (;;) {
            const std::size_t pos = Peek().pos;
            char c = 0;
            if (TakeOp('+')) c = '+'; else if (TakeOp('-')) c = '-'; else break;
            auto rhs = Term();
            if (std::holds_alternative<Error>(rhs)) return rhs;
            lhs = Build(pos, Node::Binary{std::string(1, c), std::move(std::get<NodePtr>(lhs)), std::move(std::get<NodePtr>(rhs))});
            if (std::holds_alternative<Error>(lhs)) return lhs;
        }
        return lhs;
    }

    std::variant<NodePtr, Error> Term() {
        auto lhs = Unary();
        if (std::holds_alternative<Error>(lhs)) return lhs;
        for (;;) {
            const std::size_t pos = Peek().pos;
            char c = 0;
            if (TakeOp('*')) c = '*'; else if (TakeOp('/')) c = '/'; else break;
            auto rhs = Unary();
            if (std::holds_alternative<Error>(rhs)) return rhs;
            lhs = Build(pos, Node::Binary{std::string(1, c), std::move(std::get<NodePtr>(lhs)), std::move(std::get<NodePtr>(rhs))});
            if (std::holds_alternative<Error>(lhs)) return lhs;
        }
        return lhs;
    }

    std::variant<NodePtr, Error> Unary() {
        bool over = false;
        Depth guard(depth_, max_depth_, over);
        if (over) return Error{Peek().pos, "expression nests too deeply"};
        const std::size_t pos = Peek().pos;
        if (TakeOp('-')) {
            auto operand = Unary();
            if (std::holds_alternative<Error>(operand)) return operand;
            return Build(pos, Node::Negate{std::move(std::get<NodePtr>(operand))});
        }
        return Primary();
    }

    std::variant<NodePtr, Error> Primary() {
        Token t = Take();
        if (const auto* n = std::get_if<Number>(&t.kind)) {
            return Build(t.pos, Node::Literal{n->value});
        }
        if (auto* id = std::get_if<Ident>(&t.kind)) {
            if (!std::holds_alternative<LParen>(Peek().kind)) {
                return Build(t.pos, Node::Name{std::move(id->name)});
            }
            Take();                                                     // '('
            Node::CallExpr call{std::move(id->name), {}};
            if (!std::holds_alternative<RParen>(Peek().kind)) {
                for (;;) {
                    auto arg = Comparison();
                    if (auto* err = std::get_if<Error>(&arg)) return *err;
                    call.args.push_back(std::move(std::get<NodePtr>(arg)));
                    if (!TakeOp(',')) break;
                }
            }
            if (!std::holds_alternative<RParen>(Peek().kind)) return Error{Peek().pos, "expected ')'"};
            Take();
            return Build(t.pos, std::move(call));
        }
        if (std::holds_alternative<LParen>(t.kind)) {
            auto inner = Comparison();
            if (std::holds_alternative<Error>(inner)) return inner;
            if (!std::holds_alternative<RParen>(Peek().kind)) return Error{Peek().pos, "expected ')'"};
            Take();
            return inner;
        }
        return Error{t.pos, "expected a value"};
    }

    std::vector<Token> tokens_;
    std::size_t index_ = 0;
    int depth_ = 0;
    int max_depth_;
};

// ---- the evaluator ----------------------------------------------------------

// One std::visit over the closed set, its last branch a static_assert: leave
// a node kind out and this does not compile (Chapter 10). Every failure
// carries the node's position, so the caret lands on the name that was
// unknown or the '/' that divided by zero.
std::variant<double, Error> Eval(const Node& node, const ISymbols& symbols) {
    using R = std::variant<double, Error>;
    return std::visit([&](const auto& k) -> R {
        using K = std::decay_t<decltype(k)>;
        if constexpr (std::is_same_v<K, Node::Literal>) {
            return k.value;
        } else if constexpr (std::is_same_v<K, Node::Name>) {
            // The injected object answers, or the formula reports the name -
            // the formula never knows what "wall.width" is, only who to ask.
            if (const auto v = symbols.Value(k.name)) return *v;
            return Error{node.pos, "unknown name '" + k.name + "'"};
        } else if constexpr (std::is_same_v<K, Node::Negate>) {
            auto v = Eval(*k.operand, symbols);
            if (auto* d = std::get_if<double>(&v)) return -*d;
            return v;
        } else if constexpr (std::is_same_v<K, Node::Binary>) {
            auto l = Eval(*k.lhs, symbols);
            if (std::holds_alternative<Error>(l)) return l;
            auto r = Eval(*k.rhs, symbols);
            if (std::holds_alternative<Error>(r)) return r;
            const double a = std::get<double>(l), b = std::get<double>(r);
            if (k.op == "+") return a + b;
            if (k.op == "-") return a - b;
            if (k.op == "*") return a * b;
            if (k.op == "/") {
                if (b == 0.0) return Error{node.pos, "division by zero"};
                return a / b;
            }
            if (k.op == "<")  return a < b ? 1.0 : 0.0;
            if (k.op == ">")  return a > b ? 1.0 : 0.0;
            if (k.op == "<=") return a <= b ? 1.0 : 0.0;
            if (k.op == ">=") return a >= b ? 1.0 : 0.0;
            if (k.op == "==") return a == b ? 1.0 : 0.0;
            return a != b ? 1.0 : 0.0;                               // "!=": the parser admits nothing else
        } else {
            static_assert(std::is_same_v<K, Node::CallExpr>);
            std::vector<double> args;
            for (const auto& a : k.args) {
                auto v = Eval(*a, symbols);
                if (std::holds_alternative<Error>(v)) return v;
                args.push_back(std::get<double>(v));
            }
            if (const auto v = symbols.Call(k.name, args)) return *v;
            return Error{node.pos, "unknown function '" + k.name + "' for " + std::to_string(args.size()) + " argument(s)"};
        }
    }, node.kind);
}

}  // namespace

// ---- Formula ------------------------------------------------------------------

Formula::Formula(std::unique_ptr<Node> root) : root_(std::move(root)) {}
Formula::Formula(Formula&&) noexcept = default;
Formula& Formula::operator=(Formula&&) noexcept = default;
Formula::~Formula() = default;                    // here Node is complete (Chapter 30's PIMPL rule)

std::variant<Formula, Error> Formula::Parse(std::string_view text, int max_depth) {
    auto tokens = Tokenize(text);
    if (auto* err = std::get_if<Error>(&tokens)) return *err;
    Parser parser(std::move(std::get<std::vector<Token>>(tokens)), max_depth);
    auto tree = parser.Run();
    if (auto* err = std::get_if<Error>(&tree)) return *err;
    return Formula(std::move(std::get<NodePtr>(tree)));
}

std::variant<double, Error> Formula::Evaluate(const ISymbols& symbols) const {
    // A moved-from Formula has no tree: a computed column kept in a vector
    // that reallocated is where this would otherwise dereference null.
    if (!root_) return Error{0, "the formula was moved from"};
    return Eval(*root_, symbols);
}

std::variant<double, Error> Evaluate(std::string_view text, const ISymbols& symbols, int max_depth) {
    auto parsed = Formula::Parse(text, max_depth);
    if (auto* err = std::get_if<Error>(&parsed)) return *err;
    return std::get<Formula>(parsed).Evaluate(symbols);
}

}  // namespace formula
