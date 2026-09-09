// The second half of Recipe 50's demonstration (see namespaces.cpp).
//
// It defines a function with exactly the same name as the one over there,
// in its own unnamed namespace, with a deliberately different rule - and the
// program links. That is the whole claim: internal linkage means the linker
// is never asked to choose between them, so a helper name in a .cpp is yours
// alone and renaming it is nobody else's problem.
#include "namespaces.h"

namespace {
    // Same name as namespaces.cpp's, different body, no collision. Under a
    // unity build (Appendix J) these two files share one translation unit and
    // this stops being true - which is that entry's warning, demonstrated.
    int clamp_to_range(int value, int low, int high) {
        (void)low;
        (void)high;
        return value;              // this file does not clamp at all
    }
}

int other_normalize_reading(int raw) {
    return clamp_to_range(raw, 0, 100);   // the other clamp, always
}
