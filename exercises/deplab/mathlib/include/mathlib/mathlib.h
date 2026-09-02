// Chapter 27's Try it, step 1: the dependency you write yourself.
//
// It is a dependency because it is a SEPARATE PROJECT with its own
// CMakeLists, not because it came from anywhere. Nothing here is
// interesting on purpose - the build description is the subject.
#pragma once

namespace mathlib {

int Add(int a, int b);

// Returns the version this library was BUILT from, so a consumer can prove
// which one it actually got. Step 3 turns on exactly this.
const char* Version();

}   // namespace mathlib
