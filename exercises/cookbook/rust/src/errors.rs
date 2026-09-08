//! Appendix F, Recipes 21 and 22 - your own error type, and a value or an
//! error. Included by book/F-rosetta-cookbook.md between the recipe-N
//! markers: edit here and the page follows. The tests are scaffolding and
//! appear in no listing.
use std::fmt;

// --8<-- [start:recipe-21]
#[derive(Debug, Clone, PartialEq)]
pub struct ParseError {
    pub line: u32,    // the payload the message alone cannot carry
    pub what: String,
}

impl fmt::Display for ParseError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "line {}: {}", self.line, self.what)
    }
}

impl std::error::Error for ParseError {}    // so it can travel inside Box<dyn Error>

pub fn parse_channel_count(text: &str, line: u32) -> Result<i32, ParseError> {
    match text.parse::<i32>() {
        Ok(value) if value > 0 => Ok(value),
        _ => Err(ParseError { line, what: format!("channel count is not a number: '{text}'") }),
    }
}

pub fn channels_or_default(text: &str, line: u32) -> i32 {
    match parse_channel_count(text, line) {    // no catch: the failure is a value you match on
        Ok(value) => value,
        Err(e) => {
            eprintln!("{e}");    // Display is the what(); e.line is still there
            2
        }
    }
}
// --8<-- [end:recipe-21]

#[derive(Debug, PartialEq)]
pub struct ConfigError {
    pub line: u32,
    pub what: String,
}

#[derive(Debug, PartialEq)]
pub struct Config {
    pub channels: i32,
}

// --8<-- [start:recipe-22]
// The translation at the module's edge - and Result<T, E> is the standard
// library's type: nothing to write, only the two errors to map.
pub fn load_config(text: &str) -> Result<Config, ConfigError> {
    let channels = parse_channel_count(text, 1)
        .map_err(|e| ConfigError { line: e.line, what: e.to_string() })?;    // ? returns the Err
    Ok(Config { channels })
}
// --8<-- [end:recipe-22]

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parse_and_default() {
        assert_eq!(parse_channel_count("8", 3), Ok(8));
        let err = parse_channel_count("eight", 3).unwrap_err();
        assert_eq!((err.line, err.to_string()), (3, "line 3: channel count is not a number: 'eight'".to_string()));
        assert_eq!(channels_or_default("0", 4), 2);
        assert_eq!(channels_or_default("4", 4), 4);
    }

    #[test]
    fn a_failure_is_a_value() {
        assert_eq!(load_config("6"), Ok(Config { channels: 6 }));
        assert_eq!(load_config("six"), Err(ConfigError { line: 1, what: "line 1: channel count is not a number: 'six'".to_string() }));
    }
}
