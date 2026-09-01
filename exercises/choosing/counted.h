// counted.h - a type that counts what happens to it, so the appendix's
// recommendations are numbers rather than assurances.
//
// Quoted in Appendix H ("What this costs, counted"): `struct Counts` and
// `Tally()` below - those two, whole, and nothing else in this file.
// Editing either means editing the appendix in the same commit (the
// cookbook discipline), and scripts/check_verbatim.sh checks that pairing
// in BOTH directions: every cpp fence on the page must be in this
// directory, and both of those must be on the page, whole.
//
// It is Chapter 14's Tracer in SHAPE only. That Tracer logs, and its two
// statics count objects rather than operations - `counter_` is incremented
// identically by the copy constructor and the move constructor, so nothing
// in it can tell one from the other. This is the counting variant that
// difference forces: per-operation tallies, no narration.
#pragma once
#include <cstdio>
#include <string>
#include <type_traits>
#include <utility>

struct Counts {
    int copies = 0;
    int moves  = 0;
};

inline Counts& Tally() {
    static Counts c;                 // Chapter 32's construct-on-first-use
    return c;
}

inline void ResetTally() { Tally() = Counts{}; }

// The judge. NOT assert(): `assert` compiles to nothing under -DNDEBUG, and
// a CMake Release build (Chapter 26) defines it - so an assert-judged
// harness would print its success line and exit 0 having verified nothing.
// perflab and bridgelab already made this choice; this file follows them.
inline int& Failures() {
    static int n = 0;
    return n;
}

#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::fprintf(stderr, "FAILED %s:%d: %s\n", __FILE__, __LINE__,    \
                         #cond);                                              \
            ++Failures();                                                     \
        }                                                                     \
    } while (0)

class Counted {
public:
    // Past every implementation's small-string optimization, so a copy is
    // always a real allocation. passing.cpp counts those too - the cost
    // copies-and-moves cannot see, and the one that decides a setter.
    explicit Counted(const char* name = "x") : payload_(200, 'a') { payload_ += name; }

    Counted(const Counted& other) : payload_(other.payload_) { ++Tally().copies; }
    Counted& operator=(const Counted& other) {
        payload_ = other.payload_;
        ++Tally().copies;
        return *this;
    }

    // noexcept, or vector reallocation would COPY these instead of moving
    // them (Chapter 6) - and this file exists to count that difference.
    Counted(Counted&& other) noexcept : payload_(std::move(other.payload_)) { ++Tally().moves; }
    Counted& operator=(Counted&& other) noexcept {
        payload_ = std::move(other.payload_);
        ++Tally().moves;
        return *this;
    }

    const std::string& Payload() const { return payload_; }

private:
    std::string payload_;
};

// The one check in this lab that survives -DNDEBUG and an optimizer: it
// names the property (Chapter 6's noexcept move) instead of the element
// count that happens to follow from it.
static_assert(std::is_nothrow_move_constructible<Counted>::value,
              "Counted's move must be noexcept, or vector growth copies");
