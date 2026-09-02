#include <mathlib/mathlib.h>

namespace mathlib {

int Add(int a, int b) { return a + b; }

// MATHLIB_VERSION is defined PRIVATE by this library's CMakeLists from the
// project() version - so it compiles into the library and no consumer can
// see it. A consumer that could read it would be reading a build detail.
const char* Version() { return MATHLIB_VERSION; }

}   // namespace mathlib
