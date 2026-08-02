// scorer_demo.cpp - the caller side of Technique 2. It knows the interface, the
// factory, and nothing whatsoever about Scorer.
//
// The chapter's break-it-first step - insert a pure-virtual method at the TOP
// of IScorer, rebuild only the library, watch the caller reach the wrong slot
// with no diagnostic anywhere - stays book-only: it exists to fail, and it
// needs a stale caller binary that a single build cannot produce.
#include "IScorer.h"
#include <cassert>
#include <iostream>
#include <type_traits>

// The protected destructor makes `delete scorer` a compile error rather than a
// documented rule, and this asserts that at compile time.
static_assert(!std::is_destructible_v<IScorer>,
              "IScorer's destructor must stay protected: Destroy(), never delete");

int main() {
    IScorer* s = CreateScorer(21);
    assert(s != nullptr);
    assert(s->Score() == 42);

    // delete s;   // would not compile - which is the design, not an oversight
    s->Destroy();  // hand it back to the allocator that made it

    std::cout << "scorer: created, scored 42, destroyed through the interface\n";
}
