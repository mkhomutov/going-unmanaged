#include "wire.h"
#include <cstdio>

// The ticket's capture, byte for byte: two temperature frames. Frame 1
// starts at offset 0, frame 2 at offset 10 - deliberately off every
// four-byte boundary. Decoding at any offset is the fix's claim, and one
// aligned frame cannot prove it.
static const unsigned char kCapture[] = {
    0xa5, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x01, 0x09, 0x29,
    0xa5, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x01, 0x09, 0x2b,
};

int main() {
    // The hand decode from the ICD is the oracle: no tool supplies these.
    const std::uint32_t want_seq[]     = {1, 2};
    const std::uint16_t want_reading[] = {2345, 2347};    // centi-degrees C

    const unsigned char* p = kCapture;
    std::size_t left = sizeof kCapture;
    int frame = 0;
    while (left > 0) {
        Frame f;
        std::size_t advance = 0;
        if (!decode_frame(p, left, f, &advance)) {
            std::printf("frame %d: malformed\n", frame);
            return 1;
        }
        if (frame >= 2 || f.kind != 0x01 || f.length != 2
            || f.sequence != want_seq[frame]
            || read_u16_be(f.payload) != want_reading[frame]) {
            std::printf("frame %d: decoded values do not match the hand decode\n",
                        frame);
            return 1;
        }
        std::printf("frame %d: seq %u, %u.%02u C\n", frame, f.sequence,
                    read_u16_be(f.payload) / 100, read_u16_be(f.payload) % 100);
        p += advance;
        left -= advance;
        ++frame;
    }
    return frame == 2 ? 0 : 1;
}
