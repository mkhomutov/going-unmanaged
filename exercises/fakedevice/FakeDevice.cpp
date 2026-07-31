#include "FakeDevice.h"
#include <cstring>
#include <vector>

struct DeviceImpl {
    bool open = false;
    SampleCallback cb = nullptr;
    void* ctx = nullptr;
    std::vector<int> pending;
};

namespace {
    DeviceImpl g_devices[4];
    size_t g_open = 0;
    int IndexOf(const char* name) {
        if (!name || std::strlen(name) != 7 || std::strncmp(name, "sensor", 6) != 0)
            return -1;
        int i = name[6] - '0';
        return (i >= 0 && i < 4) ? i : -1;
    }
}

size_t FakeDevice_OpenHandles() { return g_open; }

DevErr Device_Open(const char* name, DeviceHandle* outHandle) {
    if (!name || !outHandle) return DevNullParam;
    int i = IndexOf(name);
    if (i < 0) return DevNotFound;
    DeviceImpl& d = g_devices[i];
    if (d.open) return DevBusy;
    d.open = true; d.cb = nullptr; d.ctx = nullptr; d.pending.clear();
    ++g_open;
    *outHandle = &d;
    return DevOk;
}

DevErr Device_Close(DeviceHandle h) {
    if (!h) return DevOk;                 // documented: null is a no-op
    if (!h->open) return DevClosed;       // double-close is an ERROR
    h->open = false; h->cb = nullptr; h->ctx = nullptr;
    --g_open;
    return DevOk;
}

DevErr Device_SetCallback(DeviceHandle h, SampleCallback cb, void* ctx) {
    if (!h) return DevNullParam;
    if (!h->open) return DevClosed;
    h->cb = cb; h->ctx = ctx;
    return DevOk;
}

DevErr Device_Poll(DeviceHandle h) {
    if (!h) return DevNullParam;
    if (!h->open) return DevClosed;
    if (h->cb)
        for (int s : h->pending)
            h->cb(s, h->ctx);
    h->pending.clear();
    return DevOk;
}

DevErr FakeDevice_InjectSamples(DeviceHandle h, size_t n) {
    if (!h) return DevNullParam;
    if (!h->open) return DevClosed;
    for (size_t i = 0; i < n; ++i)
        h->pending.push_back(static_cast<int>(100 + i));
    return DevOk;
}
