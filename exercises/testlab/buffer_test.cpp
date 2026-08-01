// The Chapter 28 suite, quoted in full in that chapter's "Testing the Rule of
// Five". Changing it means updating the listing in the same commit.
//
// Buffer.h is the Chapter 15 solution's class, extracted so both the demo
// (solutions/buffer.cpp) and this binary can include it; build_all.sh passes
// -I solutions.
#include "Buffer.h"
#include "tiny_test.h"
#include <vector>

TEST(ConstructorZeroInitializes) {
    Buffer a(4);
    CHECK(a.Size() == 4);
    CHECK(a.At(0) == 0);             // new int[n]{} zero-fills (Finding 7)
}

TEST(CopyIsDeepNotShallow) {
    Buffer a(3);
    a.At(1) = 42;
    Buffer copy = a;
    copy.At(1) = 99;                 // if this were a shallow copy...
    CHECK(a.At(1) == 42);            // ...the original would read 99
    CHECK(copy.At(1) == 99);
}

TEST(CopyAssignAcrossSizes) {
    Buffer a(2);
    Buffer b(5);
    b.At(4) = 7;
    a = b;                           // the old, smaller block must be freed
    CHECK(a.Size() == 5);
    CHECK(a.At(4) == 7);
}

TEST(SelfAssignmentIsHarmless) {
    Buffer a(2);
    a.At(0) = 5;
    const Buffer& alias = a;         // launder it past the compiler's warning
    a = alias;
    CHECK(a.Size() == 2);
    CHECK(a.At(0) == 5);             // copy-and-swap makes this free
}

TEST(MoveLeavesSourceEmptyButValid) {
    Buffer a(3);
    a.At(2) = 11;
    Buffer moved = std::move(a);
    CHECK(moved.Size() == 3);
    CHECK(moved.At(2) == 11);
    CHECK(a.Size() == 0);            // the husk: valid, unspecified, destructible
}

TEST(VectorReallocationPreservesContents) {
    std::vector<Buffer> v;
    for (int i = 0; i < 8; ++i) {    // force at least one reallocation
        Buffer b(2);
        b.At(0) = i;
        v.push_back(std::move(b));
    }
    for (int i = 0; i < 8; ++i) CHECK(v[static_cast<size_t>(i)].At(0) == i);
}

int main() { return tiny::RunAll(); }
