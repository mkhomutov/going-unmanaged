#include "meter.h"
#include <algorithm>
#include <cassert>
#include <cmath>

Meter::Meter(std::size_t channels) : peaks_(channels, 0.0f) {}

void Meter::Tick(const std::vector<Block>& inputs) {
    assert(inputs.size() == peaks_.size());
    std::size_t ch = 0;
    for (const auto& block : inputs) {          // borrow - the second &
        float peak = peaks_[ch];
        for (const float s : block.samples) {   // a float: by value on purpose
            peak = std::max(peak, std::fabs(s));
        }
        peaks_[ch] = peak;
        ++ch;
    }
}

float Meter::Peak(std::size_t channel) const {
    assert(channel < peaks_.size());
    return peaks_[channel];
}
