// Appendix F, Recipe 40 - notice a file changed.
//
// FileWatcher below is quoted VERBATIM in book/F-rosetta-cookbook.md: editing
// it means editing the appendix in the same commit (the testlab discipline).
// main() is scaffolding, and every claim the recipe makes has a line here
// that fails without it (each was run as a mutant): an unchanged file raises
// nothing; a rewrite of a different size with the SAME timestamp is noticed
// (so the size field is load-bearing, not the time - the coarse-timestamp
// trap the recipe names, staged by hand since no harness can wait for a
// filesystem to be slow); a file restored with an OLDER timestamp is noticed
// (the != claim: a > sleeps through it); deletion is a change; a change made
// while the destructor is stopping the worker is NOT delivered (the stop
// check after the sleep, which "no callback after the join" alone cannot
// see); a file created and deleted thousands of times under a 1 ms poll
// never throws on the worker (the error_code overloads: absence is a state);
// and nothing arrives after the join. Every wait is a DEADLINE, never an
// unbounded get - Chapter 38's judge, since a watcher that never fires would
// otherwise hang CI rather than fail it. Changes are delivered by Recipe
// 38's rename, because a truncate-then-write can be polled mid-way and
// counted twice - measured, not imagined.
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <system_error>
#include <thread>
#include <utility>

// Recipe 40 - FileSystemWatcher, in the one spelling the standard library has
class FileWatcher {
public:
    FileWatcher(std::filesystem::path path, std::chrono::milliseconds interval,
                std::function<void()> on_change)
        : path_(std::move(path)),
          seen_(Snapshot(path_)),
          worker_([this, interval, on_change = std::move(on_change)] {
              while (!stop_) {
                  std::this_thread::sleep_for(interval);     // Recipe 16: a thread you own, blocked
                  if (stop_) {
                      break;
                  }
                  const Stamp now = Snapshot(path_);
                  if (now != seen_) {                        // !=, never >: a restored backup is OLDER
                      seen_ = now;
                      on_change();                           // on THIS thread - Chapter 29's rules apply
                  }
              }
          }) {}

    ~FileWatcher() {
        stop_ = true;
        worker_.join();    // Chapter 29's obligation, and the promise that no callback follows
    }
    FileWatcher(const FileWatcher&) = delete;
    FileWatcher& operator=(const FileWatcher&) = delete;

private:
    // What "changed" means to a poll: the time, the size, and whether it is
    // there at all. Absence is a state (Chapter 8's error_code overloads),
    // not an exception on the watcher's thread.
    struct Stamp {
        std::filesystem::file_time_type written{};
        std::uintmax_t size = 0;
        bool exists = false;
        bool operator!=(const Stamp& o) const {
            return written != o.written || size != o.size || exists != o.exists;
        }
    };
    static Stamp Snapshot(const std::filesystem::path& p) {
        std::error_code ec;
        Stamp s;
        s.exists = std::filesystem::is_regular_file(p, ec);
        if (s.exists) {
            s.written = std::filesystem::last_write_time(p, ec);
            if (!ec) {
                s.size = std::filesystem::file_size(p, ec);
            }
            if (ec) {
                s = Stamp{};    // it went away between the calls: absent, not a phantom of min() and -1
            }
        }
        return s;
    }

    std::filesystem::path path_;
    Stamp seen_;                        // the worker's alone once it starts
    std::atomic<bool> stop_{false};     // declared before worker_: initialized first (Recipe 16)
    std::thread worker_;
};

namespace fs = std::filesystem;
using namespace std::chrono_literals;

static void write(const fs::path& p, const std::string& text) {
    std::ofstream out(p, std::ios::binary);
    out << text;
    out.flush();
}

// Recipe 38: write beside, then rename over - so the watcher never polls a
// torn file, and a same-timestamp swap is possible at all.
static void replace_with(const fs::path& p, const std::string& text,
                         std::optional<fs::file_time_type> stamp = std::nullopt) {
    fs::path tmp = p;
    tmp += ".staged";
    write(tmp, text);
    if (stamp) {
        fs::last_write_time(tmp, *stamp);
    }
    fs::rename(tmp, p);
}

// Wait, with a deadline, for the counter to reach n. Returns whether it did.
static bool reached(const std::atomic<int>& counter, int n, std::chrono::milliseconds deadline) {
    const auto until = std::chrono::steady_clock::now() + deadline;
    while (counter.load() < n) {
        if (std::chrono::steady_clock::now() > until) {
            return false;
        }
        std::this_thread::sleep_for(5ms);
    }
    return true;
}

int main() {
    const fs::path watched = fs::temp_directory_path() / "cookbook_watched.txt";
    fs::remove(watched);
    write(watched, "a");

    std::atomic<int> changes{0};
    constexpr auto kInterval = 20ms;
    constexpr auto kDeadline = 5000ms;    // generous: the claim is arrival, not latency
    {
        FileWatcher watcher(watched, kInterval, [&changes] { ++changes; });

        // Nothing changed: nothing fires, however many polls pass.
        std::this_thread::sleep_for(10 * kInterval);
        assert(changes == 0);

        // A different size with the SAME timestamp: only the size field can
        // see this one. Delete size from Stamp and this is the line that fails.
        replace_with(watched, "bb", fs::last_write_time(watched));
        assert(reached(changes, 1, kDeadline));

        // A restored backup: same size, OLDER time. Compared with != it is a
        // change; compared with > it would be missed. The > mutant fails here.
        fs::last_write_time(watched, fs::last_write_time(watched) - 24h);
        assert(reached(changes, 2, kDeadline));

        // Deletion is a change too: the file's absence is a state.
        fs::remove(watched);
        assert(reached(changes, 3, kDeadline));

        // And it does not fire again for staying absent.
        std::this_thread::sleep_for(10 * kInterval);
        assert(changes == 3);
    }   // the destructor joins: no callback after this brace, whatever the file does

    write(watched, "after the join");
    std::this_thread::sleep_for(10 * kInterval);
    assert(changes == 3);

    // A change made while the watcher is being destroyed is not delivered:
    // the worker checks stop_ AFTER its sleep, before it polls. With a long
    // interval the worker is provably asleep when the change lands, so a
    // watcher missing that check fires once more, and this assertion sees it.
    std::atomic<int> late{0};
    {
        FileWatcher watcher(watched, 100ms, [&late] { ++late; });
        replace_with(watched, "one change, seen");
        assert(reached(late, 1, kDeadline));          // the worker has just polled and is asleep
        replace_with(watched, "a second, never seen");
    }                                                 // stop_, then join
    assert(late == 1);

    // Absence mid-poll is a state, not an exception on the worker's thread:
    // a file that appears and vanishes thousands of times under a 1 ms poll
    // lands inside the gap between is_regular_file and last_write_time often
    // enough that the throwing overloads would terminate the program.
    const fs::path flicker = fs::temp_directory_path() / "cookbook_flicker.txt";
    fs::remove(flicker);
    std::atomic<int> flickers{0};
    {
        FileWatcher watcher(flicker, 1ms, [&flickers] { ++flickers; });
        for (int i = 0; i < 3000; ++i) {
            write(flicker, "x");
            fs::remove(flicker);
        }
    }
    assert(flickers >= 1);                            // it was there, sometimes, and the worker lived to say so

    fs::remove(watched);
    std::cout << "file watcher ok: 3 changes noticed, none after the join, none while stopping, "
              << flickers << " flickers survived\n";
    return 0;
}
