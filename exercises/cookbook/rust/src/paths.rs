//! Appendix F, Recipes 10-12 - build a path, the exists pair, list a
//! directory. Included by book/F-rosetta-cookbook.md between the recipe-N
//! markers: edit here and the page follows. The tests are scaffolding and
//! appear in no listing.
use std::path::{Path, PathBuf};

// --8<-- [start:recipe-10]
pub fn log_path(dir: &Path) -> PathBuf {
    dir.join("logs").join("app.txt")    // join inserts the platform's separator
}
// --8<-- [end:recipe-10]

// --8<-- [start:recipe-11]
pub fn config_present(p: &Path) -> bool {
    p.is_file()    // File.Exists: it exists AND is a file
}

pub fn logs_dir_present(p: &Path) -> bool {
    p.is_dir()     // Directory.Exists: exists AND is a directory
}
// --8<-- [end:recipe-11]

// --8<-- [start:recipe-12]
pub fn list_files(dir: &Path) -> std::io::Result<Vec<PathBuf>> {
    let mut files = Vec::new();
    for entry in std::fs::read_dir(dir)? {    // the directory itself may be unreadable
        let entry = entry?;                   // and so may any one entry
        if entry.file_type()?.is_file() {
            files.push(entry.path());
        }
    }
    Ok(files)
}
// --8<-- [end:recipe-12]

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn paths_join_exist_and_list() {
        let root = std::env::temp_dir().join(format!("cookbook-rs-paths-{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&root);
        std::fs::create_dir_all(root.join("logs")).unwrap();
        assert_eq!(log_path(&root), root.join("logs").join("app.txt"));
        assert!(logs_dir_present(&root.join("logs")));
        assert!(!config_present(&root.join("logs")));    // a directory is not a file
        std::fs::write(root.join("a.txt"), "a").unwrap();
        std::fs::write(root.join("b.txt"), "b").unwrap();
        let mut names: Vec<_> = list_files(&root).unwrap().iter().map(|p| p.file_name().unwrap().to_owned()).collect();
        names.sort();
        assert_eq!(names, ["a.txt", "b.txt"]);    // "logs" is a directory and is not listed
        assert!(list_files(&root.join("missing")).is_err());
        std::fs::remove_dir_all(&root).unwrap();
    }
}
