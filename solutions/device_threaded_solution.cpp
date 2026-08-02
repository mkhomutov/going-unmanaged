// Threaded-callback exercise (Chapter 29, "Try it") - reference solution.
//
// Chapter 18's session was synchronous: you polled, the callback ran on your
// own thread, and the destructor could not race anything. This is the same
// device with a driver thread in front of it, which is what a real peripheral
// SDK looks like - and it is built out of the FakeDevice unchanged, because
// step 3 of the chapter's Try it makes the driver thread OURS.
//
// Two rules shape everything below.
//
// 1. CONFINEMENT. FakeDevice.h documents which thread the callback runs on
//    (the one that called Device_Poll) and says NOTHING about calling the API
//    from two threads at once. By Chapter 16's rule, silence means "not
//    thread-safe" - so every single Device_* call in this file happens on the
//    poller thread. Open, register, unregister and close are posted to it as
//    jobs on a mutex-guarded queue.
// 2. DEFERRED UNREGISTRATION. That is what rule 1 costs, and it is the point.
//    A Session's destructor can no longer unregister; it can only ASK for the
//    unregistration to happen later. So a callback really can arrive after the
//    Session is gone - the non-quiescing SDK of the chapter's fix, built out of
//    a device that only ever calls back on one thread.
//
// Build both ways - the sanitizers do not combine, and they answer different
// questions (a threaded lifetime bug is a use-after-free ASan names outright
// and TSan may or may not surface, depending on how the timing falls):
// (one line each - a trailing backslash inside a // comment is a line splice,
// which -Wall calls out, and rightly: it hides the next line from you)
//   g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -g FakeDevice.cpp device_threaded_solution.cpp -o threaded
//   g++ -std=c++17 -Wall -Wextra -fsanitize=thread -g FakeDevice.cpp device_threaded_solution.cpp -o threaded-tsan
#include "FakeDevice.h"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// The Sink - the state a callback touches, with a lifetime of its own.
// ---------------------------------------------------------------------------
// It is a separate object rather than a member of Session for one reason: it
// must be able to outlive the Session, because a late callback may still be
// holding it up. Everything in it is guarded by the one mutex.
struct Sink {
    std::mutex       m;
    bool             alive = true;      // guarded by m: "my Session still exists"
    std::vector<int> samples;           // guarded by m
};

// ---------------------------------------------------------------------------
// The context registry - why this exists is the whole of the chapter's ending.
// ---------------------------------------------------------------------------
// The SDK stores a void*. We give it a heap-allocated weak_ptr<Sink>, so the
// callback can ASK whether the Sink is still there instead of assuming. That
// holder can never be freed while the SDK might still read it: the SDK loads
// the pointer inside its dispatch loop, one instruction before it calls you,
// and nothing you write can be sequenced against that load. Deleting it in
// ~Session is the use-after-free this exercise exists to teach against.
//
// So the holders are owned HERE, at file scope, and released at shutdown -
// after the poller thread has been joined, which is the one moment when we
// genuinely know no callback is in flight. That is the chapter's own sanctioned
// variant, and it is what keeps the program clean under LeakSanitizer too.
namespace {

std::vector<std::unique_ptr<std::weak_ptr<Sink>>> g_contexts;   // main thread only

void* NewContext(const std::shared_ptr<Sink>& sink) {
    g_contexts.push_back(std::make_unique<std::weak_ptr<Sink>>(sink));
    return g_contexts.back().get();     // the unique_ptr owns it; the SDK borrows
}

std::atomic<long> g_delivered{0};       // samples that actually reached a Sink

// The device hands out 100, 101, ... per injection, so a batch is always the
// same eight values - which makes "sample number i must be 100 + i % 8" an
// invariant that survives being cut off halfway through a batch.
constexpr size_t kBatch = 8;

}   // namespace

// ---------------------------------------------------------------------------
// The poller thread - it owns every Device_* call, and a job queue.
// ---------------------------------------------------------------------------
class Poller {
public:
    // The devices this thread has open. Touched ONLY by the poller thread while
    // it runs, so nothing in here needs a lock - confinement instead of
    // synchronization, which is the cheaper of the two whenever it is available.
    struct State {
        std::map<std::string, DeviceHandle> open;
        // Off, and the sample count is exactly what was injected once, which is
        // what part 1 of main() asserts. On, and every device is refilled before
        // every poll, so the poller is almost always INSIDE a callback - which is
        // what part 2 needs, because a destructor that never races anything
        // proves nothing. One flag, flipped between the two by a posted job.
        bool stream = false;

        void PollAll() {
            for (auto& entry : open) {
                if (stream) FakeDevice_InjectSamples(entry.second, kBatch);
                Device_Poll(entry.second);
            }
        }

        DeviceHandle Take(const std::string& name) {
            auto it = open.find(name);
            assert(it != open.end());           // a close for a device we never opened
            DeviceHandle h = it->second;
            open.erase(it);
            return h;
        }
    };

    using Job = std::function<void(State&)>;

    void Start() { thread_ = std::thread(&Poller::Run, this); }

    // Called from the main thread. The mutex is what publishes the job - and
    // with it everything the job captured - to the poller thread.
    void Post(Job job) {
        std::lock_guard<std::mutex> g(m_);
        jobs_.push_back(std::move(job));
    }

    void StopAndJoin() {
        // Every job this program will ever post has been posted by the time we
        // get here, and they went through m_ - so a poller that sees stop_ set
        // is guaranteed to find all of them in its final drain below.
        stop_ = true;
        thread_.join();                 // std::thread: join or terminate (Ch 29)
    }

    const State& state() const { return state_; }   // safe AFTER the join only

private:
    void Run() {
        while (!stop_) {
            Drain();
            state_.PollAll();           // callbacks run here, on this thread
            std::this_thread::yield();
        }
        Drain();                        // the close jobs posted just before the stop
        assert(state_.open.empty());
    }

    void Drain() {
        std::vector<Job> batch;
        {
            std::lock_guard<std::mutex> g(m_);
            batch.swap(jobs_);          // run them OUTSIDE the lock
        }
        for (auto& job : batch) job(state_);
    }

    State                  state_;      // poller-thread-confined
    std::mutex             m_;
    std::vector<Job>       jobs_;       // guarded by m_
    std::atomic<bool>      stop_{false};
    std::thread            thread_;
};

// ---------------------------------------------------------------------------
// The Session - RAII over a device it is not allowed to touch.
// ---------------------------------------------------------------------------
// Note what is NOT a member: the handle. It belongs to the poller thread, and
// this object never sees it. What the Session owns is the Sink and the
// obligation to post a matching close.
class Session {
public:
    Session(Poller& poller, std::string name)
        : poller_(poller), name_(std::move(name)), sink_(std::make_shared<Sink>()) {
        void* ctx = NewContext(sink_);          // a weak reference, on the heap
        poller_.Post([name = name_, ctx](Poller::State& st) {
            DeviceHandle h = nullptr;
            DevErr err = Device_Open(name.c_str(), &h);
            assert(err == DevOk);               // DevBusy here would mean the
            err = Device_SetCallback(h, &Session::Trampoline, ctx);   // queue's
            assert(err == DevOk);               // FIFO order was violated
            err = FakeDevice_InjectSamples(h, kBatch);
            assert(err == DevOk);
            st.open.emplace(name, h);
        });
    }

    ~Session() {
        // 1. Publish "I am gone" under the lock. A callback already inside the
        //    trampoline finishes; the next one sees this and drops its sample.
        { std::lock_guard<std::mutex> g(sink_->m); sink_->alive = false; }
        // 2+3. Unregister and close - both are Device_* calls, so they are the
        //      poller thread's to make. All we can do is ask, which is exactly
        //      the property that makes the flag above load-bearing.
        poller_.Post([name = name_](Poller::State& st) {
            DeviceHandle h = st.Take(name);
            Device_SetCallback(h, nullptr, nullptr);
            DevErr err = Device_Close(h);       // exactly once, as Chapter 18 -
            assert(err == DevOk);               // a thread repeals no obligation
        });
        // 4. sink_ drops here. The Sink dies with it unless a callback is
        //    holding it up right now, in which case it goes when that returns.
        //    The context holder is NOT freed; g_contexts owns it until shutdown.
    }

    Session(const Session&)            = delete;   // owns a device: no copies
    Session& operator=(const Session&) = delete;

    // A strong reference, so the caller can inspect what arrived after the
    // Session itself is gone.
    std::shared_ptr<Sink> sink() const { return sink_; }

private:
    static void Trampoline(int sample, void* ctx) {
        auto sp = static_cast<std::weak_ptr<Sink>*>(ctx)->lock();
        if (!sp) return;                        // the Sink is gone: touch nothing
        std::lock_guard<std::mutex> g(sp->m);
        if (!sp->alive) return;                 // late callback: Session is gone
        sp->samples.push_back(sample);
        g_delivered.fetch_add(1, std::memory_order_relaxed);
    }

    Poller&               poller_;
    std::string           name_;
    std::shared_ptr<Sink> sink_;
};

namespace {

constexpr int     kCycles = 48;                 // bounded: no unbounded loops here
const char* const kNames[] = {"sensor0", "sensor1", "sensor2", "sensor3"};

std::vector<int> OneBatch() {
    std::vector<int> v;
    for (size_t i = 0; i < kBatch; ++i) v.push_back(static_cast<int>(100 + i));
    return v;
}

// Post a job and wait for the poller to run it. This is the only place the main
// thread synchronizes with the poller mid-run, and it is what makes the first
// assertion below an assertion about VALUES rather than about survival.
// The promise is held by shared_ptr so the job's copy keeps it alive no matter
// which side finishes first.
void FlushAndWait(Poller& poller) {
    auto done = std::make_shared<std::promise<void>>();
    auto wait = done->get_future();
    poller.Post([done](Poller::State& st) {
        st.PollAll();
        done->set_value();
    });
    wait.wait();
}

}   // namespace

int main() {
    assert(FakeDevice_OpenHandles() == 0);

    Poller poller;
    poller.Start();

    // ---- Part 1: the samples are right ------------------------------------
    // Survival is not the claim; the exact sequence is. One session, opened and
    // fed one batch on the poller thread, then flushed - after the flush job has
    // run, all eight samples have been dispatched, and the session is still
    // alive, so the Sink holds exactly what the device sent and nothing else.
    {
        Session s(poller, kNames[0]);
        FlushAndWait(poller);
        std::shared_ptr<Sink> sink = s.sink();
        std::lock_guard<std::mutex> g(sink->m);
        assert(sink->samples == OneBatch());
        std::cout << "samples ok: " << sink->samples.size()
                  << " values, first " << sink->samples.front()
                  << ", last " << sink->samples.back() << "\n";
    }
    FlushAndWait(poller);       // let that session's close job run before part 2

    // ---- Part 2: destroy sessions while the poller is running -------------
    // Three sessions alive at a time over four device names, so the name a new
    // session opens is always one whose close was posted at least one iteration
    // ago - FIFO through the queue then guarantees the close ran first, and
    // Device_Open never has to see DevBusy.
    poller.Post([](Poller::State& st) { st.stream = true; });   // refill every poll
    std::vector<std::shared_ptr<Sink>> kept;
    {
        std::vector<std::unique_ptr<Session>> live;
        for (int i = 0; i < kCycles; ++i) {
            live.push_back(std::make_unique<Session>(poller, kNames[i % 4]));
            // Half the sinks are kept so their contents can be checked after the
            // Session died; the other half are dropped entirely, which is what
            // exercises the weak_ptr-already-expired path in the trampoline.
            if (i % 2 == 0) kept.push_back(live.back()->sink());
            if (live.size() > 3) live.erase(live.begin());      // oldest dies here
            // Not synchronization - a window. The poller is refilling and polling
            // flat out, so it is nearly always inside a callback; this gives it
            // room to be inside one when the erase above runs the destructor.
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    }   // the last three sessions die here, all at once

    poller.StopAndJoin();       // no callback can be in flight after this line

    // ---- The verdict ------------------------------------------------------
    // Every batch is the same eight values, and the alive flag can cut a Sink
    // off part-way through one - so what a Sink holds is 100, 101, ... 107,
    // 100, 101, ..., truncated at an arbitrary point. Never a gap, never a
    // stray value, never a sample after the flag went down. That is the
    // property the flag buys, and unlike "it did not crash" it is checkable.
    long inspected = 0;
    for (const std::shared_ptr<Sink>& sink : kept) {
        for (size_t i = 0; i < sink->samples.size(); ++i)
            assert(sink->samples[i] == static_cast<int>(100 + i % kBatch));
        assert(!sink->alive);                   // every Session was destroyed
        inspected += static_cast<long>(sink->samples.size());
    }
    assert(inspected > 0);                      // callbacks really did flow
    assert(g_delivered.load() >= inspected);
    assert(poller.state().open.empty());
    assert(FakeDevice_OpenHandles() == 0);      // the Chapter 18 leak check

    // NOW the contexts can go: the thread that dispatches callbacks has been
    // joined, so there is no dispatch loop left holding a pointer we are about
    // to free. This is the only ordering that makes freeing them safe, and it
    // is why they were never freed in ~Session.
    g_contexts.clear();

    std::cout << "sessions ok: " << kCycles << " create/destroy cycles, "
              << g_delivered.load() << " samples delivered, "
              << kept.size() << " sinks inspected\n";
    std::cout << "all handles closed\n";
    return 0;
}
