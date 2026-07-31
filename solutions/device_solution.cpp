// Device SDK exercise - reference solution.
#include "FakeDevice.h"
#include <cassert>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

// Part A - RAII session. MOVE-ONLY: unlike ThingDataGuard (one struct, one
// scope), a device session is an ownable resource you may want to store in
// containers or return from factories - so it gets the full move treatment.
class DeviceSession {
public:
    DeviceSession() = default;                       // empty session

    static DevErr Open(const char* name, DeviceSession& out) {
        DeviceHandle h = nullptr;
        DevErr err = Device_Open(name, &h);
        if (err != DevOk) return err;
        out = DeviceSession(h);                      // move-assign into caller
        return DevOk;
    }

    ~DeviceSession() { Device_Close(h_); }           // null-safe by contract

    DeviceSession(const DeviceSession&) = delete;    // copying a handle would
    DeviceSession& operator=(const DeviceSession&) = delete;   // double-close

    DeviceSession(DeviceSession&& o) noexcept
        : h_(std::exchange(o.h_, nullptr)),
          onSample_(std::move(o.onSample_)) {
        Rebind();                                    // ctx points at *this* -
    }                                                // it moved, so re-register!

    DeviceSession& operator=(DeviceSession&& o) noexcept {
        if (this != &o) {
            Device_Close(h_);
            h_ = std::exchange(o.h_, nullptr);
            onSample_ = std::move(o.onSample_);
            Rebind();
        }
        return *this;
    }

    bool IsOpen() const { return h_ != nullptr; }

    // Part B - the trampoline: bridge the C callback to std::function.
    DevErr OnSample(std::function<void(int)> fn) {
        onSample_ = std::move(fn);
        return Rebind();
    }

    DevErr Poll() { return h_ ? Device_Poll(h_) : DevClosed; }
    DevErr Inject(size_t n) { return h_ ? FakeDevice_InjectSamples(h_, n) : DevClosed; }

private:
    explicit DeviceSession(DeviceHandle h) : h_(h) {}

    static void Trampoline(int sample, void* ctx) {  // the C-shaped landing pad
        auto* self = static_cast<DeviceSession*>(ctx);
        if (self->onSample_) self->onSample_(sample);
    }

    DevErr Rebind() {
        if (!h_) return DevOk;
        return onSample_
            ? Device_SetCallback(h_, &Trampoline, this)
            : Device_SetCallback(h_, nullptr, nullptr);
    }

    DeviceHandle h_ = nullptr;
    std::function<void(int)> onSample_;
};

int main() {
    {
        DeviceSession s;
        DevErr err = DeviceSession::Open("sensor0", s);
        assert(err == DevOk && s.IsOpen());
        assert(FakeDevice_OpenHandles() == 1);

        std::vector<int> got;
        s.OnSample([&got](int v) { got.push_back(v); });   // a real closure,
        s.Inject(3);                                       // riding a C API
        s.Poll();
        assert((got == std::vector<int>{100, 101, 102}));
        std::cout << "callbacks ok: got " << got.size() << " samples\n";

        // move the session - the trampoline context must follow it
        DeviceSession s2 = std::move(s);
        assert(!s.IsOpen() && s2.IsOpen());
        s2.Inject(1);
        s2.Poll();
        assert(got.size() == 4);                    // still lands in 'got'
        std::cout << "moved session still delivers: " << got.back() << "\n";

        // error paths
        DeviceSession dup;
        assert(DeviceSession::Open("sensor0", dup) == DevBusy);   // already open
        DeviceSession nope;
        assert(DeviceSession::Open("sensor9", nope) == DevNotFound);
    }   // s2's destructor closes; s's destructor closes nothing (null handle)

    assert(FakeDevice_OpenHandles() == 0);          // the leak check
    std::cout << "all handles closed\n";
    return 0;
}
