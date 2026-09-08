//! Appendix F, Recipe 8 - look up a key without inserting it. Included by
//! book/F-rosetta-cookbook.md between the recipe-N markers: edit here and
//! the page follows. The tests are scaffolding and appear in no listing.
use std::collections::BTreeMap;

// --8<-- [start:recipe-8]
pub fn apply_timeout_setting(settings: &BTreeMap<String, i32>, apply_timeout: impl FnOnce(i32)) {
    if let Some(&timeout) = settings.get("timeout") {    // get never inserts; entry() is the insert
        apply_timeout(timeout);
    }
}
// --8<-- [end:recipe-8]

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn found_applies_and_missing_leaves_the_map_alone() {
        let mut settings = BTreeMap::new();
        settings.insert("retries".to_string(), 3);
        let mut applied = None;
        apply_timeout_setting(&settings, |t| applied = Some(t));
        assert_eq!(applied, None);
        assert_eq!(settings.len(), 1);    // the C++ operator[] trap: a miss must not insert
        settings.insert("timeout".to_string(), 30);
        apply_timeout_setting(&settings, |t| applied = Some(t));
        assert_eq!(applied, Some(30));
    }
}

// --8<-- [start:recipe-18]
pub fn index_of<T: PartialEq>(values: &[T], wanted: &T) -> Option<usize> {
    values.iter().position(|v| v == wanted)    // "not found" is None, not a sentinel
}

pub fn contains_word(text: &str, word: &str) -> bool {
    text.contains(word)    // a str says it as bool; find() would say Option<usize>
}
// --8<-- [end:recipe-18]

#[cfg(test)]
mod find_tests {
    use super::*;

    #[test]
    fn index_and_substring() {
        assert_eq!(index_of(&[4, 8, 15], &15), Some(2));
        assert_eq!(index_of(&[4, 8, 15], &16), None);
        assert!(contains_word("no signal on channel 3", "signal"));
        assert!(!contains_word("no signal on channel 3", "noise"));
    }
}
