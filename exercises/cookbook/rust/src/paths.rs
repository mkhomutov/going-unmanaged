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

// --8<-- [start:recipe-39]
pub fn rotate_export(export_dir: &Path, fresh_report: &Path) -> std::io::Result<()> {
    std::fs::create_dir_all(export_dir.join("archive"))?;    // parents included; already there is not an error
    let current = export_dir.join("report.txt");
    if current.exists() {
        std::fs::copy(&current, export_dir.join("archive").join("previous.txt"))?;    // copy overwrites; C#'s default refuses
    }
    std::fs::rename(fresh_report, &current)    // File.Move onto the name - which REPLACES here, and throws in C#
}

pub fn purge(dir: &Path) -> std::io::Result<()> {
    match std::fs::remove_dir_all(dir) {    // Directory.Delete(recursive: true)
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => Ok(()),    // nothing there is not a failure
        other => other,
    }
}
// --8<-- [end:recipe-39]

#[cfg(test)]
mod rotate_tests {
    use super::*;

    #[test]
    fn rotation_archives_the_previous_report_and_purge_tolerates_absence() {
        let root = std::env::temp_dir().join(format!("cookbook-rs-rotate-{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&root);
        std::fs::create_dir_all(&root).unwrap();
        let export = root.join("export");
        std::fs::create_dir_all(&export).unwrap();
        std::fs::write(root.join("r1.txt"), "one").unwrap();
        rotate_export(&export, &root.join("r1.txt")).unwrap();    // no archive dir yet: created
        std::fs::write(root.join("r2.txt"), "two").unwrap();
        rotate_export(&export, &root.join("r2.txt")).unwrap();
        assert_eq!(std::fs::read_to_string(export.join("report.txt")).unwrap(), "two");
        assert_eq!(std::fs::read_to_string(export.join("archive").join("previous.txt")).unwrap(), "one");
        assert!(!root.join("r2.txt").exists());    // moved, not copied
        purge(&root).unwrap();
        purge(&root).unwrap();    // already gone: still Ok
        assert!(!root.exists());
    }
}
