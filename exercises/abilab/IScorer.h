// IScorer.h is included IN FULL by Chapter 30 ("Technique 2 - a pure-virtual
// interface and a factory") from the marker below: edit there and the page
// follows.
//
// No data members, so nothing whose layout the compiler chose crosses the line
// - only a vtable shape and one unmangled symbol to find. Read the chapter for
// what that costs: a published vtable is append-only, and only when YOU are the
// sole implementer.
// --8<-- [start:listing]
// IScorer.h
#pragma once
class IScorer {
public:
    virtual int  Score() const = 0;
    virtual void Destroy() = 0;      // the LIBRARY frees it, with its own allocator
protected:
    ~IScorer() = default;            // non-virtual AND protected: no delete through this
};
extern "C" IScorer* CreateScorer(int seed);   // one unmangled symbol to find
// --8<-- [end:listing]
