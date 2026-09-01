// counted.h - a type that counts what happens to it, so the appendix's
// recommendations are numbers rather than assurances.
//
// Quoted IN FULL in Appendix H ("What this costs, counted"). Editing it
// means editing the appendix in the same commit - the cookbook discipline.
// It is Chapter 14's Tracer with the narration removed and the tally kept:
// there you WATCH the special members fire, here a program asserts how
// often. Same lesson, different instrument.
#pragma once
#include <string>
#include <utility>

struct Counts {
    int copies = 0;
    int moves  = 0;
};

inline Counts& Tally() {
    static Counts c;                 // Chapter 28's construct-on-first-use
    return c;
}

inline void ResetTally() { Tally() = Counts{}; }

class Counted {
public:
    // A payload big enough that a copy is a real allocation, not a
    // small-string optimization the measurement would never see.
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
