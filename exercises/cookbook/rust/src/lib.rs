//! Appendix F, the Rust tab. Each module mirrors a cookbook translation unit
//! (`files.cpp` -> `files.rs`), each recipe sits between `recipe-N` section
//! markers the appendix includes, and each module's tests assert what the
//! recipe claims - the same judge shape as the C++ mains. Edit a recipe here
//! and the page follows.
pub mod async_work;
pub mod files;
pub mod handles;
pub mod lookups;
pub mod paths;
pub mod strings;
pub mod timing;
