// widget_demo.cpp - the caller side of Technique 1, and the only side that
// matters for the claim. It compiles against Widget.h and nothing else: Impl is
// not merely private here, it is a name with no definition in this translation
// unit, which is what makes the class's layout unchangeable from here.
//
// The chapter's proof is a relink experiment - build this once, grow Impl, link
// again without recompiling, watch sizeof stay at 8. That half stays book-only
// (it needs two implementations, and the second exists to prove a negative);
// what a single build CAN check is the fact the experiment rests on, so that is
// what the static_assert below states.
#include "Widget.h"
#include <cassert>
#include <iostream>
#include <utility>

// The whole point of the technique, as a compile-time claim: whatever Impl
// holds - a string today, a vector and a double tomorrow - the caller sees one
// pointer. There is nothing left in the header for a new member to move.
static_assert(sizeof(Widget) == sizeof(void*),
              "PIMPL means the caller sees exactly one pointer");

int main() {
    Widget a("first");
    assert(a.Score() == 7);

    // The move operations are declared in the header and defaulted in the .cpp,
    // so they exist and they work. Defaulting one in the header instead is the
    // incomplete-type error the chapter's Trap describes; omitting it entirely
    // deletes the moves (user-declared destructor), and this std::move would
    // fall back to the deleted copy instead.
    Widget b(std::move(a));                 // a is now a husk: do not ask it anything
    assert(b.Score() == 7);

    Widget c("second");
    c = std::move(b);
    assert(c.Score() == 7);

    std::cout << "widget: sizeof as the caller sees it = " << sizeof(Widget)
              << ", Score()=" << c.Score() << "\n";
}
