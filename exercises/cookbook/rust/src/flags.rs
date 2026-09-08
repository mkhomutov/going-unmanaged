//! Appendix F, Recipes 31 and 32 - a feature flag read once, and flags
//! combined as a type. Included by book/F-rosetta-cookbook.md between the
//! recipe-N markers: edit here and the page follows. The tests are
//! scaffolding and appear in no listing.

// --8<-- [start:recipe-31]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Features {
    pub audit: bool,         // the defaults ARE the off state
    pub fast_path: bool,
    pub batch_size: usize,
}

impl Default for Features {
    fn default() -> Self {
        Self { audit: false, fast_path: false, batch_size: 64 }
    }
}

impl Features {
    // One source among several. Whatever the source, it is read HERE, once, and never again.
    pub fn from_environment() -> Self {
        let flag = |name: &str| std::env::var(name).map(|v| v == "1").unwrap_or(false);
        let mut f = Self { audit: flag("MYPLUGIN_AUDIT"), fast_path: flag("MYPLUGIN_FAST_PATH"), ..Self::default() };
        if let Ok(size) = std::env::var("MYPLUGIN_BATCH_SIZE") {
            f.batch_size = size.parse().unwrap_or(f.batch_size);    // junk: batch_size stays 64 (Recipe 19)
        }
        f
    }
}

pub struct Processor {
    features: Features,    // read once, kept as a field
}

impl Processor {
    pub fn new(features: Features) -> Self {
        Self { features }
    }

    pub fn process(&self, sample: i32) -> i32 {
        if self.features.fast_path { sample } else { sample * 2 }    // a branch: free, even on the deadline path
    }
}
// --8<-- [end:recipe-31]

// --8<-- [start:recipe-32]
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Channel(u8);    // [Flags] enum Channel: a newtype over the bits, not an enum

impl Channel {
    pub const NONE: Channel = Channel(0);
    pub const LEFT: Channel = Channel(1);
    pub const RIGHT: Channel = Channel(2);
    pub const SUB: Channel = Channel(4);

    pub const fn has(self, flag: Channel) -> bool {
        self.0 & flag.0 == flag.0    // set.HasFlag(flag)
    }
}

impl std::ops::BitOr for Channel {
    type Output = Channel;
    fn bitor(self, rhs: Channel) -> Channel {
        Channel(self.0 | rhs.0)
    }
}

impl std::ops::BitAnd for Channel {
    type Output = Channel;
    fn bitand(self, rhs: Channel) -> Channel {
        Channel(self.0 & rhs.0)
    }
}
// --8<-- [end:recipe-32]

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn flags_read_once_and_junk_keeps_the_default() {
        std::env::set_var("MYPLUGIN_FAST_PATH", "1");
        std::env::set_var("MYPLUGIN_BATCH_SIZE", "lots");
        let features = Features::from_environment();
        assert_eq!(features, Features { audit: false, fast_path: true, batch_size: 64 });
        let processor = Processor::new(features);
        std::env::set_var("MYPLUGIN_FAST_PATH", "0");    // changed after the read: no effect
        assert_eq!(processor.process(21), 21);
        assert_eq!(Processor::new(Features::default()).process(21), 42);
    }

    #[test]
    fn bits_combine_and_test() {
        let stereo = Channel::LEFT | Channel::RIGHT;
        assert!(stereo.has(Channel::LEFT) && stereo.has(Channel::RIGHT) && !stereo.has(Channel::SUB));
        assert_eq!(stereo & Channel::SUB, Channel::NONE);
        assert!(Channel::NONE.has(Channel::NONE));    // HasFlag(None) is true in C# too
    }
}
