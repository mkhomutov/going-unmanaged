// The Chapter 15 demo. The class itself is in Buffer.h, so the test binary in
// exercises/testlab/ can include it too (Chapter 28); this file is the entry
// point and nothing else.
#include "Buffer.h"

#include <iostream>
#include <utility>
#include <vector>

int main() {
    Buffer a(5);
    a.At(2) = 42;                    // writable now (Finding 8)

    Buffer b = a;                    // copy ctor: deep copy
    Buffer c(3);
    c = a;                           // copy assignment via copy-and-swap
    std::cout << "c[2]=" << c.At(2) << " (expect 42)\n";

    const Buffer& view = a;
    std::cout << "view[2]=" << view.At(2) << " (const overload)\n";

    Buffer d = std::move(a);         // move ctor: a is now empty husk
    std::cout << "a.Size()=" << a.Size() << " (expect 0)\n";

    c = std::move(d);                // move assignment
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"   // the self-move is the TEST
#endif
    c = std::move(c);                // self-move: must be harmless
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
    std::cout << "c[2]=" << c.At(2) << " after self-move (expect 42)\n";

    std::vector<Buffer> v;
    v.push_back(Buffer(2));
    v.push_back(Buffer(4));          // reallocation: moves (noexcept honest)
    std::cout << "vector ok, sizes " << v[0].Size() << "," << v[1].Size() << "\n";
    return 0;
}
