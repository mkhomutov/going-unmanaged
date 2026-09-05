// Appendix F, Recipe 40 - notice a file changed.
//
// FileWatcher below is quoted VERBATIM in book/F-rosetta-cookbook.md: editing
// it means editing the appendix in the same commit (the testlab discipline).
// main() is scaffolding - it asserts that an unchanged file raises nothing,
// that a rewrite is noticed within a bounded wait (a DEADLINE, never an
// unbounded get - Chapter 38's judge, since a watcher that never fires would
// otherwise hang CI), that a file restored with an OLDER timestamp is still
// noticed (the != claim: a > would miss it, and the mutant fails here), that
// deletion is a change, and that no callback arrives after the destructor's
// join. The rewrite changes the size on purpose, so the assertion does not
// depend on the filesystem's timestamp resolution - the trap the recipe
// names and no harness can portably assert.
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
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
            s.size = std::filesystem::file_size(p, ec);
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

        // A rewrite with a different size is noticed - the size is what makes
        // this independent of the timestamp's resolution.
        write(watched, "bb");
        assert(reached(changes, 1, kDeadline));

        // A restored backup: same size, OLDER time. Compared with != it is a
        // change; compared with > it would be missed. The mutant fails here.
        const auto older = fs::last_write_time(watched) - 24h;
        fs::last_write_time(watched, older);
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

    fs::remove(watched);
    std::cout << "file watcher ok: 3 changes noticed, none after the join\n";
    return 0;
}
