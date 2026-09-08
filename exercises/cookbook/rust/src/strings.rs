//! Appendix F, Recipes 2-5 - split, join, build in a loop, format. Included
//! by book/F-rosetta-cookbook.md between the recipe-N markers: edit here and
//! the page follows. The tests are scaffolding and appear in no listing.

// --8<-- [start:recipe-2]
pub fn split(text: &str, sep: char) -> Vec<String> {
    text.split(sep).map(str::to_owned).collect()    // an iterator of &str; collect owns
}
// --8<-- [end:recipe-2]

// --8<-- [start:recipe-3]
pub fn join(parts: &[String], sep: &str) -> String {
    parts.join(sep)    // between elements only - never leading
}
// --8<-- [end:recipe-3]

// --8<-- [start:recipe-4]
pub fn build_report(values: &[i32]) -> String {
    use std::fmt::Write;
    // one allocation up front - the StringBuilder(capacity) constructor
    let mut out = String::with_capacity(values.len() * 12);
    for value in values {
        writeln!(out, "value={value}").unwrap();    // writing to a String cannot fail
    }
    out
}
// --8<-- [end:recipe-4]

// --8<-- [start:recipe-5]
pub fn describe(count: i32, ratio: f64) -> String {
    format!("{count} samples, ratio {ratio:.2}")    // one form; type-checked at compile time
}
// --8<-- [end:recipe-5]

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn split_keeps_empty_fields_and_join_puts_separators_between() {
        assert_eq!(split("a,b,,c", ','), vec!["a", "b", "", "c"]);
        assert_eq!(split("", ','), vec![""]);    // C#'s Split does the same
        let parts: Vec<String> = ["x", "y", "z"].iter().map(|s| s.to_string()).collect();
        assert_eq!(join(&parts, ", "), "x, y, z");
        assert_eq!(join(&[], ", "), "");
    }

    #[test]
    fn report_and_describe_match_the_cpp_output() {
        assert_eq!(build_report(&[1, 22]), "value=1\nvalue=22\n");
        assert_eq!(describe(3, 0.6667), "3 samples, ratio 0.67");
    }
}
