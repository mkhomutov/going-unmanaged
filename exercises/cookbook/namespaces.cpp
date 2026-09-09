// Appendix F, Recipe 50 - keep a helper out of every other file - and
// Chapter 12's inline-namespace listing.
//
// normalize_reading() is included by book/F-rosetta-cookbook.md and
// versioned_frame_size() by book/12-the-compilation-model.md, each between
// its section markers: edit here and the page follows, and a marker moved is
// what the page shows. main() is scaffolding.
//
// This TU has a SECOND half, namespaces_other.cpp, and that is the point
// rather than a build detail. An unnamed namespace's whole claim is about
// what happens ACROSS translation units - that the same name may be defined
// in two of them with two different bodies and the linker never has to
// choose - and one file cannot demonstrate a claim about two. The other file
// defines its own clamp_to_range with a deliberately different rule, and
// main() below asserts that each caller reached its own.
#include <cassert>

#include "namespaces.h"

// Recipe 50 - C#'s `internal`, or a `private static` helper
// --8<-- [start:recipe-50]
namespace {
    // Internal linkage: this name exists in this translation unit and in no
    // other, so the identically-named helper in namespaces_other.cpp is a
    // different function and the two never collide at link time. `static` at
    // namespace scope says the same thing and is the older spelling; the
    // unnamed namespace also works for types, which `static` cannot do.
    int clamp_to_range(int value, int low, int high) {
        return value < low ? low : (value > high ? high : value);
    }
}

int normalize_reading(int raw) {
    return clamp_to_range(raw, 0, 100);   // this file's clamp, always
}
// --8<-- [end:recipe-50]

// Chapter 12 - an inline namespace, the ABI-versioning idiom
// --8<-- [start:inline-namespace]
namespace audio {
    namespace v1 {
        int frame_size() { return 512; }
    }
    inline namespace v2 {          // `inline` = the one plain `audio::` means
        int frame_size() { return 1024; }
    }
}
// --8<-- [end:inline-namespace]

int main() {
    // Recipe 50's claim, and it needs both files: two functions with the same
    // name, in one program, each reached only by its own translation unit.
    assert(normalize_reading(150) == 100);   // clamped here
    assert(normalize_reading(-5) == 0);
    assert(other_normalize_reading(150) == 150);   // NOT clamped there
    assert(other_normalize_reading(-5) == -5);

    // The inline namespace. `audio::frame_size` is v2's - not a copy of it,
    // the same function, which is what makes the older name keep working.
    assert(audio::frame_size() == 1024);
    assert(audio::v2::frame_size() == 1024);
    assert(audio::v1::frame_size() == 512);
    assert(&audio::frame_size == &audio::v2::frame_size);
    assert(&audio::frame_size != &audio::v1::frame_size);

    // And the half that matters at a binary boundary: the version is part of
    // the mangled name, so a caller compiled against v1 keeps calling v1
    // however many versions ship later. That claim is about the SYMBOL rather
    // than the value, so it cannot be asserted from inside the program;
    // scripts/build_all.sh reads it back out of the object file with nm.
    return 0;
}
