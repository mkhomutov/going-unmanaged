//! Appendix F, Recipes 19 and 20 - a value that may be absent, and a value
//! that is one of several kinds. Included by book/F-rosetta-cookbook.md
//! between the recipe-N markers: edit here and the page follows. The tests
//! are scaffolding and appear in no listing.

// --8<-- [start:recipe-19]
pub fn parse_port(text: &str) -> Option<u16> {
    text.parse::<u16>().ok()    // not a port: absence, not an error - u16 already means 0..=65535
}

pub fn port_or_default(port: Option<u16>) -> u16 {
    port.unwrap_or(8080)    // the ?? operator
}

pub fn digits_in(text: Option<&str>) -> Option<usize> {
    text.map(str::len)    // the ?. operator: map runs only when there is something to map
}
// --8<-- [end:recipe-19]

// --8<-- [start:recipe-20]
pub enum Event {
    Temperature { centi: i32 },    // centi-degrees, as the wire carries them
    Fault { code: i32 },
    Heartbeat,
}

pub fn describe(e: &Event) -> String {
    match e {    // exhaustive: a fourth kind is a compile error here, not a fall-through
        Event::Temperature { centi } => format!("temperature {centi} centi-degrees"),
        Event::Fault { code } => format!("fault {code}"),
        Event::Heartbeat => "heartbeat".to_string(),
    }
}
// --8<-- [end:recipe-20]

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn optional_values() {
        assert_eq!(parse_port("8080"), Some(8080));
        assert_eq!(parse_port("80x"), None);
        assert_eq!(parse_port("70000"), None);
        assert_eq!(parse_port("-1"), None);
        assert_eq!(port_or_default(None), 8080);
        assert_eq!(port_or_default(Some(443)), 443);
        assert_eq!(digits_in(Some("12345")), Some(5));
        assert_eq!(digits_in(None), None);
    }

    #[test]
    fn every_kind_describes_itself() {
        assert_eq!(describe(&Event::Temperature { centi: 2150 }), "temperature 2150 centi-degrees");
        assert_eq!(describe(&Event::Fault { code: 7 }), "fault 7");
        assert_eq!(describe(&Event::Heartbeat), "heartbeat");
    }
}
