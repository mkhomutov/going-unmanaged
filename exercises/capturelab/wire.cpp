#include "wire.h"

std::uint16_t read_u16_be(const unsigned char* p) {
    return static_cast<std::uint16_t>((std::uint32_t(p[0]) << 8) | p[1]);
}

std::uint32_t read_u32_be(const unsigned char* p) {
    return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16)
         | (std::uint32_t(p[2]) << 8)  |  std::uint32_t(p[3]);
}

bool decode_frame(const unsigned char* p, std::size_t size,
                  Frame& out, std::size_t* advance) {
    if (size < kHeaderSize || p[kOffSync] != 0xA5) {
        return false;
    }
    out.sequence = read_u32_be(p + kOffSequence);
    out.length   = read_u16_be(p + kOffLength);
    out.kind     = p[kOffKind];
    if (size - kHeaderSize < out.length) {
        return false;    // payload would run past the capture
    }
    out.payload = p + kHeaderSize;
    if (advance != nullptr) {
        *advance = kHeaderSize + out.length;
    }
    return true;
}
