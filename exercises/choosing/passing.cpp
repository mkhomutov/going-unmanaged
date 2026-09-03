// Appendix H, procedures 2 and 3 - how to take a parameter, what to return -
// plus two of Chapter 6's value-category traps, priced with the same
// instrument.
//
// Quoted in Appendix H, whole and by name: `class Widget`, `MakeTemporary`,
// `MakeNamed`, `TheSinkAllocatesWhereTheBorrowDoesNot` and
// `ReturningCostsNoCopy`. Quoted in Chapter 6 ("Value categories in one
// table"), whole and by name: `MakeNamedMoved`, `MovingFromAConstObjectCopies`
// and `ReturnStdMoveCostsTheMoveElisionRemoved`. Editing a named unit means
// editing its page in the same commit (the cookbook discipline), and
// scripts/check_verbatim.sh holds every pairing in BOTH directions - each
// unit named here must be on its page whole, and every cpp fence on Appendix
// H must be in this directory. Everything else here, main() included,
// appears in no listing.
#include <cstdio>
#include <cstdlib>
#include <new>
#include <utility>
#include <vector>

#include "counted.h"

// The second instrument, borrowed from perflab: a heap-allocation counter.
// Copies and moves are not the whole cost of a signature - a by-value sink
// cannot reuse the destination's buffer, and a const& copy-assignment can.
// That difference is invisible to Tally() and decides the setter case, so
// this file counts it rather than leaving the appendix to assert it.
namespace {
long g_allocs = 0;
}
long Allocations() { return g_allocs; }

void* operator new(std::size_t n) {
    ++g_allocs;
    if (void* p = std::malloc(n)) {
        return p;
    }
    throw std::bad_alloc{};
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

// ---- procedure 2: the sink -------------------------------------------------
// "This function KEEPS a copy." By value plus std::move: one move from an
// rvalue caller, one copy plus one move from an lvalue caller - and the
// caller chooses which by what they pass, without a second overload.
class Widget {
public:
    void SetPayload(Counted c) { payload_ = std::move(c); }

    // The const& alternative, for comparison. It cannot steal: assigning
    // from a const reference copies, whatever the caller passed.
    void SetPayloadByRef(const Counted& c) { payload_ = c; }

private:
    Counted payload_;
};

// ---- procedure 2: the borrower ---------------------------------------------
// "I only look at it, and only during the call." No copy, no ownership.
std::size_t PayloadSize(const Counted& c) { return c.Payload().size(); }

// ---- procedure 3: returning ------------------------------------------------
// A temporary returned outright: C++17 elides this into the caller's
// storage. Not "optimized usually" - required, and no copy or move
// constructor need even exist. build_all.sh builds this file a second time
// under -fno-elide-constructors, where this function's count is unchanged
// and MakeNamed's is not: that is the whole difference, made visible.
Counted MakeTemporary() { return Counted("made"); }

// A NAMED local: the return is treated as an rvalue, so the fallback is a
// MOVE, never a copy. NRVO may remove even that - permitted, not
// guaranteed, which is exactly why the assertion below allows either.
Counted MakeNamed() {
    Counted local("named");
    return local;
}

// ---- Chapter 6: value categories, priced -----------------------------------
// return std::move(local) casts a candidate for NRVO into a plain rvalue:
// the compiler may no longer build `local` in the caller's storage, so the
// move it would have elided now always happens. clang and GCC both say so
// (-Wpessimizing-move, in -Wall); the pragma exists because this file's job
// is to measure the thing the warning is about.
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpessimizing-move"
#endif
Counted MakeNamedMoved() {
    Counted local("named");
    return std::move(local);             // the pessimizing move
}
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

// Every cell of the appendix's cost table, including the const& column,
// whose lower two rows the table claims and no earlier version measured.
void SinkCostsPerCallerKind() {
    {   // from a temporary: one move, no copy
        Widget w;
        ResetTally();
        w.SetPayload(Counted("temp"));
        CHECK(Tally().copies == 0);
        CHECK(Tally().moves  == 1);
    }
    {   // from an lvalue the caller keeps: one copy, one move
        Widget w;
        Counted keep("mine");
        ResetTally();
        w.SetPayload(keep);              // copy into the parameter, move in
        CHECK(Tally().copies == 1);
        CHECK(Tally().moves  == 1);
        CHECK(keep.Payload().size() > 0);    // the caller's object survives
    }
    {   // from an lvalue the caller is done with: two moves
        Widget w;
        Counted done("done");
        ResetTally();
        w.SetPayload(std::move(done));   // std::move is a cast, not a move
        CHECK(Tally().copies == 0);
        CHECK(Tally().moves  == 2);      // one INTO the parameter, one out of it
    }
    {   // const& from a temporary: a copy, because it cannot steal
        Widget w;
        ResetTally();
        w.SetPayloadByRef(Counted("temp"));
        CHECK(Tally().copies == 1);      // the branch the sink exists to beat
        CHECK(Tally().moves  == 0);
    }
    {   // const& from an lvalue the caller keeps: the same one copy
        Widget w;
        Counted keep("mine");
        ResetTally();
        w.SetPayloadByRef(keep);
        CHECK(Tally().copies == 1);
        CHECK(Tally().moves  == 0);
    }
    {   // const& from std::move(x): still a copy - the cast buys nothing
        Widget w;
        Counted done("done");
        ResetTally();
        w.SetPayloadByRef(std::move(done));
        CHECK(Tally().copies == 1);
        CHECK(Tally().moves  == 0);
    }
}

// The cost the tally cannot see, and the one that reverses the advice for
// a setter called over and over: the sink allocates a fresh buffer every
// call, while copy-assignment through const& reuses the member's.
void TheSinkAllocatesWhereTheBorrowDoesNot() {
    Counted keep("mine");
    Widget sink;
    Widget borrow;
    sink.SetPayload(keep);              // warm both members to the same
    borrow.SetPayloadByRef(keep);       // size, so only the steady state counts

    const long a0 = Allocations();
    for (int i = 0; i < 100; ++i) sink.SetPayload(keep);
    const long sink_allocs = Allocations() - a0;

    const long b0 = Allocations();
    for (int i = 0; i < 100; ++i) borrow.SetPayloadByRef(keep);
    const long borrow_allocs = Allocations() - b0;

    CHECK(sink_allocs == 100);      // one per call: the parameter is a new string
    CHECK(borrow_allocs == 0);      // the member's buffer was big enough already
}

void BorrowingCostsNothing() {
    Counted c("borrowed");
    ResetTally();
    const std::size_t n = PayloadSize(c);
    CHECK(n > 0);
    CHECK(Tally().copies == 0 && Tally().moves == 0);
}

// Chapter 2's Trap 1, priced: the missing & is one silent copy per element.
void TheMissingAmpersandCostsNCopies() {
    std::vector<Counted> v;
    v.reserve(4);
    for (int i = 0; i < 4; ++i) v.emplace_back("item");

    ResetTally();
    for (auto c : v) (void)c.Payload();          // the accidental copy
    const int by_value = Tally().copies;

    ResetTally();
    for (const auto& c : v) (void)c.Payload();   // the idiom
    const int by_ref = Tally().copies;

    CHECK(by_value == 4);       // one per element, silently
    CHECK(by_ref   == 0);
}

// The two return shapes, asserted differently because only one of them is
// guaranteed. Under -fno-elide-constructors the first is unchanged and the
// second costs its move - which is what "permitted, not required" means.
void ReturningCostsNoCopy() {
    {   // a temporary: nothing happens at all
        ResetTally();
        Counted c = MakeTemporary();
        CHECK(Tally().copies == 0);
        CHECK(Tally().moves  == 0);      // mandatory elision, C++17
        (void)c;
    }
    {   // a named local: never a copy; a move at worst
        ResetTally();
        Counted c = MakeNamed();
        CHECK(Tally().copies == 0);      // guaranteed: the implicit move
        CHECK(Tally().moves  <= 1);      // 0 with NRVO, 1 without - both legal
        (void)c;
    }
}

// A const object cannot be stolen from. std::move casts it to const&&, and
// nothing a class writes takes const&& - so overload resolution falls back
// to the copy constructor. No warning, no error: one silent copy, and the
// source untouched.
void MovingFromAConstObjectCopies() {
    const Counted keep("const");
    ResetTally();
    Counted taken = std::move(keep);     // reads as a move, is a copy
    CHECK(Tally().copies == 1);
    CHECK(Tally().moves  == 0);
    CHECK(keep.Payload().size() > 0);    // nothing was taken from it
    (void)taken;
}

// The move that elision would have removed, measured on both build passes:
// ReturningCostsNoCopy allows MakeNamed zero or one move, because NRVO is
// permitted; MakeNamedMoved is always exactly one, because the cast forbade
// it. -fno-elide-constructors changes the first and not the second.
void ReturnStdMoveCostsTheMoveElisionRemoved() {
    ResetTally();
    Counted c = MakeNamedMoved();
    CHECK(Tally().copies == 0);
    CHECK(Tally().moves  == 1);          // always one: NRVO was cast away
    (void)c;
}

void ReturningAVectorCopiesNoElement() {
    ResetTally();
    std::vector<Counted> v = [] {
        std::vector<Counted> out;
        out.reserve(3);
        for (int i = 0; i < 3; ++i) out.emplace_back("bulk");
        return out;                      // the whole vector, by value
    }();
    CHECK(v.size() == 3);
    CHECK(Tally().copies == 0);          // no element was copied out
    CHECK(Tally().moves  == 0);          // nor moved: a vector move is 3 pointers
}

int main() {
    SinkCostsPerCallerKind();
    TheSinkAllocatesWhereTheBorrowDoesNot();
    BorrowingCostsNothing();
    TheMissingAmpersandCostsNCopies();
    ReturningCostsNoCopy();
    MovingFromAConstObjectCopies();
    ReturnStdMoveCostsTheMoveElisionRemoved();
    ReturningAVectorCopiesNoElement();

    if (Failures() != 0) {
        std::printf("choosing/passing: %d FAILED check(s)\n", Failures());
        return 1;
    }
    std::printf("choosing/passing: every branch of procedures 2 and 3 costs "
                "what the appendix says, and Chapter 6's two traps what it says\n");
    return 0;
}
