// Appendix F, Recipe 43 - share a buffer with another process.
//
// Frame and SharedRegion are quoted VERBATIM in book/F-rosetta-cookbook.md:
// editing either means editing the appendix in the same commit (the
// testlab discipline). main() is scaffolding. On POSIX it forks: the child
// writes a frame and bumps the sequence counter, the parent waits on the
// counter with a DEADLINE (Chapter 38's judge - a hang would stop CI, not
// fail it), asserts the fields, reaps the child, and unlinks; a second
// phase asserts that after shm_unlink the name is gone - the leak Appendix
// G names, demonstrated and then closed. On Windows the harness maps one
// region twice in ONE process and asserts a write through one view is
// visible through the other: the mechanism, with the cross-process claim
// stated as unverified there, as Chapter 40 does for its toolchain file.
// Two more judges, both scaffolding: a second create of the live name
// through the class must be refused, and by shm_open itself - on macOS a
// class without O_EXCL is refused by the second ftruncate instead, for the
// wrong reason, and then unlinks a name it never owned - and the lowest free descriptor
// is the same before and after the region lived, so a close left out of the
// destructor is seen. One failure shape differs by platform: MAP_PRIVATE on
// a shm object is refused by macOS's mmap outright, where Linux accepts it
// and the parent's deadline is what fails.
//
// No library: the platform is the dependency. Older glibc (< 2.34) needs
// -lrt for shm_open, which build_all.sh adds on Linux.
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>

#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

// Recipe 43 - MemoryMappedFile.CreateOrOpen + CreateViewAccessor

// What goes IN the region is a wire format (Chapter 34), not an object:
// fixed-width fields, a version first (Chapter 30), no pointers - an address
// means nothing in the other process - and nothing whose layout the
// compiler chose. The static_asserts are Chapter 41's judge on that claim.
struct Frame {
    std::uint32_t version;              // sizeof(Frame): a reader built against an older layout can tell
    std::atomic<std::uint32_t> seq;     // bumped by the writer AFTER the payload: the reader's "is it there yet"
    std::uint32_t width;
    std::uint32_t height;
    std::uint8_t  pixels[64];
};
static_assert(std::is_standard_layout<Frame>::value, "a shared layout has no vtable and no surprises");
static_assert(std::atomic<std::uint32_t>::is_always_lock_free,
              "a lock-based atomic holds a lock that exists in ONE process");
static_assert(sizeof(Frame) == 4 + 4 + 4 + 4 + 64, "the layout is the contract; a change here is a version bump");
// Not is_trivially_copyable: an atomic has no copy at all, and what the trait
// says about that differs by standard library. And none of the three refuses
// a pointer or a std::string - both are standard-layout - so that half of
// the rule is yours to keep; the asserts hold the size, the vtable, the lock.

// One name, one mapping, two handles - the object and the view - released
// in reverse on every path. Recipe 7's shape, twice, behind one class.
class SharedRegion {
public:
    SharedRegion(const std::string& name, std::size_t size, bool create)
        : size_(size) {
#if defined(_WIN32)
        SetLastError(0);
        mapping_ = create
            ? CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,   // INVALID_HANDLE_VALUE: the paging file backs it
                                 static_cast<DWORD>(size), name.c_str())
            : OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name.c_str());
        if (mapping_ == nullptr) {
            throw std::runtime_error("file mapping failed: " + name);
        }
        if (create && GetLastError() == ERROR_ALREADY_EXISTS) {   // Windows has no O_EXCL: a live name is RETURNED, not refused
            CloseHandle(mapping_);
            throw std::runtime_error("file mapping exists: " + name);
        }
        view_ = MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, size);
        if (view_ == nullptr) {
            CloseHandle(mapping_);
            throw std::runtime_error("MapViewOfFile failed: " + name);
        }
#else
        const int flags = create ? (O_RDWR | O_CREAT | O_EXCL) : O_RDWR;   // EXCL: a stale name is an error, not a reuse
        fd_ = ::shm_open(name.c_str(), flags, 0600);
        if (fd_ < 0) {
            throw std::runtime_error("shm_open failed: " + name);
        }
        if (create && ::ftruncate(fd_, static_cast<off_t>(size)) != 0) {   // once, at creation: macOS refuses a second (EINVAL)
            ::close(fd_);
            ::shm_unlink(name.c_str());
            throw std::runtime_error("ftruncate failed: " + name);
        }
        view_ = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (view_ == MAP_FAILED) {
            ::close(fd_);
            if (create) ::shm_unlink(name.c_str());
            throw std::runtime_error("mmap failed: " + name);
        }
#endif
    }

    ~SharedRegion() {
#if defined(_WIN32)
        UnmapViewOfFile(view_);
        CloseHandle(mapping_);          // the object dies with its last handle: nothing to unlink (a FILE-backed mapping leaves its file)
#else
        ::munmap(view_, size_);
        ::close(fd_);                   // the NAME stays until someone unlinks it - see unlink()
#endif
    }
    SharedRegion(const SharedRegion&) = delete;
    SharedRegion& operator=(const SharedRegion&) = delete;

    // The creator's last duty: without it the region outlives every process
    // that mapped it (Appendix G's price), and the next create fails on the
    // stale name. Windows has no such step and no such leak.
    static void unlink(const std::string& name) {
#if !defined(_WIN32)
        ::shm_unlink(name.c_str());
#else
        (void)name;
#endif
    }

    void* data() const { return view_; }

private:
    std::size_t size_;
    void* view_ = nullptr;
#if defined(_WIN32)
    HANDLE mapping_ = nullptr;
#else
    int fd_ = -1;
#endif
};

using namespace std::chrono_literals;

// The reader's wait, bounded: the counter is the only synchronization, and
// a writer that never comes is a failed assertion, not a hung harness.
static bool seq_reaches(const Frame& f, std::uint32_t n, std::chrono::milliseconds deadline) {
    const auto until = std::chrono::steady_clock::now() + deadline;
    while (f.seq.load(std::memory_order_acquire) < n) {
        if (std::chrono::steady_clock::now() > until) return false;
        std::this_thread::sleep_for(2ms);
    }
    return true;
}

static void write_frame(Frame& f) {
    f.version = 1;
    f.width = 8;
    f.height = 8;
    for (std::uint8_t i = 0; i < 64; ++i) f.pixels[i] = static_cast<std::uint8_t>(i * 3);
    f.seq.store(1, std::memory_order_release);      // AFTER the payload: release pairs with the reader's acquire
}

static void check_frame(const Frame& f) {
    assert(f.version == 1);
    assert(f.width == 8 && f.height == 8);
    for (int i = 0; i < 64; ++i) assert(f.pixels[i] == static_cast<std::uint8_t>(i * 3));
}

int main() {
#if defined(_WIN32)
    const std::string name = "Local\\cookbook_shm_" + std::to_string(GetCurrentProcessId());
    SharedRegion a(name, sizeof(Frame), true);
    SharedRegion b(name, sizeof(Frame), false);          // a second view of the same object
    Frame& fa = *new (a.data()) Frame{};
    Frame& fb = *static_cast<Frame*>(b.data());   // the overlay Chapter 34 bans for a wire - see the Why for why it is the tool here
    write_frame(fa);
    assert(seq_reaches(fb, 1, 1000ms));
    check_frame(fb);
    std::cout << "shared memory ok (Windows): one process, two views, "
              << sizeof(Frame) << "-byte frame visible through both; cross-process not verified here\n";
#else
    // macOS caps the name at 31 characters (PSHMNAMLEN) - a longer one fails
    // with ENAMETOOLONG. Per-pid, so two runs never collide on a stale name.
    const std::string name = "/cb_shm_" + std::to_string(static_cast<long>(::getpid()));
    assert(name.size() <= 31);
    SharedRegion::unlink(name);                           // a previous run that died: start clean

    // The lowest free descriptor, before and after: a close the destructor
    // forgot would move it. (POSIX: open() returns the lowest free number.)
    const int fd_before = ::open("/dev/null", O_RDONLY);
    ::close(fd_before);

    {
        SharedRegion region(name, sizeof(Frame), true);
        Frame& frame = *new (region.data()) Frame{};      // zeroed: seq starts at 0

        // O_EXCL, judged through the class: a second create of the live name
        // must be refused, and refused BY shm_open. A class without the flag
        // is refused one call later on macOS, by ftruncate, and its error
        // path then unlinks a name it never owned - and on Linux it is not
        // refused at all - which is why the flag is not a nicety, and why
        // the assertion reads which call said no.
        std::string refusal;
        try {
            SharedRegion again(name, sizeof(Frame), true);
        } catch (const std::runtime_error& e) {
            refusal = e.what();
        }
        assert(refusal.rfind("shm_open failed", 0) == 0);

        const pid_t child = ::fork();
        assert(child >= 0);
        if (child == 0) {
            // The other process: map by name, write, bump the counter, leave.
            SharedRegion theirs(name, sizeof(Frame), false);
            write_frame(*static_cast<Frame*>(theirs.data()));
            _exit(0);
        }
        assert(seq_reaches(frame, 1, 5000ms));            // the deadline, never an unbounded wait
        check_frame(frame);
        int status = 0;
        assert(::waitpid(child, &status, 0) == child);
        assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }   // both handles released; the NAME is still there

    // Appendix G's price, demonstrated: the region outlives the process that
    // made it. Opening by name still works after every mapping is gone...
    {
        SharedRegion still_there(name, sizeof(Frame), false);
        assert(static_cast<Frame*>(still_there.data())->seq.load() == 1);   // ...and still holds the data
    }
    const int fd_after = ::open("/dev/null", O_RDONLY);
    ::close(fd_after);
    assert(fd_after == fd_before);                        // every descriptor the regions opened was closed
    // ...until the creator unlinks it, after which the name is gone.
    SharedRegion::unlink(name);
    bool gone = false;
    try {
        SharedRegion ghost(name, sizeof(Frame), false);
    } catch (const std::runtime_error&) {
        gone = true;
    }
    assert(gone);
    std::cout << "shared memory ok: a " << sizeof(Frame) << "-byte frame written by a child process, "
              << "read by the parent, the name outlived both mappings and died on unlink\n";
#endif
    return 0;
}
