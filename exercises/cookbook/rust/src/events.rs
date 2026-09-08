//! Appendix F, Recipe 14 - expose an event. Included by
//! book/F-rosetta-cookbook.md between the recipe-N markers: edit here and
//! the page follows. The tests are scaffolding and appear in no listing.

// --8<-- [start:recipe-14]
pub struct SampleSource {
    handlers: Vec<(u32, Box<dyn FnMut(i32)>)>,
    next_id: u32,
}

impl SampleSource {
    pub fn new() -> Self {
        Self { handlers: Vec::new(), next_id: 0 }
    }

    pub fn subscribe(&mut self, handler: impl FnMut(i32) + 'static) -> u32 {
        self.handlers.push((self.next_id, Box::new(handler)));
        self.next_id += 1;
        self.next_id - 1    // the token is how -= works without delegate identity
    }

    pub fn unsubscribe(&mut self, id: u32) {
        self.handlers.retain(|(token, _)| *token != id);
    }

    pub fn raise(&mut self, sample: i32) {    // the ?.Invoke: an empty list is a zero-pass loop
        for (_, handler) in &mut self.handlers {
            handler(sample);
        }
    }
}

impl Default for SampleSource {
    fn default() -> Self {
        Self::new()
    }
}
// --8<-- [end:recipe-14]

#[cfg(test)]
mod tests {
    use super::*;
    use std::cell::RefCell;
    use std::rc::Rc;

    #[test]
    fn subscription_order_token_unsubscribe_and_raising_into_nothing() {
        let log = Rc::new(RefCell::new(Vec::new()));
        let mut source = SampleSource::new();
        source.raise(0);    // no subscribers: a no-op, not a null dereference
        let (a, b) = (Rc::clone(&log), Rc::clone(&log));
        let first = source.subscribe(move |s| a.borrow_mut().push(format!("first {s}")));
        let _second = source.subscribe(move |s| b.borrow_mut().push(format!("second {s}")));
        source.raise(7);
        source.unsubscribe(first);
        source.raise(8);
        assert_eq!(*log.borrow(), ["first 7", "second 7", "second 8"]);
    }
}
