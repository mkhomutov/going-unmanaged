// Appendix F, Recipes 33 and 34 - hold an owned object as a field, and an
// object too big for the stack.
//
// Log, Sink, Session, FrameBuffer and make_frame() are quoted VERBATIM in
// book/F-rosetta-cookbook.md: editing one means editing the appendix in the
// same commit (the testlab discipline). main() is scaffolding - it records
// the order the fields die in, checks that a co-owned Sink outlives the
// Session that shared it, and proves the big object landed on the heap.
#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// Recipe 32 - a field of class type, and who disposes it
class Log {                                  // polymorphic: lives behind a pointer (Chapter 2)
public:
    virtual ~Log() = default;
    virtual void write(const std::string& line) = 0;
};

struct Sink {                                // shared with a callback: co-owned (Chapter 29)
    std::vector<int> samples;
};

class Session {
public:
    Session(std::string name, std::unique_ptr<Log> log, std::shared_ptr<Sink> sink)
        : name_(std::move(name)), log_(std::move(log)), sink_(std::move(sink)) {}

    void record(int sample) {
        sink_->samples.push_back(sample);
        if (log_) {                          // the pointer is where "may be absent" lives
            log_->write(name_ + ": recorded");
        }
    }

private:
    std::string name_;                       // by value: the field IS the object, and dies with the owner
    std::vector<int> history_;               // by value too: its elements are on the heap, the field is three pointers
    std::unique_ptr<Log> log_;               // one owner, polymorphic, optional: behind a unique_ptr
    std::shared_ptr<Sink> sink_;             // co-owned: alive while anyone still holds it
};   // no Dispose to write: the fields die in reverse order of declaration, then the object

// Recipe 33 - an object too big for the stack
struct FrameBuffer {
    std::array<std::uint8_t, 4 * 1024 * 1024> pixels{};   // 4 MB inline: a class this size has no business on a stack
};
static_assert(sizeof(FrameBuffer) > 1024 * 1024, "FrameBuffer is a heap object by design");

std::unique_ptr<FrameBuffer> make_frame() {
    return std::make_unique<FrameBuffer>();  // one owner on the stack, four megabytes on the heap
}

namespace {
    // A Log that reports its own death, so main can watch the field order.
    class RecordingLog : public Log {
    public:
        explicit RecordingLog(std::vector<std::string>& events) : events_(events) {}
        ~RecordingLog() override { events_.push_back("log destroyed"); }
        void write(const std::string& line) override { events_.push_back(line); }
    private:
        std::vector<std::string>& events_;
    };
}

int main() {
    // Recipe 32: the field that is a unique_ptr dies with the Session; the
    // shared one does not, because someone else still holds it.
    std::vector<std::string> events;
    auto sink = std::make_shared<Sink>();
    {
        Session s("bench", std::make_unique<RecordingLog>(events), sink);
        s.record(7);
        assert(sink.use_count() == 2);       // the Session and this frame
    }                                        // ~Session: sink_, then log_, then history_, then name_
    assert(events.size() == 2);
    assert(events[0] == "bench: recorded");
    assert(events[1] == "log destroyed");    // the unique_ptr field freed its Log, unasked
    assert(sink.use_count() == 1);           // the shared field released its claim...
    assert(sink->samples.size() == 1);       // ...and the Sink is still here, because we are

    // A Session with no Log: the pointer is where absence lives.
    Session quiet("quiet", nullptr, sink);
    quiet.record(8);
    assert(sink->samples.size() == 2);

    // Chapter 6's Rule of Zero, read back from the type: a unique_ptr field
    // deletes the copy and keeps the move, and nothing was written for it.
    static_assert(!std::is_copy_constructible_v<Session>);
    static_assert(std::is_nothrow_move_constructible_v<Session>);

    // Recipe 33: the object is on the heap and the owner is the size of a
    // pointer; a FrameBuffer local in a function running on a macOS worker
    // thread (512 KB of stack, Chapter 3) would not survive its own prologue.
    std::unique_ptr<FrameBuffer> frame = make_frame();
    static_assert(sizeof(frame) == sizeof(void*));
    frame->pixels[0] = 255;
    frame->pixels.back() = 1;
    assert(frame->pixels[1] == 0);           // make_unique value-initialised the four megabytes
    return 0;
}
