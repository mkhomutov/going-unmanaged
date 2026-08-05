#pragma once
#include <memory>

// The optional calibration pack: present only when the device reports the
// capability. THE INVARIANT: cal_ may be null for an entire session, and
// every path that touches it owns the check.
struct Calibration {
    double scale  = 1.0;
    double offset = 0.0;
    long   folded = 0;     // readings folded through this pack so far
};

class Session {
public:
    explicit Session(bool calibrated);

    void Ingest(double reading);
    double Report() const;                 // session close: the mean, calibrated
    bool Calibrated() const { return cal_ != nullptr; }

private:
    double FoldedMean() const;             // applies the pack to the mean

    double sum_   = 0.0;
    long   count_ = 0;
    std::unique_ptr<Calibration> cal_;     // null when the capability is absent
};
