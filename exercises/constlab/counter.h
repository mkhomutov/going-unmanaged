// A small const-correct class — Appendix I.
//
// Everything interesting here is about which half of the interface a caller
// reaches through a `const Counter&`. Nothing in it is about counting.
#pragma once

#include <cstddef>
#include <vector>

class Counter {
public:
    // ---- the const half: what a caller holding a const& may call ---------
    std::size_t Size() const { return samples_.size(); }

    // Logically const, and not bitwise const. The average is derived from
    // state that has not changed, so recomputing it on every call would be
    // wasteful and caching it changes nothing a caller can observe - which
    // is exactly the case `mutable` exists for.
    double Average() const {
        if (!cached_) {
            double total = 0;
            for (int s : samples_) total += s;
            average_ = samples_.empty() ? 0.0 : total / static_cast<double>(samples_.size());
            cached_ = true;
            ++recomputes_;              // not reachable through the const half
        }
        return average_;
    }

    // The overload pair. Same name, same arguments, different constness -
    // and the const one hands back a reference the caller cannot write
    // through, which is the whole point of having two.
    const int& At(std::size_t i) const { return samples_[i]; }
    int& At(std::size_t i) { cached_ = false; return samples_[i]; }

    // Test-only, and deliberately NOT const: a probe the const half could
    // reach is a counter `const` has stopped protecting, which is the trap
    // this class exists to avoid.
    int Recomputes() { return recomputes_; }

    // ---- the non-const half: what mutation looks like --------------------
    void Add(int sample) {
        samples_.push_back(sample);
        cached_ = false;               // any mutator invalidates the cache
    }

private:
    std::vector<int> samples_;

    // `mutable` is the escape hatch, and it is narrow on purpose: it says
    // "this member is not part of the value", so writing it inside a const
    // member function is legal. Use it for caches, memo tables and mutexes.
    // Use it for anything a caller could observe and const stops meaning
    // anything at all.
    mutable double average_ = 0.0;
    mutable bool   cached_  = false;
    mutable int    recomputes_ = 0;
};
