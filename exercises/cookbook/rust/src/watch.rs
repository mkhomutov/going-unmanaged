//! Appendix F, Recipe 40 - notice a file changed. Included by
//! book/F-rosetta-cookbook.md between the recipe-N markers: edit here and
//! the page follows. The tests are scaffolding and appear in no listing.
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;
use std::time::{Duration, SystemTime};

// --8<-- [start:recipe-40]
// What "changed" means to a poll: the time, the size, and whether it is
// there at all. Absence is a state, not a panic on the watcher's thread.
#[derive(Clone, Copy, PartialEq, Eq, Debug, Default)]
struct Stamp {
    written: Option<SystemTime>,
    size: u64,
    exists: bool,
}

fn snapshot(path: &Path) -> Stamp {
    match std::fs::metadata(path) {    // one call answers all three; an Err is "absent"
        Ok(meta) if meta.is_file() => Stamp { written: meta.modified().ok(), size: meta.len(), exists: true },
        _ => Stamp::default(),
    }
}

pub struct FileWatcher {
    stop: Arc<AtomicBool>,
    worker: Option<std::thread::JoinHandle<()>>,
}

impl FileWatcher {
    pub fn new(path: PathBuf, interval: Duration, mut on_change: impl FnMut() + Send + 'static) -> Self {
        let stop = Arc::new(AtomicBool::new(false));
        let seen_stop = Arc::clone(&stop);
        let worker = std::thread::spawn(move || {
            let mut seen = snapshot(&path);
            while !seen_stop.load(Ordering::Relaxed) {
                std::thread::sleep(interval);    // Recipe 16: a thread you own, blocked
                if seen_stop.load(Ordering::Relaxed) {
                    break;
                }
                let now = snapshot(&path);
                if now != seen {    // !=, never >: a restored backup is OLDER
                    seen = now;
                    on_change();    // on THIS thread - Chapter 29's rules apply
                }
            }
        });
        Self { stop, worker: Some(worker) }
    }
}

impl Drop for FileWatcher {
    fn drop(&mut self) {
        self.stop.store(true, Ordering::Relaxed);
        if let Some(worker) = self.worker.take() {
            let _ = worker.join();    // Chapter 29's obligation, and the promise that no callback follows
        }
    }
}
// --8<-- [end:recipe-40]

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::atomic::AtomicUsize;

    #[test]
    fn a_rewrite_and_a_removal_each_fire_once_and_nothing_fires_after_drop() {
        let path = std::env::temp_dir().join(format!("cookbook-rs-watch-{}.txt", std::process::id()));
        std::fs::write(&path, "v1").unwrap();
        let fired = Arc::new(AtomicUsize::new(0));
        let count = Arc::clone(&fired);
        {
            let _watcher = FileWatcher::new(path.clone(), Duration::from_millis(10), move || {
                count.fetch_add(1, Ordering::Relaxed);
            });
            std::thread::sleep(Duration::from_millis(40));
            assert_eq!(fired.load(Ordering::Relaxed), 0);    // an unchanged file raises nothing
            std::fs::write(&path, "v2 - longer").unwrap();    // size changes even if the clock did not tick
            std::thread::sleep(Duration::from_millis(60));
            assert_eq!(fired.load(Ordering::Relaxed), 1);
            std::fs::remove_file(&path).unwrap();
            std::thread::sleep(Duration::from_millis(60));
            assert_eq!(fired.load(Ordering::Relaxed), 2);    // absence is a change too
        }
        let after = fired.load(Ordering::Relaxed);
        std::fs::write(&path, "v3").unwrap();
        std::thread::sleep(Duration::from_millis(40));
        assert_eq!(fired.load(Ordering::Relaxed), after);    // dropped: no callback follows
        let _ = std::fs::remove_file(&path);
    }
}
