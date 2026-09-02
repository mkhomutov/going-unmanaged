// The implementation. Modern C++ inside, C at the seam - Chapter 30's rule,
// unchanged by the caller being a runtime.
#include "plugin.h"

#include <cstring>
#include <new>
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

    PluginImpl* p = new (std::nothrow) PluginImpl;
    if (!p) return PLUGIN_BAD_ARGUMENT;
    // Deliberately non-ASCII: the name is 8 characters and 10 UTF-8 bytes,
    // which is the whole point of Plugin_GetName's contract.
    p->name = "Z\xC3\xA4hler-\xC2\xB5";
    p->gain = options->gain;
    p->channels = options->channels;
    *out = p;
    return PLUGIN_OK;
}

PluginResult Plugin_Destroy(PluginHandle h) {
    delete h;                  // tolerates null, like every good Destroy
    return PLUGIN_OK;
}

PluginResult Plugin_GetName(PluginHandle h, char* buffer, size_t capacity,
                            size_t* needed) {
    if (!h || !needed) return PLUGIN_BAD_ARGUMENT;

    const size_t required = h->name.size() + 1;      // including terminator
    *needed = required;
    if (!buffer) return PLUGIN_OK;                   // the sizing call
    if (capacity < required) return PLUGIN_BUFFER_TOO_SMALL;

    std::memcpy(buffer, h->name.c_str(), required);
    return PLUGIN_OK;
}

PluginResult Plugin_SetSink(PluginHandle h, PluginSink sink, void* user) {
    if (!h || !sink) return PLUGIN_BAD_ARGUMENT;
    h->sink = sink;
    h->user = user;
    return PLUGIN_OK;
}

PluginResult Plugin_ClearSink(PluginHandle h) {
    if (!h) return PLUGIN_BAD_ARGUMENT;
    h->sink = nullptr;         // the promise in the header, kept in one line
    h->user = nullptr;
    return PLUGIN_OK;
}

PluginResult Plugin_Pump(PluginHandle h, int32_t sample) {
    if (!h) return PLUGIN_BAD_ARGUMENT;
    if (h->sink) h->sink(sample * h->gain, h->user);
    return PLUGIN_OK;
}

}   // extern "C"
