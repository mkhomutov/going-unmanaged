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
#include <vector>

// The real thing, in policy clothing: static functions over FakeDevice's C
// API (Chapter 18), with the trampoline inside Poll where the void* is.
struct FakeDeviceSdk {
    using Handle = DeviceHandle;

    static Handle Open(const char* name) {
        Handle h = nullptr;
        if (Device_Open(name, &h) != DevOk) throw std::runtime_error("open failed");
        return h;
    }
    static void Close(Handle h) { Device_Close(h); }   // null-safe by the SDK's contract
    static void Poll(Handle h, std::function<void(int)>& sink) {
        Device_SetCallback(h, &Trampoline, &sink);
        Device_Poll(h);
        Device_SetCallback(h, nullptr, nullptr);
    }

private:
    static void Trampoline(int sample, void* ctx) {
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
        for (int s : scripts.at(h - 1)) sink(s);
    }
};
