//! Appendix F, Recipe 6 - time a call. Included by book/F-rosetta-cookbook.md
//! between the recipe-N markers: edit here and the page follows. The tests
//! are scaffolding and appear in no listing.
use std::time::{Duration, Instant};

// --8<-- [start:recipe-6]
pub fn report_batch_time(run_the_batch: impl FnOnce()) -> Duration {
    let start = Instant::now();    // monotonic, like steady_clock - never the wall clock
    run_the_batch();               // the code being timed
    let elapsed = start.elapsed();
    println!("{} ms", elapsed.as_millis());
    elapsed
}
// --8<-- [end:recipe-6]

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn measures_at_least_the_sleep() {
        let elapsed = report_batch_time(|| std::thread::sleep(Duration::from_millis(20)));
        assert!(elapsed >= Duration::from_millis(20));
    }
}
