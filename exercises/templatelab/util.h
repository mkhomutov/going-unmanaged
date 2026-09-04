// util.h - the three templates every codebase ends up writing: a variadic
// join with a fold expression, an if constexpr that branches on the type,
// and a class template with a non-type parameter.
//
// Quoted VERBATIM in Chapter 41 below this banner: editing it means editing
// the chapter in the same commit.
#pragma once
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

// The dependent-false idiom: a static_assert in a discarded if constexpr
// branch must depend on T, or an older compiler (clang before 17, GCC
// before 13) refuses the whole template at definition.
template <class>
inline constexpr bool dependent_false_v = false;

// if constexpr: one function, one body per kind of T, and the branch not
// taken is discarded - not instantiated, so std::to_string is never resolved
// against a string_view for the string case (it is still parsed).
template <class T>
std::string Describe(const T& value) {
    static_assert(!std::is_null_pointer_v<T>, "Describe: nullptr has no text");
    if constexpr (std::is_same_v<T, char>) {
        return std::string(1, value);                // char is arithmetic: 'A' is not "65"
    } else if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(value);
    } else if constexpr (std::is_convertible_v<T, std::string_view>) {
        return std::string(std::string_view(value));
    } else {
        static_assert(dependent_false_v<T>, "Describe: no spelling for this type");
    }
}

// The variadic every codebase has: C#'s params object[], resolved at compile
// time. The fold expression `((expr), ...)` expands to expr once per argument.
template <class... Parts>
std::string Join(std::string_view separator, const Parts&... parts) {
    std::string out;
    std::size_t n = 0;
    ((out += (n++ ? std::string(separator) : std::string()) + Describe(parts)), ...);
    return out;
}

// A class template with a non-type parameter: the size is part of the type,
// so a Ring<float, 8> and a Ring<float, 16> cannot be confused, and the
// storage is inline (Chapter 11's std::array, not a heap block).
template <class T, std::size_t N>
class Ring {
    static_assert(N > 0, "a Ring needs room for at least one element");
public:
    void Push(T value) {
        data_[head_] = std::move(value);             // a by-value sink is moved from (Appendix H)
        head_ = (head_ + 1) % N;
        if (size_ < N) ++size_;
    }
    std::size_t Size() const { return size_; }
    // Precondition: Size() > 0. An empty ring has no oldest element.
    const T& Oldest() const { return data_[(head_ + N - size_) % N]; }

private:
    std::array<T, N> data_{};
    std::size_t head_ = 0;
    std::size_t size_ = 0;
};
