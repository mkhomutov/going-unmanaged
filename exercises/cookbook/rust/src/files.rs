//! Appendix F, Recipes 1 and 9 - read and write a whole file. Included by
//! book/F-rosetta-cookbook.md between the recipe-N markers: edit here and
//! the page follows. The tests are scaffolding and appear in no listing.
use std::path::Path;

// --8<-- [start:recipe-1]
pub fn read_all_text(path: &Path) -> std::io::Result<String> {
    std::fs::read_to_string(path)    // one call; a missing file is an Err, not an empty string
}
// --8<-- [end:recipe-1]

// --8<-- [start:recipe-9]
pub fn write_all_text(path: &Path, text: &str) -> std::io::Result<()> {
    std::fs::write(path, text)    // create or truncate, write, close - every failure is the Err
}
// --8<-- [end:recipe-9]

#[cfg(test)]
mod tests {
    use super::*;

    fn temp_file(name: &str) -> std::path::PathBuf {
        let dir = std::env::temp_dir().join(format!("cookbook-rs-{}", std::process::id()));
        std::fs::create_dir_all(&dir).unwrap();
        dir.join(name)
    }

    #[test]
    fn round_trip_and_missing_file() {
        let path = temp_file("owned.txt");
        write_all_text(&path, "owned\n").unwrap();
        assert_eq!(read_all_text(&path).unwrap(), "owned\n");
        std::fs::remove_file(&path).unwrap();
        // Recipe 1's trap in C++ - a failed open reads as empty - cannot happen here:
        assert!(read_all_text(&path).is_err());
        assert!(write_all_text(Path::new("no_such_dir_xyz/f.txt"), "x").is_err());
    }
}
