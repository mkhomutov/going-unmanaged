//! Appendix F, Recipe 13 - run work on another thread and wait for it.
//! Included by book/F-rosetta-cookbook.md between the recipe-N markers: edit
//! here and the page follows. The tests are scaffolding and appear in no
//! listing.

// --8<-- [start:recipe-13]
pub fn overlap_work(count_defects: fn() -> i32, do_other_work: fn() -> i32) -> i32 {
    let task = std::thread::spawn(count_defects);    // starts now, on its own thread
    let other = do_other_work();                     // runs while count_defects runs
    other + task.join().expect("count_defects panicked")    // the await: blocks until the result arrives
}
// --8<-- [end:recipe-13]

#[cfg(test)]
mod tests {
    use super::*;

    fn count_defects() -> i32 {
        std::thread::sleep(std::time::Duration::from_millis(10));
        7
    }
    fn do_other_work() -> i32 {
        35
    }
    fn panics() -> i32 {
        panic!("boom")
    }

    #[test]
    fn sums_both_halves() {
        assert_eq!(overlap_work(count_defects, do_other_work), 42);
    }

    #[test]
    #[should_panic(expected = "count_defects panicked")]
    fn a_panic_inside_the_work_surfaces_at_join() {
        overlap_work(panics, do_other_work);    // the one C# behaviour that ports exactly
    }
}
