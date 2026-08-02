// scorer.cpp - the implementation the caller never sees a declaration of.
//
// Chapter 30 describes this file rather than printing it: "the implementation
// lives entirely in your .cpp, in an anonymous namespace, and never appears in
// a header". This is that sentence compiled. Changing the shape means checking
// the chapter's prose still describes it.
#include "IScorer.h"

namespace {

// `final` is not decoration: without it, `delete this` below is
// -Wdelete-non-virtual-dtor, because a class with virtual functions and a
// non-virtual destructor is a hazard for anything that might derive from it.
// Nothing can derive from this one, and nothing outside this file can even name
// it - the anonymous namespace gives it internal linkage, so it has no external
// symbol to collide with a caller's, whatever they happen to have called their
// own types.
class Scorer final : public IScorer {
public:
    explicit Scorer(int seed) : seed_(seed) {}
    int  Score() const override { return seed_ * 2; }
    // The other half of the whoever-allocates-frees rule: this delete runs in
    // the library, so it uses the library's allocator - the one that ran the
    // new in CreateScorer.
    void Destroy() override { delete this; }
private:
    int seed_;
};

}  // namespace

// extern "C" for the name, not for the language: it suppresses mangling so the
// caller (or a dlsym/GetProcAddress) can find one predictable symbol. The body
// is ordinary C++ and returns a C++ object.
extern "C" IScorer* CreateScorer(int seed) { return new Scorer(seed); }
