// engine_demo.cpp - the caller side of Technique 3, written the way Chapter 17
// and Chapter 18 taught you to consume one: every error code checked, the
// handle destroyed exactly once.
//
// It is a C++ file, but notice how little of that shows at the seam - the same
// calls would compile in C, which is the reach the technique buys.
#include "engine.h"
#include <cassert>
#include <iostream>

int main() {
    // The happy path, with every return value inspected rather than assumed.
    EngineHandle h = nullptr;
    int rc = Engine_Create(21, &h);
    assert(rc == 0);
    assert(h != nullptr);

    int score = 0;
    rc = Engine_Score(h, &score);
    assert(rc == 0);
    assert(score == 42);                    // a value, not merely "it survived"

    // The documented null-parameter path. A boundary that segfaults on a bad
    // argument has no failure contract; this one returns 1 and does nothing.
    assert(Engine_Create(21, nullptr) == 1);
    assert(Engine_Score(nullptr, &score) == 1);
    assert(Engine_Score(h, nullptr) == 1);
    assert(Engine_Destroy(nullptr) == 1);
    assert(score == 42);                    // and the failed calls wrote nothing

    // Exactly once. A second Engine_Destroy(h) would be a double free, and the
    // handle tells the caller nothing that would let it notice - which is why
    // Chapter 17's RAII wrapper exists on this side of the boundary.
    rc = Engine_Destroy(h);
    assert(rc == 0);

    std::cout << "engine: created, scored " << score << ", destroyed once\n";
}
