// ONE application, consumed three ways.
//
// This file is identical for all three consumption paths - that is the point
// of the lab. Nothing here says where mathlib came from, and nothing here
// names a header path: the angle-bracket include works because the library's
// CMakeLists declared its include directory PUBLIC, so the include path
// arrives through the link, not through the app's build description.
#include <mathlib/mathlib.h>

#include <cstdio>

int main() {
    std::printf("mathlib %s: 2+3=%d\n", mathlib::Version(), mathlib::Add(2, 3));
    return 0;
}
