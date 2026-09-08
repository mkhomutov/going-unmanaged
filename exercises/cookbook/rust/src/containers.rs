//! Appendix F, Recipe 27 - pre-size a collection. Included by
//! book/F-rosetta-cookbook.md between the recipe-N markers: edit here and
//! the page follows. The tests are scaffolding and appear in no listing.

// --8<-- [start:recipe-27]
pub fn read_samples(expected: usize, mut next_sample: impl FnMut() -> i32) -> Vec<i32> {
    let mut samples = Vec::with_capacity(expected);    // List<T>(capacity): room for expected, len still 0
    for _ in 0..expected {
        samples.push(next_sample());    // len grows; no reallocation until the room runs out
    }
    samples
}

pub fn zeroed(n: usize) -> Vec<f64> {
    vec![0.0; n]    // new double[n]: n elements, every one 0.0
}
// --8<-- [end:recipe-27]

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn capacity_is_not_length() {
        let mut next = 0;
        let samples = read_samples(3, || { next += 1; next });
        assert_eq!(samples, [1, 2, 3]);
        let reserved: Vec<i32> = Vec::with_capacity(8);
        assert_eq!((reserved.len(), reserved.capacity() >= 8), (0, true));    // the resize-vs-reserve trap
        assert_eq!(zeroed(4), [0.0, 0.0, 0.0, 0.0]);
    }
}
