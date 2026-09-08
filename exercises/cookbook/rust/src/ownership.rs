//! Appendix F, Recipes 33 and 34 - hold an owned object as a field, and an
//! object too big for the stack. Included by book/F-rosetta-cookbook.md
//! between the recipe-N markers: edit here and the page follows. The tests
//! are scaffolding and appear in no listing.
use std::cell::RefCell;
use std::rc::Rc;

// --8<-- [start:recipe-33]
pub trait Log {                              // polymorphic: lives behind a Box (Chapter 2)
    fn write(&mut self, line: &str);
}

#[derive(Default)]
pub struct Sink {                            // shared with a callback: co-owned (Chapter 29)
    pub samples: Vec<i32>,
}

pub struct Session {
    name: String,                            // by value: the field IS the object, and dies with the owner
    history: Vec<i32>,                       // by value too: its elements are on the heap, the field is three words
    log: Option<Box<dyn Log>>,               // one owner, polymorphic, optional: Option says "may be absent"
    sink: Rc<RefCell<Sink>>,                 // co-owned: alive while anyone still holds it; RefCell because it is mutated
}

impl Session {
    pub fn new(name: String, log: Option<Box<dyn Log>>, sink: Rc<RefCell<Sink>>) -> Self {
        Self { name, history: Vec::new(), log, sink }
    }

    pub fn record(&mut self, sample: i32) {
        self.history.push(sample);
        self.sink.borrow_mut().samples.push(sample);
        if let Some(log) = &mut self.log {   // the Option is where "may be absent" lives
            log.write(&format!("{}: recorded", self.name));
        }
    }
}   // no Dispose to write: the fields drop in declaration order, then the struct
// --8<-- [end:recipe-33]

// --8<-- [start:recipe-34]
pub struct FrameBuffer {
    pub pixels: Box<[u8]>,    // 4 MB on the heap; the struct itself is two words
}

pub fn make_frame() -> FrameBuffer {
    // vec! allocates the bytes directly on the heap. Box::new([0u8; N]) would
    // build the array on the stack first and then copy it - and 4 MB of stack
    // is the overflow Recipe 34 exists to avoid.
    FrameBuffer { pixels: vec![0u8; 4 * 1024 * 1024].into_boxed_slice() }
}
// --8<-- [end:recipe-34]

#[cfg(test)]
mod tests {
    use super::*;

    struct Recorder(Rc<RefCell<Vec<String>>>);
    impl Log for Recorder {
        fn write(&mut self, line: &str) {
            self.0.borrow_mut().push(line.to_string());
        }
    }

    #[test]
    fn fields_own_share_and_may_be_absent() {
        let lines = Rc::new(RefCell::new(Vec::new()));
        let sink = Rc::new(RefCell::new(Sink::default()));
        {
            let mut session = Session::new("s1".into(), Some(Box::new(Recorder(Rc::clone(&lines)))), Rc::clone(&sink));
            session.record(5);
            let mut quiet = Session::new("s2".into(), None, Rc::clone(&sink));
            quiet.record(6);    // no log: nothing to call, nothing to null-check
        }    // both sessions gone; the sink outlives them because this test still holds it
        assert_eq!(sink.borrow().samples, [5, 6]);
        assert_eq!(*lines.borrow(), ["s1: recorded"]);
        assert_eq!(Rc::strong_count(&sink), 1);
    }

    #[test]
    fn the_frame_lives_on_the_heap() {
        let frame = std::thread::Builder::new()
            .stack_size(256 * 1024)    // a small stack: a 4 MB stack temporary would overflow it
            .spawn(make_frame).unwrap().join().unwrap();
        assert_eq!(frame.pixels.len(), 4 * 1024 * 1024);
        assert_eq!(std::mem::size_of::<FrameBuffer>(), 2 * std::mem::size_of::<usize>());
    }
}
