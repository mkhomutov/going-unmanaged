//! Appendix F, Recipe 52 - round a number, and turn it into an integer.
//!
//! Two of the C++ side's hazards are not hazards here: the conversion
//! saturates instead of being undefined, and the ties-to-even spelling has a
//! name of its own rather than depending on the process's rounding mode.

// --8<-- [start:recipe-52]
// The same five questions, and Rust names four of them the same way.
pub fn to_nearest_even(v: f64) -> f64 { v.round_ties_even() }   // Math.Round(v)
pub fn to_nearest_away(v: f64) -> f64 { v.round() }             // Math.Round(v, AwayFromZero)
pub fn down_always(v: f64) -> f64 { v.floor() }
pub fn up_always(v: f64) -> f64 { v.ceil() }
pub fn toward_zero(v: f64) -> f64 { v.trunc() }

// The conversion needs no guard: `as` saturates at the target's bounds and
// maps NaN to zero, all defined. The guard is still worth writing when
// "it did not fit" is a different outcome from "it was very large" - which
// in a plug-in reading a user's number it usually is.
pub fn to_int(value: f64) -> Option<i32> {
    if !value.is_finite() {
        return None;
    }
    let whole = value.trunc();
    if whole < i32::MIN as f64 || whole > i32::MAX as f64 {
        return None;
    }
    Some(whole as i32)
}
// --8<-- [end:recipe-52]

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_halfway_rows() {
        assert_eq!(to_nearest_even(2.5), 2.0);
        assert_eq!(to_nearest_even(-2.5), -2.0);
        assert_eq!(to_nearest_away(2.5), 3.0);
        assert_eq!(to_nearest_away(-2.5), -3.0);
    }

    #[test]
    fn the_three_that_never_round_to_nearest() {
        assert_eq!(down_always(-2.7), -3.0);
        assert_eq!(up_always(-2.1), -2.0);
        assert_eq!(toward_zero(-2.7), -2.0);
    }

    #[test]
    fn the_conversion_and_what_as_does_without_it() {
        assert_eq!(to_int(-2.7), Some(-2));
        assert_eq!(to_int(1e20), None);
        assert_eq!(to_int(f64::NAN), None);
        // Unguarded, `as` is defined where C++'s cast is undefined: it
        // saturates rather than reaching for a value that is not there.
        assert_eq!(1e20_f64 as i32, i32::MAX);
        assert_eq!(-1e20_f64 as i32, i32::MIN);
        assert_eq!(f64::NAN as i32, 0);
    }
}
