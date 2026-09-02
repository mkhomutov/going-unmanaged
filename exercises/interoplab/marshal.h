// A stand-in for the marshaller, so the boundary can be judged offline.
//
// This is the same move the book already makes with FakeSDK and FakeDevice:
// rather than depend on a real vendor, model the idiom. Here the thing being
// modelled is the .NET marshaller - what it does to a struct, a string and a
// delegate on their way across - written in C++ so build_all.sh can run it
// under the canonical flags. No CLR is involved and none is needed: every
// mistake this chapter is about is observable from THIS side of the call.
//
// What it does NOT model, deliberately: the garbage collector. A collected
// delegate is a use-after-free, and this repository does not commit programs
// whose bug is the point (CLAUDE.md). Instead the sink below carries an
// `alive` flag, so the harness can ASSERT the documented window is honoured
// rather than demonstrate what happens when it is not.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

// The house judge: counts failures and sets the exit code. Never assert(),
// which -DNDEBUG compiles away - see exercises/choosing/counted.h.
inline int& Failures() { static int n = 0; return n; }

#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::fprintf(stderr, "FAILED %s:%d: %s\n", __FILE__, __LINE__,    \
                         #cond);                                              \
            ++Failures();                                                     \
        }                                                                     \
    } while (0)

namespace marshal {

// What `CharSet`/`StringMarshalling` actually decides. A managed string is
// UTF-16; the boundary here is UTF-8; somebody has to convert, and the only
// question is whether both sides agree on who and to what. Recipe 17 of
// Appendix F is the same conversion written for its own sake, and written
// more carefully: it maps invalid input to U+FFFD, where this one assumes
// well-formed UTF-8 because that is what the boundary contract promises.
// A sink that throws, standing in for managed code that raises. The native
// side cannot let this unwind past the boundary, so plugin.cpp turns it into
// a result code - and this is what proves it does.
inline void ThrowingSink(int32_t, void*) { throw std::runtime_error("sink threw"); }

inline std::vector<uint16_t> Utf16FromUtf8(const std::string& s) {
    std::vector<uint16_t> out;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        uint32_t cp;
        size_t len;
        if (c < 0x80)            { cp = c;          len = 1; }
        else if ((c >> 5) == 6)  { cp = c & 0x1Fu;  len = 2; }
        else if ((c >> 4) == 14) { cp = c & 0x0Fu;  len = 3; }
        else                     { cp = c & 0x07u;  len = 4; }
        for (size_t k = 1; k < len && i + k < s.size(); ++k)
            cp = (cp << 6) | (static_cast<unsigned char>(s[i + k]) & 0x3Fu);
        i += len;
        if (cp < 0x10000) {
            out.push_back(static_cast<uint16_t>(cp));
        } else {                                     // surrogate pair
            cp -= 0x10000;
            out.push_back(static_cast<uint16_t>(0xD800 + (cp >> 10)));
            out.push_back(static_cast<uint16_t>(0xDC00 + (cp & 0x3FF)));
        }
    }
    return out;
}

// Counts what a human means by "length", which is a third number again.
inline size_t CodePoints(const std::string& s) {
    size_t n = 0;
    for (unsigned char c : s)
        if ((c & 0xC0u) != 0x80u) ++n;               // skip continuation bytes
    return n;
}

// The managed side of a callback: a target object, and a function pointer
// that is only meaningful while that object is rooted. `alive` is what the
// GC would decide for you and what you cannot see from C++.
struct SinkTarget {
    std::vector<int32_t> received;
    bool alive = true;
    int  calls_after_death = 0;
};

inline void SinkTrampoline(int32_t sample, void* user) {
    SinkTarget* t = static_cast<SinkTarget*>(user);
    if (!t->alive) { ++t->calls_after_death; return; }
    t->received.push_back(sample);
}

}   // namespace marshal
