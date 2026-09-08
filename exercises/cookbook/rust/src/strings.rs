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

// --8<-- [start:recipe-17]
// UTF-8 -> UTF-16. A &str is valid UTF-8 by construction, so the only
// conversion left is the encoding; invalid BYTES are handled one step
// earlier, by from_utf8_lossy, which writes U+FFFD the way browsers do.
pub fn utf8_to_utf16(utf8: &str) -> Vec<u16> {
    utf8.encode_utf16().collect()
}

pub fn utf8_bytes_to_utf16(bytes: &[u8]) -> Vec<u16> {
    String::from_utf8_lossy(bytes).encode_utf16().collect()
}

// UTF-16 -> UTF-8. Lone surrogates become U+FFFD; everything else is mechanical.
pub fn utf16_to_utf8(utf16: &[u16]) -> String {
    String::from_utf16_lossy(utf16)
}
// --8<-- [end:recipe-17]

// --8<-- [start:recipe-23]
/// The one null there is: a C API's "no name". A &str cannot be null, so
/// the check lives at the boundary where the raw pointer arrives.
///
/// # Safety
/// `from_c_api` must be null or point at a NUL-terminated string that
/// outlives the call.
pub unsafe fn name_or_default(from_c_api: *const std::os::raw::c_char) -> String {
    if from_c_api.is_null() {
        return "unnamed".to_string();
    }
    std::ffi::CStr::from_ptr(from_c_api).to_string_lossy().into_owned()    // safe now
}
// --8<-- [end:recipe-23]

#[cfg(test)]
mod encoding_tests {
    use super::*;

    #[test]
    fn utf8_utf16_round_trips_and_repairs() {
        let units = utf8_to_utf16("h\u{e9}llo \u{1F600}");
        assert_eq!(units.len(), 8);    // e-acute is one unit, the emoji a surrogate pair
        assert_eq!(utf16_to_utf8(&units), "h\u{e9}llo \u{1F600}");
        assert_eq!(utf16_to_utf8(&[0x0041, 0xD800, 0x0042]), "A\u{FFFD}B");    // lone surrogate
        assert_eq!(utf8_bytes_to_utf16(&[0x41, 0xFF, 0x42]), utf8_to_utf16("A\u{FFFD}B"));    // stray byte
    }

    #[test]
    fn a_null_name_is_unnamed() {
        let owned = std::ffi::CString::new("probe").unwrap();
        unsafe {
            assert_eq!(name_or_default(owned.as_ptr()), "probe");
            assert_eq!(name_or_default(std::ptr::null()), "unnamed");
        }
    }
}

// --8<-- [start:recipe-44]
// The general answer is the regex crate, a dependency this crate does not
// take. This pattern - a fixed prefix and digits - does not need one: the
// standard library's strip_prefix and a digit check say it exactly.
pub fn sensor_index(id: &str) -> Option<u32> {
    let digits = id.strip_prefix("sensor")?;    // the anchor and the literal: None if absent
    if digits.is_empty() || !digits.bytes().all(|b| b.is_ascii_digit()) {
        return None;    // IsMatch false: absence, not an error
    }
    digits.parse().ok()    // matched, but more digits than a u32 holds: None as well
}

pub fn redact_digits(text: &str) -> String {
    let mut out = String::with_capacity(text.len());
    let mut in_run = false;
    for c in text.chars() {    // Regex.Replace("[0-9]+", "#"): every run of digits becomes one '#'
        match (c.is_ascii_digit(), in_run) {
            (true, false) => { out.push('#'); in_run = true; }
            (true, true) => {}
            (false, _) => { out.push(c); in_run = false; }
        }
    }
    out
}
// --8<-- [end:recipe-44]

// --8<-- [start:recipe-45]
pub fn trim(s: &str) -> &str {
    s.trim_matches(|c| c == ' ' || c == '\t' || c == '\r' || c == '\n')    // a &str INTO s: the borrow checker holds the lifetime
}

pub fn equals_ignore_case(a: &str, b: &str) -> bool {
    a.eq_ignore_ascii_case(b)    // ASCII only, and the name says so (Chapter 9)
}

pub fn starts_with(s: &str, prefix: &str) -> bool {
    s.starts_with(prefix)
}

pub fn ends_with(s: &str, suffix: &str) -> bool {
    s.ends_with(suffix)
}
// --8<-- [end:recipe-45]

#[cfg(test)]
mod text_tests {
    use super::*;

    #[test]
    fn pattern_without_a_regex() {
        assert_eq!(sensor_index("sensor12"), Some(12));
        assert_eq!(sensor_index("sensor"), None);
        assert_eq!(sensor_index("sensor12x"), None);
        assert_eq!(sensor_index("Sensor12"), None);
        assert_eq!(sensor_index("sensor99999999999"), None);
        assert_eq!(redact_digits("card 4111 1111, pin 07"), "card # #, pin #");
        assert_eq!(redact_digits("no digits"), "no digits");
    }

    #[test]
    fn trim_compare_prefix_suffix() {
        assert_eq!(trim("  \tname\r\n"), "name");
        assert_eq!(trim(" \n "), "");
        assert!(equals_ignore_case("Sensor", "sENSOR"));
        assert!(!equals_ignore_case("Sensor", "Sensors"));
        assert!(starts_with("sensor12", "sensor") && ends_with("report.txt", ".txt"));
        assert!(!ends_with("txt", "report.txt"));
    }
}
