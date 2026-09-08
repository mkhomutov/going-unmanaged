//! Appendix F, the Rust tab. Each module mirrors a cookbook translation unit
//! (`files.cpp` -> `files.rs`), each recipe sits between `recipe-N` section
//! markers the appendix includes, and each module's tests assert what the
//! recipe claims - the same judge shape as the C++ mains. Edit a recipe here
//! and the page follows.
pub mod alternatives;
pub mod async_work;
pub mod containers;
pub mod errors;
pub mod events;
pub mod files;
pub mod flags;
pub mod handles;
pub mod logging;
pub mod lookups;
pub mod ownership;
pub mod paths;
pub mod strings;
pub mod timing;
