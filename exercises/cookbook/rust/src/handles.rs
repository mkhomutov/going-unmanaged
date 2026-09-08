//! Appendix F, Recipe 7 - wrap a C handle so it frees itself. Included by
//! book/F-rosetta-cookbook.md between the recipe-N markers: edit here and
//! the page follows. The C functions are declared here rather than through
//! the libc crate so the crate stays dependency-free; std links libc anyway.
//! The tests are scaffolding and appear in no listing.
use std::ffi::CString;
use std::os::raw::{c_char, c_int, c_void};

extern "C" {
    fn fopen(path: *const c_char, mode: *const c_char) -> *mut c_void;
    fn fclose(stream: *mut c_void) -> c_int;
}

// --8<-- [start:recipe-7]
pub struct FileHandle(*mut c_void);    // the raw C handle, owned

impl Drop for FileHandle {
    fn drop(&mut self) {
        unsafe { fclose(self.0) };    // the destructor: runs on every path out of scope
    }
}

pub fn open_file(path: &str, mode: &str) -> Option<FileHandle> {
    let (path, mode) = (CString::new(path).ok()?, CString::new(mode).ok()?);
    let raw = unsafe { fopen(path.as_ptr(), mode.as_ptr()) };
    if raw.is_null() { None } else { Some(FileHandle(raw)) }    // a failed open is None; no Drop runs
}
// --8<-- [end:recipe-7]

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn scope_end_closes_and_failed_open_is_none() {
        let path = std::env::temp_dir().join(format!("cookbook-rs-handle-{}.txt", std::process::id()));
        let name = path.to_str().unwrap();
        {
            let file = open_file(name, "w");
            assert!(file.is_some());
        }    // scope end IS the fclose - the C# using block, as a type
        assert!(path.is_file());
        std::fs::remove_file(&path).unwrap();
        assert!(open_file("no_such_dir_xyz/f.txt", "r").is_none());
    }
}
