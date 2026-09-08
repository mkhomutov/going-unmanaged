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

// --8<-- [start:recipe-38]
pub fn save_file(path: &Path, text: &str) -> std::io::Result<()> {
    let mut tmp = path.as_os_str().to_owned();
    tmp.push(".tmp");                       // a suffix, not a segment: same directory, same volume
    let tmp = std::path::PathBuf::from(tmp);
    std::fs::write(&tmp, text)?;            // Recipe 9: written and closed, or the ? returned and path is untouched
    std::fs::rename(&tmp, path)             // one atomic step: a reader sees the old file or the new, never half
}
// --8<-- [end:recipe-38]

#[cfg(test)]
mod save_tests {
    use super::*;

    #[test]
    fn a_save_replaces_whole_and_leaves_no_temp() {
        let dir = std::env::temp_dir().join(format!("cookbook-rs-save-{}", std::process::id()));
        std::fs::create_dir_all(&dir).unwrap();
        let path = dir.join("prefs.txt");
        save_file(&path, "old").unwrap();
        save_file(&path, "new").unwrap();
        assert_eq!(read_all_text(&path).unwrap(), "new");
        assert!(!dir.join("prefs.txt.tmp").exists());    // renamed away, not left behind
        std::fs::remove_dir_all(&dir).unwrap();
    }
}
