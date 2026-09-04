// policies.h - two policies for one Session: the vendor's device behind
// FakeDevice.h, and a recording double that never touches a device. Same
// three static functions, same Handle typedef, and Session cannot tell.
//
// Quoted VERBATIM in Chapter 41 below this banner: editing it means editing
// the chapter in the same commit.
#pragma once
#include "FakeDevice.h"

#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

// The real thing, in policy clothing: static functions over FakeDevice's C
// API (Chapter 18), with the trampoline inside Poll where the void* is.
struct FakeDeviceSdk {
    using Handle = DeviceHandle;

    static Handle Open(const char* name) {
        Handle h = nullptr;
        const DevErr err = Device_Open(name, &h);
        if (err != DevOk) {
            throw std::runtime_error("Device_Open failed: code " + std::to_string(err));
        }
        return h;
    }
    static void Close(Handle h) { Device_Close(h); }   // null-safe by the SDK's contract
    static void Poll(Handle h, std::function<void(int)>& sink) {
        Device_SetCallback(h, &Trampoline, &sink);
        Device_Poll(h);
        Device_SetCallback(h, nullptr, nullptr);       // sink dies with this frame
    }

private:
    // noexcept: the sink is called from inside the vendor's C frame, and
    // nothing may escape through it (Chapter 30's rule). A throwing sink
    // terminates here, loudly, rather than unwinding through C.
    static void Trampoline(int sample, void* ctx) noexcept {
        (*static_cast<std::function<void(int)>*>(ctx))(sample);
    }
};

// The double: Chapter 28's "a fake you own", swapped in at compile time.
// Every session is an index into a table of scripted samples.
struct RecordingSdk {
    using Handle = std::size_t;                         // 0 = closed; slots start at 1

    static inline std::vector<std::vector<int>> scripts;   // what each open() will deliver
    static inline std::size_t open_count = 0;
    static inline std::size_t close_count = 0;

    static Handle Open(const char*) { ++open_count; return open_count; }
    static void Close(Handle h) { if (h != 0) ++close_count; }
    static void Poll(Handle h, std::function<void(int)>& sink) {
        if (h == 0) return;                             // a closed handle delivers nothing
        for (int s : scripts.at(h - 1)) sink(s);
    }
};
