// Appendix H, procedures 2 and 3 - how to take a parameter, what to return.
//
// The Sink/Viewer/etc. functions below are quoted in the appendix: editing
// one means editing Appendix H in the same commit (the cookbook
// discipline). main() is scaffolding - it asserts what each procedure's
// branches claim, so the advice cannot rot into folklore.
#include <cassert>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "counted.h"

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
// constructor need even exist.
Counted MakeTemporary() { return Counted("made"); }

// A NAMED local: the return is treated as an rvalue, so the fallback is a
// MOVE, never a copy. NRVO may remove even that - permitted, not
// guaranteed, which is exactly why the assertion below allows either.
Counted MakeNamed() {
    Counted local("named");
    return local;
}

int main() {
    // --- the sink, from an rvalue: one move, no copy ------------------------
    {
        Widget w;
        ResetTally();
        w.SetPayload(Counted("temp"));
        assert(Tally().copies == 0);
        assert(Tally().moves  == 1);
    }
    // --- the sink, from an lvalue the caller keeps: one copy, one move ------
    {
        Widget w;
        Counted keep("mine");
        ResetTally();
        w.SetPayload(keep);              // copy into the parameter, move in
        assert(Tally().copies == 1);
        assert(Tally().moves  == 1);
        assert(keep.Payload().size() > 0);   // the caller's object survives
    }
    // --- the sink, from an lvalue the caller is done with: one move --------
    {
        Widget w;
        Counted done("done");
        ResetTally();
        w.SetPayload(std::move(done));   // std::move is a cast, not a move
        assert(Tally().copies == 0);
        assert(Tally().moves  == 2);     // one INTO the parameter, one out of it
    }
    // --- const& into a member: always a copy, even from a temporary ---------
    {
        Widget w;
        ResetTally();
        w.SetPayloadByRef(Counted("temp"));
        assert(Tally().copies == 1);     // the branch the sink exists to beat
        assert(Tally().moves  == 0);
    }
    // --- borrowing costs nothing -------------------------------------------
    {
        Counted c("borrowed");
        ResetTally();
        assert(PayloadSize(c) > 0);
        assert(Tally().copies == 0 && Tally().moves == 0);
    }
    // --- the loop: the missing & is N copies --------------------------------
    {
        std::vector<Counted> v;
        v.reserve(4);
        for (int i = 0; i < 4; ++i) v.emplace_back("item");

        ResetTally();
        for (auto c : v) (void)c.Payload();          // Chapter 2's Trap 1
        const int by_value = Tally().copies;

        ResetTally();
        for (const auto& c : v) (void)c.Payload();   // the idiom
        const int by_ref = Tally().copies;

        assert(by_value == 4);       // one per element, silently
        assert(by_ref   == 0);
    }
    // --- returning a temporary: nothing happens at all ----------------------
    {
        ResetTally();
        Counted c = MakeTemporary();
        assert(Tally().copies == 0);
        assert(Tally().moves  == 0);     // mandatory elision, C++17
        (void)c;
    }
    // --- returning a named local: never a copy; a move at worst -------------
    {
        ResetTally();
        Counted c = MakeNamed();
        assert(Tally().copies == 0);         // guaranteed: the implicit move
        assert(Tally().moves  <= 1);         // 0 with NRVO, 1 without - both legal
        (void)c;
    }
    // --- returning a big collection is not a copy either --------------------
    {
        ResetTally();
        std::vector<Counted> v = [] {
            std::vector<Counted> out;
            out.reserve(3);
            for (int i = 0; i < 3; ++i) out.emplace_back("bulk");
            return out;                      // the whole vector, by value
        }();
        assert(v.size() == 3);
        assert(Tally().copies == 0);         // no element was copied out
    }

    std::printf("choosing/passing: every branch of procedures 2 and 3 costs "
                "what the appendix says\n");
    return 0;
}
