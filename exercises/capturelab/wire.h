#pragma once
#include <cstddef>
#include <cstdint>

// The vendor's frame header as the ICD documents it: 8 bytes on the wire,
// network byte order, no padding. The layout lives here as named offsets,
// not as a struct - the wire's layout belongs to the ICD, a struct's to
// the compiler, and neither may impersonate the other.
constexpr std::size_t kHeaderSize  = 8;
constexpr std::size_t kOffSync     = 0;    // u8, always 0xA5
constexpr std::size_t kOffSequence = 1;    // u32, big-endian
constexpr std::size_t kOffLength   = 5;    // u16, big-endian, payload bytes
constexpr std::size_t kOffKind     = 7;    // u8, 0x01 = temperature

std::uint16_t read_u16_be(const unsigned char* p);
std::uint32_t read_u32_be(const unsigned char* p);

// The DECODED frame - host-order values in a struct the compiler lays out
// however it likes, because this struct never touches the wire.
struct Frame {
    std::uint32_t sequence;
    std::uint16_t length;              // payload bytes
    std::uint8_t  kind;
    const unsigned char* payload;      // borrow into the capture buffer
};

// Decode one frame starting at p. Returns false if the sync byte is wrong
// or the frame runs past the end; on success fills out and sets *advance
// to the bytes consumed.
bool decode_frame(const unsigned char* p, std::size_t size,
                  Frame& out, std::size_t* advance);
