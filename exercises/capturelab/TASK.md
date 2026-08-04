# Parse This Capture — ticket card (Chapter 34)

This lab is a **ticket, not a task** — and its evidence is a bus capture,
attached below with the vendor's own header table. Chapter 34 states
everything in full and walks the diagnosis behind a spoiler fold — work the
ticket cold first, and start **on paper**: decode the capture by hand from
the table before you run anything. Fair warning from the chapter: the
handbook's sanitizer flags stay green on this bug. Your hand decode is the
only oracle this ticket has.

> **#5347 — Ingest rejects every capture from the new analyzer
> (bring-up).** The new bus analyzer's telemetry feed is being brought up.
> The vendor's own viewer opens every capture file fine; our ingest calls
> every one of them malformed. The developer who started the bring-up notes
> that the sync byte parses fine — and that the header's kind field comes
> back 165 on a frame the vendor's viewer calls a temperature reading
> (kind 1). A minimal capture is attached, two frames straight off the bus,
> along with the ICD's header table.

**The files beside this card are the FIXED reference** — the lab's green
state, asserted against the chapter's hand decode by `build_all.sh` on
every push. Do not start from them. Recreate the broken parser below in a
scratch directory of your own, and work from there.

## The wire, as the ICD documents it

| Offset | Size | Field | Notes |
|---|---|---|---|
| 0 | 1 | sync | always `0xA5` |
| 1 | 4 | sequence | big-endian |
| 5 | 2 | payload length | big-endian; payload bytes after the header |
| 7 | 1 | kind | `0x01` = temperature; payload is a u16, centi-°C |

> Multi-byte fields are network byte order (big-endian). The header is
> packed: 8 bytes, no padding. — *ICD, section 4.2*

## The attached capture

Twenty bytes, two frames:

```text
a5 00 00 00 01 00 02 01 09 29 a5 00 00 00 02 00
02 01 09 2b
```

## The code as it shipped

`ingest.cpp`, one file:

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

## Work the ticket

1. **Decode the capture by hand — no compiler.** From the ICD table alone,
   write down every field of both frames: sequence, payload length, kind,
   and the temperature. This paper is the ticket's oracle; nothing in the
   toolchain supplies it.
2. **Find the 165.** `165` is `0xA5`. Locate it in the dump. Which byte is
   the struct actually reading for `kind` — and what does that say about
   where the struct thinks the fields live?
3. **Predict `sizeof(Header)`, then print it.** The ICD says 8. Print the
   member offsets too (`offsetof`) and compare them with the table's.
4. **Apply the tempting fix.** Wrap the struct in `#pragma pack(push, 1)` /
   `#pragma pack(pop)` and rerun. The new numbers are still wrong — write
   16777216 out in hex before reading any further.
5. **Write the honest fix.** Named offsets from the ICD and readers that
   spell the wire's byte order (`read_u16_be`, `read_u32_be` — shifts over
   `unsigned char`). `scripts/check.sh` must run green, but green is not
   the bar here: the decode must match your step-1 paper, field for field,
   for both frames.
6. **Stretch: give the sanitizer its one line.** Overlay the *second*
   frame (offset 10) with the unpacked struct under the canonical flags —
   UBSan speaks its only sentence in this whole ticket. Then pack the
   struct and watch the same run go quiet while the value stays mirrored.
