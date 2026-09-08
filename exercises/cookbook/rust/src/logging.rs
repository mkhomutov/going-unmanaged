//! Appendix F, Recipes 15 and 24 - print a diagnostic you will see; compile
//! one out of Release. Included by book/F-rosetta-cookbook.md between the
//! recipe-N markers: edit here and the page follows. The tests are
//! scaffolding and appear in no listing.

// --8<-- [start:recipe-15]
pub fn report_progress(done: usize, total: usize) {
    println!("processed {done} of {total}");    // stdout: line-buffered, and a terminal shows it
}

pub fn report_failure(what: &str) {
    eprintln!("error: {what}");    // stderr: unbuffered, survives a crash, separable in a pipe
}
// --8<-- [end:recipe-15]

// --8<-- [start:recipe-24]
pub fn check_channel_count(channels: i32) {
    debug_assert!(channels > 0, "a session has at least one channel");    // gone in --release
    if cfg!(debug_assertions) {
        eprintln!("[debug] channels={channels}");    // and so is this block: cfg! is a constant
    }
}
// --8<-- [end:recipe-24]

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn diagnostics_print_without_panicking() {
        report_progress(3, 10);
        report_failure("disk full");
        check_channel_count(2);
    }

    #[test]
    #[cfg(debug_assertions)]    // the assertion exists only where debug_assert! does
    #[should_panic(expected = "at least one channel")]
    fn zero_channels_is_refused_in_debug() {
        check_channel_count(0);
    }
}
