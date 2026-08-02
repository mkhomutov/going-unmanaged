// IScorer.h
//
// Quoted IN FULL in Chapter 30 ("Technique 2 - a pure-virtual interface and a
// factory"). Changing it means updating that listing in the same commit.
//
// No data members, so nothing whose layout the compiler chose crosses the line
// - only a vtable shape and one unmangled symbol to find. Read the chapter for
// what that costs: a published vtable is append-only, and only when YOU are the
// sole implementer.
#pragma once
class IScorer {
public:
    virtual int  Score() const = 0;
    virtual void Destroy() = 0;      // the LIBRARY frees it, with its own allocator
protected:
    ~IScorer() = default;            // non-virtual AND protected: no delete through this
};
extern "C" IScorer* CreateScorer(int seed);   // one unmangled symbol to find
