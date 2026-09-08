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
