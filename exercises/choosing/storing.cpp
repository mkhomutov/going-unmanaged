// Appendix H, procedures 1 and 4 - which container, and what goes in it.
//
// Quoted in Appendix H, whole and by name: `GrowthRelocatesAndMovesEveryElement`
// and `BoxedElementsStandStillWhenTheVectorGrows`. Editing one means editing
// Appendix H in the same commit (the cookbook discipline), and
// scripts/check_verbatim.sh checks that pairing in BOTH directions - every
// cpp fence on the page must be in this directory, and each function named
// above must be on the page, whole. Everything else here, main() and the
// remaining measurements included, appears in no listing.
//
// What every function here measures is address stability: the property a
// reader cannot see by reading, and the one procedure 4 exists to separate
// from the two other reasons for the same shape.
#include <cstdint>
#include <cstdio>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
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

// The address of a container's block, as an INTEGER. Taken before a
// reallocation and compared after it, which is the whole experiment - and
// an integer because reading a pointer whose storage has been freed is
// implementation-defined ([basic.stc]), even just to compare it.
template <class Vec>
std::uintptr_t BlockAddress(const Vec& v) {
    return reinterpret_cast<std::uintptr_t>(v.data());
}

// Fill to the capacity the implementation ACTUALLY gave us. reserve(n)
// promises capacity >= n, not == n, so counting on eight would make this
// harness fail on a conforming library that rounds up.
template <class Vec, class Make>
int FillToCapacity(Vec& v, Make make) {
    while (v.size() < v.capacity()) v.push_back(make());
    return static_cast<int>(v.size());
}

// Growing a vector<T> relocates every element: the block moves, and each
// element is move-constructed into the new one. Both halves are asserted,
// because "no moves" is also what you measure when nothing grew at all.
void GrowthRelocatesAndMovesEveryElement() {
    std::vector<Counted> v;
    v.reserve(8);
    const int filled = FillToCapacity(v, [] { return Counted("x"); });
    const std::uintptr_t before = BlockAddress(v);

    ResetTally();
    v.emplace_back("trigger");                  // the growth

    CHECK(BlockAddress(v) != before);           // the ground really moved
    CHECK(Tally().moves  == filled);            // every element, move-constructed anew
    CHECK(Tally().copies == 0);                 // moved, not copied: noexcept pays
}

// The same growth, with the elements behind unique_ptr: the block still
// moves, the pointers still shuffle, and the objects never learn about it.
void BoxedElementsStandStillWhenTheVectorGrows() {
    std::vector<std::unique_ptr<Counted>> v;
    v.reserve(8);
    FillToCapacity(v, [] { return std::make_unique<Counted>("x"); });
    const Counted* object = v[0].get();         // a pointer to the OBJECT
    const std::uintptr_t before = BlockAddress(v);

    ResetTally();
    v.push_back(std::make_unique<Counted>("trigger"));

    CHECK(BlockAddress(v) != before);           // the same growth as above
    CHECK(object == v[0].get());                // the pointers moved; the object did not
    CHECK(!object->Payload().empty());          // ...and is still readable
    CHECK(Tally().moves  == 0);                 // no element move-constructed
    CHECK(Tally().copies == 0);
}

// reserve is a promise: within the capacity it bought, nothing relocates.
void ReserveHoldsAddressesStill() {
    std::vector<Counted> v;
    v.reserve(64);
    v.emplace_back("a");
    const std::uintptr_t before = BlockAddress(v);
    for (int i = 0; i < 40; ++i) v.emplace_back("more");
    CHECK(BlockAddress(v) == before);
    CHECK(v.capacity() >= 41);                  // we stayed inside the promise
}

// A tree and a linked list rebalance or splice POINTERS, never nodes.
void NodeBasedContainersNeverMoveAnElement() {
    std::map<int, Counted> m;
    m.emplace(1, Counted("first"));
    const Counted* node = &m.at(1);
    for (int i = 2; i < 64; ++i) m.emplace(i, Counted("filler"));
    CHECK(node == &m.at(1));

    std::list<Counted> l;
    l.emplace_back("first");
    const Counted* cell = &l.front();
    for (int i = 0; i < 16; ++i) l.emplace_back("filler");
    CHECK(cell == &l.front());
}

// The two rows of the appendix's stability column that are neither "no" nor
// a flat "yes" - and the two a C# habit is least prepared for.
void ReferencesSurviveWhereIteratorsDoNot() {
    // unordered_map: a rehash invalidates iterators and spares references.
    std::unordered_map<int, Counted> m;
    m.emplace(1, Counted("first"));
    const Counted* ref = &m.at(1);
    const std::size_t buckets = m.bucket_count();
    for (int i = 2; i < 512; ++i) m.emplace(i, Counted("filler"));
    CHECK(m.bucket_count() != buckets);         // a rehash really happened
    CHECK(ref == &m.at(1));                     // and the reference survived it

    // deque: insertion at either end spares references to existing elements.
    std::deque<Counted> d;
    d.emplace_back("first");
    const Counted* first = &d[0];
    const std::string payload = first->Payload();
    for (int i = 0; i < 200; ++i) {
        d.emplace_back("back");
        d.emplace_front("front");
    }
    CHECK(first == &d[200]);                    // it slid in the index, not in memory
    CHECK(first->Payload() == payload);         // and reading it is not a use-after-free
}

void ThePolymorphicBranchDispatches() {
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Triangle>());
    shapes.push_back(std::make_unique<Square>());
    int total = 0;
    for (const auto& s : shapes) total += s->Sides();
    CHECK(total == 7);                          // 3 + 4: each kept its own identity
}

int main() {
    GrowthRelocatesAndMovesEveryElement();
    BoxedElementsStandStillWhenTheVectorGrows();
    ReserveHoldsAddressesStill();
    NodeBasedContainersNeverMoveAnElement();
    ReferencesSurviveWhereIteratorsDoNot();
    ThePolymorphicBranchDispatches();

    if (Failures() != 0) {
        std::printf("choosing/storing: %d FAILED check(s)\n", Failures());
        return 1;
    }
    std::printf("choosing/storing: growth moves vector elements and leaves "
                "boxed ones alone, exactly as the appendix claims\n");
    return 0;
}
