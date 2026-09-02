// The implementation. Modern C++ inside, C at the seam - Chapter 30's rule,
// unchanged by the caller being a runtime.
#include "plugin.h"

#include <cstring>
#include <memory>
#include <string>

struct PluginImpl {
    std::string name;          // std:: is fine IN HERE. It never crosses.
    int32_t gain = 0;
    int32_t channels = 0;
    PluginSink sink = nullptr;
    void* user = nullptr;
};

extern "C" {

PluginResult Plugin_Create(const PluginOptions* options, PluginHandle* out) {
    if (!out || !options) return PLUGIN_BAD_ARGUMENT;
    *out = nullptr;

    // The size field earning its place. A caller built against a smaller or
    // larger declaration is caught HERE, with a result code, rather than by
    // reading fields that were never written.
    if (options->size != sizeof(PluginOptions)) return PLUGIN_VERSION_MISMATCH;
    if (options->channels <= 0) return PLUGIN_BAD_ARGUMENT;

    // Nothing escapes an exported function - Chapter 30's rule, and the frame
    // above this one belongs to a runtime that cannot catch a C++ exception at
    // all. Every entry point here carries the guard for that reason, whether or
    // not its body can throw today. RAII inside, a raw pointer at the seam: the
    // handle only becomes the caller's on the line that cannot fail.
    try {
        auto p = std::make_unique<PluginImpl>();
        // Deliberately non-ASCII and deliberately past the BMP: 9 characters,
        // 14 UTF-8 bytes and TEN UTF-16 units - three different numbers, which
        // is the whole point of Plugin_GetName's contract.
        p->name = "Z\xC3\xA4hler-\xC2\xB5\xF0\x9D\x84\x9E";
        p->gain = options->gain;
        p->channels = options->channels;
        *out = p.release();
    } catch (...) { return PLUGIN_FAILED; }
    return PLUGIN_OK;
}

PluginResult Plugin_Destroy(PluginHandle h) {
    try { delete h; }          // tolerates null, like every good Destroy
    catch (...) { return PLUGIN_FAILED; }
    return PLUGIN_OK;
}

PluginResult Plugin_GetName(PluginHandle h, char* buffer, size_t capacity,
                            size_t* needed) {
    if (!h || !needed) return PLUGIN_BAD_ARGUMENT;

    const size_t required = h->name.size() + 1;      // including terminator
    *needed = required;
    if (!buffer) return PLUGIN_OK;                   // the sizing call
    if (capacity < required) return PLUGIN_BUFFER_TOO_SMALL;

    try { std::memcpy(buffer, h->name.c_str(), required); }
    catch (...) { return PLUGIN_FAILED; }
    return PLUGIN_OK;
}

PluginResult Plugin_SetSink(PluginHandle h, PluginSink sink, void* user) {
    if (!h || !sink) return PLUGIN_BAD_ARGUMENT;
    try { h->sink = sink; h->user = user; }
    catch (...) { return PLUGIN_FAILED; }
    return PLUGIN_OK;
}

PluginResult Plugin_ClearSink(PluginHandle h) {
    if (!h) return PLUGIN_BAD_ARGUMENT;
    try { h->sink = nullptr;   // the promise in the header, kept in one line
          h->user = nullptr; }
    catch (...) { return PLUGIN_FAILED; }
    return PLUGIN_OK;
}

PluginResult Plugin_Pump(PluginHandle h, int32_t sample) {
    if (!h) return PLUGIN_BAD_ARGUMENT;
    // The guard that is not theoretical: the sink is the CALLER'S code, run
    // from inside our exported function, so anything it throws unwinds out
    // through us - Chapter 18's trampoline rule arriving from the other side.
    try { if (h->sink) h->sink(sample * h->gain, h->user); }
    catch (...) { return PLUGIN_FAILED; }
    return PLUGIN_OK;
}

}   // extern "C"
