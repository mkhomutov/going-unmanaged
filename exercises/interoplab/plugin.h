// The surface you publish for a managed caller to P/Invoke — Chapter 39.
//
// It is Chapter 30's extern "C" façade with four additions that only matter
// once the caller is a runtime rather than a compiler. Every one of them
// exists because the declaration on the other side is written BY HAND, in
// another language, and nothing checks that the two agree.
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 0 is success, as everywhere in this book.
typedef int32_t PluginResult;
#define PLUGIN_OK              0
#define PLUGIN_BAD_ARGUMENT    1
#define PLUGIN_BUFFER_TOO_SMALL 2
#define PLUGIN_VERSION_MISMATCH 3
// Something threw inside and nothing left this library. No output parameter
// was written, so Create allocated nothing and GetName wrote nothing.
#define PLUGIN_FAILED          4

// THREAD SAFETY: one handle, one thread. Two threads may use two different
// handles; two threads on one handle is a bug in the caller, and Destroy must
// not overlap any other call on that handle. Chapter 16's fourth question,
// asked of us - so we answer it here rather than leaving it to be guessed.
typedef struct PluginImpl* PluginHandle;   // opaque: no layout to disagree about

// Addition 1: a leading size field, and it is not decoration here.
// A managed caller re-declares this struct by hand. `size` is the only way
// either side can find out at RUNTIME that the two declarations disagree,
// and it is the difference between a clear error and a silent wrong answer.
//
// Every field below is BLITTABLE: fixed-width integers only, no bool, no
// char, no pointer-to-anything-managed. The marshaller copies these bytes
// and does not translate them, which is the cheapest and least surprising
// thing it can do.
typedef struct {
    uint32_t size;        // caller sets this to sizeof(PluginOptions)
    int32_t  gain;
    int32_t  channels;
} PluginOptions;

PluginResult Plugin_Create(const PluginOptions* options, PluginHandle* out);
PluginResult Plugin_Destroy(PluginHandle h);

// Addition 2: the caller allocates the string, and we report what we need.
//
// `size_t` is the one non-fixed-width type on this surface, and it is
// deliberate: a buffer length genuinely is pointer-sized. The managed
// spelling is `nuint` (or `UIntPtr` before C# 9) - never `int`, which agrees
// with us on 32-bit and truncates on everything we actually ship.
//
// Call with buffer == NULL to learn `*needed` (INCLUDING the terminator),
// then call again with a buffer that size. Nothing we allocate ever crosses
// the boundary, so the question "which heap frees this?" never has to be
// answered - and the managed side cannot get it wrong, because there is
// nothing there for it to free.
//
// Addition 3: the encoding, stated rather than assumed.
//
// ENCODING IS PART OF THE CONTRACT: this is UTF-8, always, on every
// platform. A managed caller marshals it as UTF-8 explicitly. It is not
// "the platform's char", because there is no such agreement to rely on.
PluginResult Plugin_GetName(PluginHandle h, char* buffer, size_t capacity,
                            size_t* needed);

// Addition 4: a callback whose lifetime window is written down.
//
// WE HOLD THIS POINTER FROM Plugin_SetSink UNTIL Plugin_ClearSink RETURNS,
// AND NOT ONE INSTRUCTION LONGER.
//
// That sentence is the entire contract, and it exists because a managed
// caller's function pointer is only alive while something on their side
// keeps it rooted. They cannot write that code correctly unless we say how
// long we hold it - so we say, in the header, where they will read it.
// The managed delegate needs [UnmanagedFunctionPointer(CallingConvention.Cdecl)]:
// [DllImport]'s default on Windows is StdCall, this is a cdecl function
// pointer, and on 32-bit x86 the stack pays for the difference.
typedef void (*PluginSink)(int32_t sample, void* user);

PluginResult Plugin_SetSink(PluginHandle h, PluginSink sink, void* user);
PluginResult Plugin_ClearSink(PluginHandle h);
PluginResult Plugin_Pump(PluginHandle h, int32_t sample);

#ifdef __cplusplus
}
#endif
