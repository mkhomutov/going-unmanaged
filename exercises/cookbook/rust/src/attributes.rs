//! Appendix F, Recipe 51 - tell the compiler what a function promises.
//!
//! The C++ side needs three builds that must fail, because an attribute's
//! whole effect is on a diagnostic. Rust's answers are the same three ideas
//! with the enforcement moved: `#[must_use]` is `[[nodiscard]]`, the `!`
//! return type is `[[noreturn]]` written in the type system rather than
//! beside it, and `#[deprecated]` is the same attribute under the same name.

// --8<-- [start:recipe-51]
pub enum Status {
    Ok,
    Busy,
    Failed,
}

// `!` is the never type: this function has no return value because it does
// not return, and that is a fact about its TYPE, so the caller needs no
// `return` after it and the compiler needs no attribute to be told.
pub fn fatal(why: &str) -> ! {
    panic!("fatal: {why}");
}

// `#[must_use]` is `[[nodiscard]]`, and the lint is on by default.
#[must_use]
pub fn session_status(id: i32) -> Status {
    if id > 0 { Status::Ok } else { Status::Failed }
}

pub fn describe(status: &Status, _verbosity: i32) -> &'static str {
    // No fallthrough to say out loud: arms do not fall through, and the
    // `|` pattern is how two of them share a body. The match is exhaustive
    // or it does not compile, which is the other half of `[[fallthrough]]`'s
    // job done by the language.
    match status {
        Status::Ok => "ok",
        Status::Busy | Status::Failed => "not available",
    }
}

pub fn required_channel(configured: i32) -> i32 {
    if configured > 0 {
        return configured;
    }
    fatal("channel not configured - the caller's contract, broken")
}

#[deprecated(note = "use session_status")]
pub fn retire_session(id: i32) -> Status {
    session_status(id)
}
// --8<-- [end:recipe-51]

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_values_the_attributes_do_not_change() {
        assert!(matches!(session_status(1), Status::Ok));
        assert!(matches!(session_status(0), Status::Failed));
        assert_eq!(describe(&Status::Ok, 0), "ok");
        assert_eq!(describe(&Status::Busy, 0), "not available");
        assert_eq!(required_channel(3), 3);
    }

    #[test]
    #[should_panic(expected = "channel not configured")]
    fn the_never_type_really_does_not_return() {
        required_channel(0);
    }
}
