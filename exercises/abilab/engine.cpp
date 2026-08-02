// engine.cpp - modern C++ inside, C at the seam.
//
// Chapter 30 prints EngineImpl and Engine_Create verbatim; the other two
// functions complete the excerpt consistently, so changing the top half means
// updating that listing in the same commit.
//
// The discipline is only at the seam, and it is exactly two rules: nothing
// whose layout the compiler chose appears in a signature, and no exception
// leaves this file. std::string in EngineImpl is fine - it never crosses.
#include "engine.h"
#include <string>

struct EngineImpl { int seed; std::string note; };   // std:: is fine IN HERE

extern "C" int Engine_Create(int seed, EngineHandle* out) {
    if (!out) return 1;
    try { *out = new EngineImpl{seed, "internal"}; }
    catch (...) { return 2; }                        // no exception escapes. ever.
    return 0;
}

extern "C" int Engine_Score(EngineHandle h, int* outScore) {
    if (!h || !outScore) return 1;
    // Nothing here can throw today. The guard is not about today: it is the
    // entry point's standing contract, so that whatever this body grows into
    // still cannot unwind into a caller that may not even be C++.
    try { *outScore = h->seed * 2; }
    catch (...) { return 2; }
    return 0;
}

extern "C" int Engine_Destroy(EngineHandle h) {
    if (!h) return 1;
    // The delete happens HERE, with the allocator that ran the new above -
    // which is why the header hands out this function instead of a struct
    // definition the caller could free itself.
    try { delete h; }
    catch (...) { return 2; }
    return 0;
}
