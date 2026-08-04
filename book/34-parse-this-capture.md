## Chapter 34 — Parse This Capture

The third ticket-shaped chapter, and this one takes something away. Chapters 32 and 33 both ended with a sanitizer naming the crime; here the handbook's flags stay green from the first run to the last, because none of what is wrong in this ticket — and three separate things are wrong — is anything a sanitizer checks. The evidence attached this time is a bus capture and the vendor's own header table, and the oracle is you: the ticket is solved on paper, by decoding the bytes by hand, before the compiler contributes anything but confirmation.

### The ticket

> **#5347 — Ingest rejects every capture from the new analyzer (bring-up).** The new bus analyzer's telemetry feed is being brought up. The vendor's own viewer opens every capture file fine; our ingest calls every one of them malformed. The developer who started the bring-up notes that the sync byte parses fine — and that the header's kind field comes back 165 on a frame the vendor's viewer calls a temperature reading (kind 1). A minimal capture is attached, two frames straight off the bus, along with the ICD's header table.

A bring-up ticket, not a regression: nothing used to work, so there is no diff to suspect and no "what changed?" to ask. All you have is a document, twenty bytes, and a parser that disagrees with both.

### The wire, as the ICD documents it

| Offset | Size | Field | Notes |
|---|---|---|---|
| 0 | 1 | sync | always `0xA5` |
| 1 | 4 | sequence | big-endian |
| 5 | 2 | payload length | big-endian; payload bytes after the header |
| 7 | 1 | kind | `0x01` = temperature; payload is a u16, centi-°C |

> Multi-byte fields are network byte order (big-endian). The header is packed: 8 bytes, no padding. — *ICD, section 4.2*

And the attached capture, twenty bytes, two frames:

```text
a5 00 00 00 01 00 02 01 09 29 a5 00 00 00 02 00
02 01 09 2b
```

### The code it happened to

One file. The struct is the ICD table, transcribed — which is exactly what makes it wrong, and exactly why it looked right in review:

```cpp
#include <cstddef>
#include <cstdint>
#include <cstdio>

// The frame header, straight from the ICD table ("8-byte header, network
// byte order, no padding" - section 4.2)
struct Header {
    std::uint8_t  sync;          // offset 0: always 0xA5
    std::uint32_t sequence;      // offset 1
    std::uint16_t payload_len;   // offset 5: payload bytes after the header
    std::uint8_t  kind;          // offset 7: 0x01 = temperature
};

static const unsigned char kCapture[] = {
    0xa5, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x01, 0x09, 0x29,
    0xa5, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x01, 0x09, 0x2b,
};

int main() {
    const unsigned char* p = kCapture;
    std::size_t left = sizeof kCapture;
    int frame = 0;
    while (left >= sizeof(Header)) {
        const Header* h = reinterpret_cast<const Header*>(p);    // map it onto the bytes
        if (h->sync != 0xA5) {
            std::printf("frame %d: bad sync\n", frame);
            return 1;
        }
        std::printf("frame %d: seq %u, %u payload bytes, kind %u\n",
                    frame, h->sequence, h->payload_len, h->kind);
        if (h->payload_len > left - sizeof(Header)) {
            std::printf("frame %d: payload runs past the capture - malformed, stopping\n",
                        frame);
            return 1;
        }
        p += sizeof(Header) + h->payload_len;
        left -= sizeof(Header) + h->payload_len;
        ++frame;
    }
    return 0;
}
```

It compiles clean under `-Wall -Wextra`. Under the full canonical flags it runs *identically* — not one report. The output, plain or sanitized, on this machine:

```text
frame 0: seq 16908289, 10505 payload bytes, kind 165
frame 0: payload runs past the capture - malformed, stopping
```

Chapter 9's cast table filed `reinterpret_cast` under "serialization/interop only" and called it a code-review question mark. This chapter is that question mark, cashed: the one line above that contains it is where all three of the ticket's bugs live.

### Try it — before reading on

The ticket card is `exercises/capturelab/TASK.md`, with the table, the capture and the broken listing together (the files beside the card are the fixed reference — no peeking). Paper first, and this time that is not a discipline exercise — it is the only oracle there is:

1. **Decode the capture by hand.** From the ICD table alone, write down every field of both frames: sequence, payload length, kind, temperature. Nothing in the toolchain will ever hand you these numbers; this paper is what "correct" means for the rest of the ticket.
2. **Find the 165.** `165` is `0xA5`. Locate it in the dump. Which byte is the struct actually reading for `kind`, and what does that say about where the struct believes the fields live?
3. **Predict `sizeof(Header)`, then print it.** The ICD says 8. Print the member offsets too (`offsetof`) and set them beside the table's.
4. **Apply the tempting fix.** `#pragma pack(push, 1)` around the struct, rerun. The new numbers are still wrong — write 16777216 out in hex before reading on.
5. **Write the honest fix and hold it to the paper.** Named offsets, readers that spell the wire's byte order. `scripts/check.sh` green is necessary and nowhere near sufficient: the decode must match your step-1 paper, field for field, both frames.
6. **Stretch: give the sanitizer its one line.** Overlay the *second* frame — it starts at offset 10 — with the unpacked struct under the canonical flags. UBSan speaks its only sentence in this entire ticket. Then pack the struct and watch the same run go quiet while the value stays wrong.

### The diagnosis, walked through

<details>
<summary>Show the walkthrough — decode the capture by hand first</summary>

The hand decode first, because everything else is measured against it. Frame 1: sync `a5`; sequence `00 00 00 01` = 1; length `00 02` = 2; kind `01` = temperature; payload `09 29` = 0x0929 = 2345 centi-degrees — 23.45 °C. Frame 2, starting at byte 10: sequence 2, length 2, kind 1, `09 2b` = 23.47 °C. Twenty bytes, fully accounted for. The vendor's viewer is right.

Now the program's claim: sequence 16908289, length 10505, kind 165. Start with the field you can *find*: 165 is `0xA5`, and the only `a5` bytes in this capture are the two sync bytes, at offsets 0 and 10. The struct read its `kind` — documented at offset 7 — from offset 10, the next frame's sync byte. Three bytes too far. The other two fields confirm the drift: 16908289 is `0x01020001`, which is bytes 4–7 of the capture (`01 00 02 01`) read as one little-endian integer — the struct fetched its four-byte `sequence` from offset 4, not offset 1.

Print what the compiler actually built: `sizeof(Header)` is **12**, and `offsetof` reports sync at 0, sequence at **4**, payload length at **8**, kind at **10**. Set beside the ICD's 0, 1, 5, 7, the mechanism is plain: a `std::uint32_t` must sit at an address divisible by four, so the compiler slid `sequence` from offset 1 to offset 4 and paid three bytes of **padding** for it; everything after slid too, and two more bytes of tail padding rounded the whole struct to 12. The ICD's "no padding" was a statement about the wire. The compiler never read the ICD. `sizeof` answers a question about *your compiler's* layout, and the ticket needed a question about *the document's*.

So: `#pragma pack(1)`, the fix the whole industry reaches for first. `sizeof` becomes 8, the offsets match the table, and the run now prints:

```text
frame 0: seq 16777216, 512 payload bytes, kind 1
frame 0: payload runs past the capture - malformed, stopping
```

`kind` is right — it is one byte, and one-byte fields have no byte order to get wrong. The rest is still nonsense, but *patterned* nonsense: 16777216 is `0x01000000` — the hand decode's 1, mirrored; 512 is `0x0200` — the mirrored 2. The wire is big-endian, as section 4.2 said in its first sentence; this host is little-endian and the struct overlay reads every multi-byte field in the host's order. Two independent lies, and the first was hiding the second: while the offsets were wrong, the values were noise and the byte order was invisible. Only after packing did the mirror become visible. That layering is worth remembering — **padding scrambles, endianness mirrors**, and you cannot see the mirror until the scramble is fixed.

And beneath both lies, the cast itself was never legal. The standard lets an `unsigned char*` read the bytes of any object; it does not grant the reverse, and a `Header*` aimed at a byte buffer violates **strict aliasing** — undefined behavior that nothing in `-fsanitize=address,undefined` looks for, no matter how the run goes. Alignment is the third strand of the same rope: frame 2 begins at offset 10, so its overlaid `sequence` would be a four-byte load from an address divisible by two, not four — that one, uniquely in this ticket, UBSan *will* name (`member access within misaligned address ... which requires 4 byte alignment`) if the parser ever limps that far. This one never does; it dies on frame 0's length check, in silence.

Which is the real finding of this ticket, beyond its two mechanisms: every read here was in bounds, aligned by luck on frame 0, and wrong. The teacher of the last two chapters has nothing to say. What convicted the parser was twenty bytes decoded by hand against a table — and that oracle was in the ticket from the start.

</details>

### What the contract actually says

Names for the notes file: **struct padding** and **endianness** — the two canonical bugs of byte-level protocol work, both invisible from C#, where layout is the runtime's problem.

The compiler lays out a struct for the CPU, not for the wire: each member goes at the next address matching its own alignment (a `std::uint32_t` at a multiple of four), gaps appear wherever the declaration order forces them, and the total is rounded up so the struct can sit in an array. All of it varies by ABI — which is precisely why Chapter 30 banned compiler-laid-out types from crossing a binary boundary. A wire format is the opposite kind of thing: a list of offsets fixed by a document, identical on every machine that will ever parse it. The two can be made to coincide — that is what `#pragma pack(1)` does — but coinciding is not the same as being the same thing, and byte order is where the difference resurfaces: the wire's document picks one order for its multi-byte fields (network protocols traditionally big-endian, most device buses little-endian), while your host has its own opinion, applied silently by every load the struct overlay performs.

> [!NOTE]
> **Surprise for C# devs:** you have never seen a struct pad, because day-to-day C# never shows you an object's layout — no addresses, no overlay casts, and the JIT free to arrange fields as it likes. The one API that admits machines disagree is the confession you may have met: `BitConverter.IsLittleEndian`. Modern .NET went further and put the answer in the method name — `BinaryPrimitives.ReadUInt32BigEndian` — which is exactly the discipline this chapter teaches: say the wire's order at every read, and the host's stops mattering.

The third contract is subtler and has no tool behind it at all: **strict aliasing**. `unsigned char*` may inspect the bytes of anything; pointing a `Header*` at a byte buffer is the reverse direction, and it is undefined behavior even when the offsets and the byte order happen to line up — license the optimizer can and does use. No sanitizer in this book's toolchain checks it. The legal spellings are `std::memcpy` into an object, or — better, because it dissolves the padding and endianness questions in the same motion — not overlaying at all.

### The fix: the wire gets offsets, the struct gets the results

The committed lab replaces the overlay with two small commitments. First, the wire's layout becomes *data about the document* — named offsets, cited to the ICD, in one place:

```cpp
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
```

Second, the readers spell the wire's byte order — with shifts, not with casts:

```cpp
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
```

Look at what those readers do *not* contain: no host-order detection, no `#if` on the architecture, no byte-swap called conditionally. `p[0]` is the high byte because *the document says so*, and the same expression computes the same value on a little-endian laptop, a big-endian switch fabric, or anything else — the code states the wire's order and thereby stops depending on the host's. Note also that `Frame` is still a struct: structs are fine as *destinations*, laid out however the compiler pleases, precisely because a decoded result never touches the wire. Only the overlay was the sin.

The fixed `main` decodes the ticket's own capture and asserts every field of both frames against the hand decode from the fold — which is the honest acceptance test, since no tool in the chain knows what these bytes mean:

```cpp
// The ticket's capture, byte for byte: two temperature frames. Frame 1
// starts at offset 0, frame 2 at offset 10 - deliberately off every
// four-byte boundary. Decoding at any offset is the fix's claim, and one
// aligned frame cannot prove it.
static const unsigned char kCapture[] = {
    0xa5, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x01, 0x09, 0x29,
    0xa5, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x01, 0x09, 0x2b,
};
```

`build_all.sh` runs exactly that on every push. The two earlier ticket labs proved their fixes by varying what the bug depended on — link order, growth — and this one does the same: the capture keeps one frame aligned and one not, because offset-independence is part of what "decoded correctly" means for a stream.

### Pitfalls

- **`#pragma pack` is a treaty, not a fix.** It makes the compiler imitate the document's offsets — and changes nothing else: the values stay mirrored, the cast stays illegal, and it *silences the one report you had* — a packed struct's alignment requirement drops to one, so the misaligned overlay that UBSan named now runs quietly and prints 33554432 (verified: the mirrored 2). Packing has a legitimate job — matching a layout an SDK's own headers declare, Chapter 16 territory — but as a wire-format tool it converts a loud wrong into a quiet one.
- **`memcpy` fixes the crime, not the testimony.** Copying the bytes into a (packed) struct is the legal spelling of the overlay — aliasing solved, alignment solved — and every multi-byte field is still in the wire's order, waiting for a per-field swap you now owe anyway. At that point the field-wise readers were fewer moving parts telling fewer lies.
- **The tell is in *which* fields are wrong.** One-byte fields correct while multi-byte fields are absurd: byte order. Everything after the first multi-byte field shifted, with values findable elsewhere in the dump: padding. The ticket's `kind 165` — a value sitting in plain sight two bytes from where it belonged — was the padding diagnosis, attached, like Chapter 33's workaround nobody decoded.
- **"It works on the bench" proves the bench.** A little-endian wire, a little-endian host, and a layout that happens not to pad: the overlay decodes perfectly and ships. Part of that is undefined behavior waiting on an optimizer (the aliasing), part is mere non-portability waiting on a port (the order) — and both arrive years later as "the new toolchain misparses" with no diff to blame. Right-by-coincidence does not announce itself; Chapter 3 said it about memory, and it is just as true about layout.

> [!TIP]
> **Key principle:** "A struct's layout belongs to my compiler and a frame's belongs to the wire — I never overlay one on the other. I decode at documented offsets with readers that spell the wire's byte order, and the same code is right on every host."

### In the wild

The oldest confession in the field is the sockets API's `htons`/`ntohl` family — "host to network short" has been converting byte order since before most of today's hosts existed, and its existence is the admission that the two orders were never going to agree. Device work meets both dialects at once: USB descriptors are little-endian, network protocols big-endian, and a gateway box parses each in the same afternoon — which is why the byte-order sentence is the first thing to find in any ICD, the practical sibling of Chapter 16's four questions. The industrial answer to this whole chapter is the serialization library — protobuf, FlatBuffers, Cap'n Proto exist in no small part because hand-rolled struct overlays kept shipping — and the discipline they encode is visible in any well-documented format: SQLite's file header, for instance, is specified byte by byte, big-endian, as offsets in a document rather than as anyone's struct. When you author the boundary yourself (Chapter 30), extend that chapter's rule the last step: nothing whose layout your compiler chose crosses a binary boundary — *including the wire*. Define your format as bytes and offsets in a document you own, and both sides get to be right on every host. (C++20 adds `std::endian` to name the host's order and `std::bit_cast` to legalize the memcpy spelling; neither changes the advice, because the readers above never needed to know the host's order in the first place.)

<!-- nav:begin -->
[← Chapter 33 — Here Is the Report](33-here-is-the-report.md) · [Contents](README.md) · [Chapter 35 — Still Live at Unload →](35-still-live-at-unload.md)
<!-- nav:end -->
