//! Chapter 30, Technique 3, from the caller's side in Rust. The header is
//! not included - Rust cannot - so its three declarations are written a
//! second time here, which is Chapter 39's point about P/Invoke: nothing
//! compares the two, and the size and the types are the whole contract.
//! Included by book/30-authoring-an-abi-boundary.md between section markers:
//! edit here and the page follows. The tests are the judge and appear in no
//! listing; they are engine_demo.cpp's assertions, in Rust.
use std::os::raw::{c_int, c_void};

// --8<-- [start:declarations]
// engine.h, declared again. EngineHandle is an opaque pointer: *mut c_void
// says exactly what `struct EngineImpl*` with no definition says.
type EngineHandle = *mut c_void;

extern "C" {
    fn Engine_Create(seed: c_int, out: *mut EngineHandle) -> c_int;    // 0 = ok
    fn Engine_Score(h: EngineHandle, out_score: *mut c_int) -> c_int;
    fn Engine_Destroy(h: EngineHandle) -> c_int;
}
// --8<-- [end:declarations]

// --8<-- [start:wrapper]
/// Chapter 17's RAII wrapper, on this side of the boundary: the handle is
/// owned, Destroy runs exactly once, in Drop, and the error codes become the
/// Result the rest of the program speaks.
pub struct Engine(EngineHandle);

impl Engine {
    pub fn create(seed: i32) -> Result<Engine, i32> {
        let mut raw: EngineHandle = std::ptr::null_mut();
        match unsafe { Engine_Create(seed, &mut raw) } {    // unsafe: the compiler cannot check C's contract
            0 => Ok(Engine(raw)),
            rc => Err(rc),
        }
    }

    pub fn score(&self) -> Result<i32, i32> {
        let mut score: c_int = 0;
        match unsafe { Engine_Score(self.0, &mut score) } {
            0 => Ok(score),
            rc => Err(rc),
        }
    }
}

impl Drop for Engine {
    fn drop(&mut self) {
        unsafe { Engine_Destroy(self.0) };    // exactly once; a second call cannot be written
    }
}
// --8<-- [end:wrapper]

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_happy_path_and_the_documented_null_paths() {
        let engine = Engine::create(21).expect("create");
        assert_eq!(engine.score(), Ok(42));    // a value, not merely "it survived"
        // The null-parameter contract, called raw: 1, and nothing written.
        let mut score: c_int = 7;
        unsafe {
            assert_eq!(Engine_Create(21, std::ptr::null_mut()), 1);
            assert_eq!(Engine_Score(std::ptr::null_mut(), &mut score), 1);
            assert_eq!(Engine_Score(engine.0, std::ptr::null_mut()), 1);
            assert_eq!(Engine_Destroy(std::ptr::null_mut()), 1);
        }
        assert_eq!(score, 7);
    }    // drop: Engine_Destroy, once - the double free of a second call is unwritable
}
