// Appendix H, procedures 1 and 4 - which container, and what goes in it.
//
// The three storage shapes below are quoted in the appendix: editing one
// means editing Appendix H in the same commit (the cookbook discipline).
// main() is scaffolding - it asserts the property each branch is chosen
// FOR, which for procedure 4 is address stability: the reason a reader
// cannot see by reading, and the one this book states twice without ever
// naming (Chapter 20 by way of slicing, Chapter 33 by way of a dangling
// pointer into a grown vector).
#include <cassert>
#include <cstdio>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "counted.h"

// A polymorphic pair, for the branch slicing decides (Chapter 20's lab
// proves the slicing; here it only has to dispatch).
class Shape {
public:
    virtual ~Shape() = default;
    virtual int Sides() const = 0;
};
class Triangle : public Shape {
public:
    int Sides() const override { return 3; }
};
class Square : public Shape {
public:
    int Sides() const override { return 4; }
};

// Force a reallocation and report whether the first element moved. Taking
// the address BEFORE and comparing AFTER is the whole experiment.
template <class Vec>
bool FirstElementMovedOnGrowth(Vec& v) {
    const void* before = &v[0];
    while (v.size() < v.capacity()) v.emplace_back();   // fill to capacity
    v.emplace_back();                                   // the growth
    return before != static_cast<const void*>(&v[0]);
}

int main() {
    // --- procedure 1: a vector's ELEMENTS move when it grows ---------------
    {
        std::vector<Counted> v;
        v.reserve(2);
        v.emplace_back("a");
        assert(FirstElementMovedOnGrowth(v));   // every pointer, reference and
    }                                           // iterator into it just died

    // --- procedure 1: within reserved capacity, nothing moves --------------
    {
        std::vector<Counted> v;
        v.reserve(64);
        v.emplace_back("a");
        const void* before = &v[0];
        for (int i = 0; i < 40; ++i) v.emplace_back("more");
        assert(before == static_cast<const void*>(&v[0]));   // reserve is a promise
    }

    // --- procedure 4: unique_ptr elements - the OBJECTS stand still --------
    {
        std::vector<std::unique_ptr<Counted>> v;
        v.reserve(2);
        v.push_back(std::make_unique<Counted>("a"));
        const Counted* object = v[0].get();     // a pointer to the OBJECT

        while (v.size() < v.capacity()) v.push_back(std::make_unique<Counted>());
        v.push_back(std::make_unique<Counted>());   // the same growth as above

        assert(object == v[0].get());   // the pointers moved; the object did not
        assert(!object->Payload().empty());        // ...and is still readable
    }

    // --- procedure 4: what the growth COSTS the elements -------------------
    {
        std::vector<Counted> plain;
        plain.reserve(8);
        for (int i = 0; i < 8; ++i) plain.emplace_back("x");
        ResetTally();
        plain.emplace_back("trigger");          // reallocate 8 elements
        const int moved_objects = Tally().moves;

        std::vector<std::unique_ptr<Counted>> boxed;
        boxed.reserve(8);
        for (int i = 0; i < 8; ++i) boxed.push_back(std::make_unique<Counted>("x"));
        ResetTally();
        boxed.push_back(std::make_unique<Counted>("trigger"));
        const int moved_boxed = Tally().moves;

        assert(moved_objects == 8);   // every element move-constructed anew
        assert(moved_boxed  == 0);    // only pointers moved: the objects never knew
    }

    // --- procedure 1: node-based containers never move an element ----------
    {
        std::map<int, Counted> m;
        m.emplace(1, Counted("first"));
        const Counted* node = &m.at(1);
        for (int i = 2; i < 200; ++i) m.emplace(i, Counted("filler"));
        assert(node == &m.at(1));     // a tree rebalances POINTERS, not nodes

        std::list<Counted> l;
        l.emplace_back("first");
        const Counted* cell = &l.front();
        for (int i = 0; i < 200; ++i) l.emplace_back("filler");
        assert(cell == &l.front());
    }

    // --- procedure 4: the polymorphic branch -------------------------------
    {
        std::vector<std::unique_ptr<Shape>> shapes;
        shapes.push_back(std::make_unique<Triangle>());
        shapes.push_back(std::make_unique<Square>());
        int total = 0;
        for (const auto& s : shapes) total += s->Sides();
        assert(total == 7);           // 3 + 4: each kept its own identity
    }

    std::printf("choosing/storing: growth moves vector elements and leaves "
                "boxed ones alone, exactly as the appendix claims\n");
    return 0;
}
