//! Appendix F, Recipe 6 - time a call. Included by book/F-rosetta-cookbook.md
//! between the recipe-N markers: edit here and the page follows. The tests
//! are scaffolding and appear in no listing.
use std::time::{Duration, Instant};

// --8<-- [start:recipe-6]
pub fn report_batch_time(run_the_batch: impl FnOnce()) -> Duration {
    let start = Instant::now();    // monotonic, like steady_clock - never the wall clock
    run_the_batch();               // the code being timed
    let elapsed = start.elapsed();
    println!("{} ms", elapsed.as_millis());
    elapsed
}
// --8<-- [end:recipe-6]

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn measures_at_least_the_sleep() {
        let elapsed = report_batch_time(|| std::thread::sleep(Duration::from_millis(20)));
        assert!(elapsed >= Duration::from_millis(20));
    }
}

// --8<-- [start:recipe-16]
pub struct RepeatingTimer {
    stop: std::sync::Arc<std::sync::atomic::AtomicBool>,
    worker: Option<std::thread::JoinHandle<()>>,
}

impl RepeatingTimer {
    pub fn new(interval: Duration, mut tick: impl FnMut() + Send + 'static) -> Self {
        let stop = std::sync::Arc::new(std::sync::atomic::AtomicBool::new(false));
        let seen = std::sync::Arc::clone(&stop);
        let worker = std::thread::spawn(move || {
            while !seen.load(std::sync::atomic::Ordering::Relaxed) {
                // Task.Delay, spelled honestly: a thread you own, blocked.
                std::thread::sleep(interval);
                if !seen.load(std::sync::atomic::Ordering::Relaxed) {
                    tick();
                }
            }
        });
        Self { stop, worker: Some(worker) }
    }
}

impl Drop for RepeatingTimer {
    fn drop(&mut self) {
        self.stop.store(true, std::sync::atomic::Ordering::Relaxed);
        if let Some(worker) = self.worker.take() {
            let _ = worker.join();    // Chapter 29's obligation - and this join IS the Stop()
        }
    }
}
// --8<-- [end:recipe-16]

#[cfg(test)]
mod timer_tests {
    use super::*;
    use std::sync::atomic::{AtomicUsize, Ordering};
    use std::sync::Arc;

    #[test]
    fn ticks_while_alive_and_stops_when_dropped() {
        let ticks = Arc::new(AtomicUsize::new(0));
        let counter = Arc::clone(&ticks);
        {
            let _timer = RepeatingTimer::new(Duration::from_millis(5), move || {
                counter.fetch_add(1, Ordering::Relaxed);
            });
            std::thread::sleep(Duration::from_millis(60));
        }    // drop joins: no tick can run after this line
        let after_drop = ticks.load(Ordering::Relaxed);
        assert!(after_drop >= 2, "expected ticks, saw {after_drop}");
        std::thread::sleep(Duration::from_millis(30));
        assert_eq!(ticks.load(Ordering::Relaxed), after_drop);
    }
}

// --8<-- [start:recipe-28]
pub struct ScopedTimer<'a> {
    record: &'a mut Duration,
    start: Instant,
}

impl<'a> ScopedTimer<'a> {
    pub fn new(record: &'a mut Duration) -> Self {
        Self { record, start: Instant::now() }
    }
}

impl Drop for ScopedTimer<'_> {
    fn drop(&mut self) {
        *self.record = self.start.elapsed();    // return, ?, panic: every path out runs this
    }
}

pub fn time_call<R>(record: &mut Duration, f: impl FnOnce() -> R) -> R {
    let _timer = ScopedTimer::new(record);
    f()    // the result passes straight through; the timer records on the way out
}
// --8<-- [end:recipe-28]

// --8<-- [start:recipe-30]
// The vendor's declaration: a bare integer, the unit in the name. Passed in
// here so the recipe can be tested without the vendor; in a plug-in it is
// the extern "C" function itself.
pub type DeviceWait = unsafe extern "C" fn(timeout_ms: u32) -> std::os::raw::c_int;

pub fn wait_for_sample(timeout: Duration, device_wait: DeviceWait) -> std::os::raw::c_int {
    let ms = u32::try_from(timeout.as_millis()).unwrap_or(u32::MAX);    // the unit leaves the type HERE, and only here
    unsafe { device_wait(ms) }
}
// --8<-- [end:recipe-30]

#[cfg(test)]
mod scoped_tests {
    use super::*;

    #[test]
    fn records_on_every_exit_and_passes_the_result_through() {
        let mut took = Duration::ZERO;
        let answer = time_call(&mut took, || { std::thread::sleep(Duration::from_millis(5)); 42 });
        assert_eq!(answer, 42);
        assert!(took >= Duration::from_millis(5));
    }

    unsafe extern "C" fn fake_device_wait(timeout_ms: u32) -> std::os::raw::c_int {
        timeout_ms as std::os::raw::c_int    // echoes the integer the vendor would receive
    }

    #[test]
    fn the_unit_is_converted_once_at_the_boundary() {
        assert_eq!(wait_for_sample(Duration::from_secs(2), fake_device_wait), 2000);
        assert_eq!(wait_for_sample(Duration::from_secs(u64::MAX / 1000), fake_device_wait), u32::MAX as std::os::raw::c_int);
    }
}
