#include "session.h"

Session::Session(bool calibrated) {
    if (calibrated) {
        cal_ = std::make_unique<Calibration>();
        cal_->scale  = 0.5;     // the capability's factory constants
        cal_->offset = 1.0;
    }
}

void Session::Ingest(double reading) {
    sum_ += reading;
    ++count_;
}

double Session::FoldedMean() const {
    const double mean = count_ > 0 ? sum_ / static_cast<double>(count_) : 0.0;
    if (cal_ == nullptr) {                 // the field configuration: no pack,
        return mean;                       // so the mean ships uncalibrated
    }
    cal_->folded += count_;                // bookkeeping the pack expects
    return mean * cal_->scale + cal_->offset;
}

double Session::Report() const {
    return FoldedMean();
}
