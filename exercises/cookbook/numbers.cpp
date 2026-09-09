// Appendix F, Recipe 52 - round a number, and turn it into an integer.
//
// The five rounding functions and to_int() are included by
// book/F-rosetta-cookbook.md between the recipe-52 markers: edit here and the
// page follows. main() is scaffolding, and here it is also the point: it is a
// value table, and half its rows exist because C++ answers them differently
// from C#. Every one of those differences was run before it was written down.
//
// The out-of-range cast the recipe guards is undefined behavior, so it is NOT
// executed here - the whole file builds under -fsanitize=undefined, which
// reports it as "1e+20 is outside the range of representable values of type
// 'int'". That report is quoted in the recipe's Trap; the guarded path is
// what this file runs.
#include <cassert>
#include <cmath>
#include <limits>
#include <optional>

// --8<-- [start:recipe-52]
// "Round it" is five different questions, and C# has a name for each.
double to_nearest_even(double v) { return std::nearbyint(v); }  // Math.Round(v)
double to_nearest_away(double v) { return std::round(v); }      // Math.Round(v, AwayFromZero)
double down_always(double v)     { return std::floor(v); }      // Math.Floor(v)
double up_always(double v)       { return std::ceil(v); }       // Math.Ceiling(v)
double toward_zero(double v)     { return std::trunc(v); }      // Math.Truncate(v)

// And the conversion, which is where the undefined behavior lives: a double
// that does not fit in an int is UB to cast - not wrapped, not clamped, not
// an exception. The bounds compare exactly because INT_MIN and INT_MAX are
// both powers of two either side, representable in a double to the bit.
std::optional<int> to_int(double value) {
    if (!std::isfinite(value)) {                   // NaN and the infinities
        return std::nullopt;
    }
    const double whole = std::trunc(value);        // toward zero, like C#'s (int)
    if (whole < static_cast<double>(std::numeric_limits<int>::min())
        || whole > static_cast<double>(std::numeric_limits<int>::max())) {
        return std::nullopt;
    }
    return static_cast<int>(whole);
}
// --8<-- [end:recipe-52]

int main() {
    // The halfway rows, which are the whole reason this recipe exists.
    // Math.Round(2.5) is 2 in C#; std::round(2.5) is 3. They are different
    // questions with similar names, and only one of them is "Math.Round".
    assert(to_nearest_even(2.5) == 2.0);
    assert(to_nearest_even(3.5) == 4.0);
    assert(to_nearest_even(-2.5) == -2.0);
    assert(to_nearest_away(2.5) == 3.0);
    assert(to_nearest_away(3.5) == 4.0);
    assert(to_nearest_away(-2.5) == -3.0);

    // to_nearest_even(-0.5) is negative zero: it compares equal to 0.0 and
    // prints as "-0". Worth knowing before it reaches a user's screen.
    assert(to_nearest_even(-0.5) == 0.0);
    assert(std::signbit(to_nearest_even(-0.5)));

    // The three that never round to nearest, and never disagree with C#.
    assert(down_always(2.7) == 2.0 && down_always(-2.7) == -3.0);
    assert(up_always(2.1) == 3.0 && up_always(-2.1) == -2.0);
    assert(toward_zero(2.7) == 2.0 && toward_zero(-2.7) == -2.0);

    // A cast truncates toward zero, so it agrees with trunc and disagrees
    // with floor on every negative value - the off-by-one that reaches a
    // report and is blamed on the data.
    assert(static_cast<int>(-2.7) == -2);
    assert(down_always(-2.7) == -3.0);

    // The conversion, both ways.
    assert(to_int(2.7) == 2);
    assert(to_int(-2.7) == -2);
    assert(to_int(static_cast<double>(std::numeric_limits<int>::max())) == std::numeric_limits<int>::max());
    assert(to_int(static_cast<double>(std::numeric_limits<int>::min())) == std::numeric_limits<int>::min());
    assert(to_int(1e20) == std::nullopt);
    assert(to_int(-1e20) == std::nullopt);
    assert(to_int(std::numeric_limits<double>::quiet_NaN()) == std::nullopt);
    assert(to_int(std::numeric_limits<double>::infinity()) == std::nullopt);

    // Rounding first, then converting, is two decisions and not one - which
    // is the shape to write when a total must match a report to the penny.
    assert(to_int(to_nearest_away(2.5)) == 3);
    assert(to_int(to_nearest_even(2.5)) == 2);
    return 0;
}
