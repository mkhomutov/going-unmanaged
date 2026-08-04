// ============================================================================
// FakeSDK2.h - version 2.0 of the FakeSDK: the project's Things are now
// shared, REFERENCE-COUNTED objects behind opaque handles.
// DO NOT MODIFY THIS FILE. Treat it as vendor code: read it, wrap it, obey it.
//
// THE OWNERSHIP CONVENTION (migration notes, section 2 - read it twice):
//   - Thing_Acquire hands you a reference YOU OWN: the SDK retains it on
//     your behalf, and you owe exactly one Thing_Release when you are done.
//   - Project_PeekActive hands you a reference you DO NOT own: the project
//     keeps it alive for now, and your pointer is valid only until the
//     active Thing changes. Do not release it. Call Thing_Retain first if
//     you intend to keep it.
//   - Thing_Retain / Thing_Release return the new count FOR DIAGNOSTICS
//     ONLY - do not base logic on it.
// ============================================================================
#pragma once
#include <cstddef>

using ErrCode = int;

constexpr ErrCode NoErr        = 0;
constexpr ErrCode ErrNullParam = 1;   // a required pointer was null
constexpr ErrCode ErrBadIndex  = 2;   // no Thing with that index

struct ThingRef;    // opaque: the SDK owns the definition

// How many Things exist in the "project". Never fails if count is non-null.
ErrCode Project_GetCount(size_t* count);

// Fill *out with a reference to the Thing at 'index' (0-based). On success
// the reference is RETAINED ON YOUR BEHALF - you own one Release.
ErrCode Thing_Acquire(size_t index, ThingRef** out);

// The project's currently-active Thing, or null if none. BORROWED: the
// project keeps its own reference, and yours is valid only until the
// active Thing changes. Retain it if you keep it; never release a peek.
ThingRef* Project_PeekActive();

size_t Thing_Retain(ThingRef* ref);    // +1; returns the new count (diagnostics only)
size_t Thing_Release(ThingRef* ref);   // -1; frees the Thing at zero; returns the new count

// Sum of the Thing's values. Requires a non-null ref.
ErrCode Thing_Sum(const ThingRef* ref, double* sum);

// Test-support: (re)build the fake project - thingCount Things, the one at
// activeIndex marked active. Releases any previous project first.
void FakeSdk2_Setup(size_t thingCount, size_t activeIndex);

// Test-support: the host closing the document - the project releases its
// own references and forgets its Things. Objects a client still holds
// references to survive this.
void FakeSdk2_Shutdown();

// Test-support: how many Thing objects are currently alive. The host
// checks this at plug-in unload, AFTER shutdown - it MUST be zero, and it
// is this SDK's leak detector (v1's FakeSdk_LiveAllocations, grown up).
size_t FakeSdk2_LiveObjects();
