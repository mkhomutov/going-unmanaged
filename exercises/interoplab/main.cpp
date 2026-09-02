// The judge. Five assertions, one per thing a hand-written declaration on
// the managed side can get wrong - and each is checked from the native side,
// because that is where every one of them is observable.
#include "marshal.h"
#include "plugin.h"

#include <string>

// A managed caller re-declares your struct BY HAND, in another language.
// This is that declaration, written by someone who read an older header -
// same field order, no size field. It is a different type with a different
// sizeof, and no compiler on either side will ever compare the two.
struct PluginOptionsAsMisdeclared {
    int32_t gain;
    int32_t channels;
};

int main() {
    // ---- 1. the size field catches a declaration that has drifted --------
    // The mismatched caller fills in what it believes the struct is, and the
    // first word of that memory lands in `size`. It is not sizeof anything,
    // so Plugin_Create refuses rather than reading two fields that were
    // never written. Without the field this call would "succeed" with a
    // gain nobody set.
    //
    // The overlay cast below is the one Chapter 34 calls illegal even on the
    // days it works, and it is deliberate here: it stands in for a caller
    // that is not C++ at all and never had our struct to begin with.
    PluginOptionsAsMisdeclared wrong{7, 2};
    PluginHandle stale = nullptr;
    PluginResult rc = Plugin_Create(
        reinterpret_cast<const PluginOptions*>(&wrong), &stale);
    CHECK(rc == PLUGIN_VERSION_MISMATCH);
    CHECK(stale == nullptr);

    // ---- 2. the agreed declaration works ---------------------------------
    PluginOptions opts{};
    opts.size = static_cast<uint32_t>(sizeof(PluginOptions));   // the caller's job
    opts.gain = 3;
    opts.channels = 2;

    PluginHandle h = nullptr;
    CHECK(Plugin_Create(&opts, &h) == PLUGIN_OK);
    CHECK(h != nullptr);

    // ---- 3. caller-allocates: nothing we own ever crosses ----------------
    size_t needed = 0;
    CHECK(Plugin_GetName(h, nullptr, 0, &needed) == PLUGIN_OK);  // sizing call
    CHECK(needed == 15);                    // 14 UTF-8 bytes + terminator

    char small[4];
    size_t ignored = 0;
    CHECK(Plugin_GetName(h, small, sizeof(small), &ignored)
              == PLUGIN_BUFFER_TOO_SMALL);  // reports, never overflows

    std::string name(needed, '\0');
    CHECK(Plugin_GetName(h, &name[0], name.size(), &needed) == PLUGIN_OK);
    name.resize(needed - 1);                // drop the terminator
    CHECK(name == "Z\xC3\xA4hler-\xC2\xB5\xF0\x9D\x84\x9E");

    // ---- 4. three different "lengths", which is why the header names one --
    // The managed side sees 10 UTF-16 units; the boundary carries 14 bytes; a
    // human counts 9 characters. Three different numbers, on purpose: the last
    // character is past the BMP, so it costs a surrogate PAIR - which is why
    // string.Length answers the third number while looking like the first. A
    // buffer sized from the wrong one is the bug, and "the platform's char"
    // names none of them.
    CHECK(name.size() == 14);                            // UTF-8 bytes
    CHECK(marshal::Utf16FromUtf8(name).size() == 10);    // UTF-16 units
    CHECK(marshal::CodePoints(name) == 9);               // characters

    // ---- 5. the callback window the header promises ----------------------
    marshal::SinkTarget target;
    CHECK(Plugin_SetSink(h, &marshal::SinkTrampoline, &target) == PLUGIN_OK);
    CHECK(Plugin_Pump(h, 5) == PLUGIN_OK);
    CHECK(target.received.size() == 1);
    CHECK(target.received[0] == 15);                     // 5 * gain 3

    // The managed side unregisters BEFORE letting its target be collected -
    // which it can only know to do because the header says how long we hold
    // the pointer. After this returns we hold nothing.
    CHECK(Plugin_ClearSink(h) == PLUGIN_OK);
    target.alive = false;                                // "collected"

    CHECK(Plugin_Pump(h, 5) == PLUGIN_OK);               // still a valid call
    CHECK(target.calls_after_death == 0);                // and it reached nobody
    CHECK(target.received.size() == 1);                  // unchanged

    // ---- 6. nothing escapes an exported function -------------------------
    // The sink is the CALLER'S code running inside our entry point, so a sink
    // that throws is the one exception path we cannot talk anyone out of. It
    // must come back as a result code, not as an unwind through a frame that
    // may not be C++ at all.
    CHECK(Plugin_SetSink(h, &marshal::ThrowingSink, nullptr) == PLUGIN_OK);
    CHECK(Plugin_Pump(h, 5) == PLUGIN_FAILED);
    CHECK(Plugin_ClearSink(h) == PLUGIN_OK);

    CHECK(Plugin_Destroy(h) == PLUGIN_OK);
    CHECK(Plugin_Destroy(nullptr) == PLUGIN_OK);         // tolerates null

    if (Failures() == 0) std::printf("interoplab: all boundary claims hold\n");
    return Failures() == 0 ? 0 : 1;
}
