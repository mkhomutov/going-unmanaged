#pragma once
#include <cstddef>
#include <vector>

// One channel's samples for one tick of the host's meter clock.
struct Block {
    std::vector<float> samples;
};

class Meter {
public:
    explicit Meter(std::size_t channels);

    // Called once per tick on the host's audio thread - the deadline path.
    // Borrows the blocks for the duration of the call: no copy, no
    // allocation, nothing that can block.
    void Tick(const std::vector<Block>& inputs);

    // Running peak for one channel, linear [0, 1].
    float Peak(std::size_t channel) const;

private:
    std::vector<float> peaks_;
};
