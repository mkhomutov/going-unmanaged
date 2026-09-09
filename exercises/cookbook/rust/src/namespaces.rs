//! Appendix F, Recipe 50 - keep a helper out of every other file.
//!
//! The C++ side needs two translation units to make its point, because
//! internal linkage is a fact about the linker. Rust has no linker question
//! to answer: privacy is the module system's, checked by the compiler, and
//! the default is private - so the recipe here is what you do NOT write.

// --8<-- [start:recipe-50]
pub mod reading {
    // No `pub`: private to this module, and the compiler - not the linker -
    // is what enforces it. `pub(crate)` is C#'s `internal`; bare `pub` is
    // public. Another module may define its own `clamp_to_range` freely.
    fn clamp_to_range(value: i32, low: i32, high: i32) -> i32 {
        value.clamp(low, high)
    }

    pub fn normalize(raw: i32) -> i32 {
        clamp_to_range(raw, 0, 100)
    }
}
// --8<-- [end:recipe-50]

pub mod other_reading {
    // The same name, a different rule, and nothing to arrange: two modules
    // are two namespaces, so this is not a collision to be avoided but the
    // ordinary case.
    fn clamp_to_range(value: i32, _low: i32, _high: i32) -> i32 {
        value
    }

    pub fn normalize(raw: i32) -> i32 {
        clamp_to_range(raw, 0, 100)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn each_module_reaches_its_own_helper() {
        assert_eq!(reading::normalize(150), 100);
        assert_eq!(reading::normalize(-5), 0);
        assert_eq!(other_reading::normalize(150), 150);
        assert_eq!(other_reading::normalize(-5), -5);
    }
}
