// Appendix F, Recipes 36 and 37 - hash bytes, and seal bytes for a reader
// in C#: SHA-256 and AES-256-GCM through OpenSSL's EVP interface.
//
// The cookbook's second TU built behind a probe: it needs libcrypto, which
// build_all.sh locates through pkg-config and prints SKIPPED without
// (--require-openssl refuses to skip; CI passes it). Bytes, hex(), sha256(),
// CipherCtx, seal() and open_sealed() are quoted VERBATIM in
// book/F-rosetta-cookbook.md: editing one means editing the appendix in the
// same commit (the testlab discipline). main() is scaffolding - it holds the
// listings to PUBLISHED test vectors (NIST's SHA-256 of "abc" and of
// nothing; the GCM specification's test cases 13 and 14), because a round
// trip proves only that the two halves agree with each other, and the
// question is whether they agree with .NET's AesGcm. Then a round trip with
// a random nonce, and the tamper that must fail.
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using Bytes = std::vector<std::uint8_t>;

std::string hex(const Bytes& bytes) {                        // Convert.ToHexStringLower (.NET 9); ToHexString is UPPER-case
    static constexpr char digits[] = "0123456789abcdef";
    std::string out;
    for (const std::uint8_t b : bytes) {
        out += digits[b >> 4];
        out += digits[b & 0x0F];
    }
    return out;
}

// Recipe 36 - SHA256.HashData(bytes)
Bytes sha256(std::string_view data) {
    Bytes digest(EVP_MAX_MD_SIZE);
    unsigned int written = 0;
    if (EVP_Digest(data.data(), data.size(), digest.data(), &written, EVP_sha256(), nullptr) != 1) {
        throw std::runtime_error("EVP_Digest failed");        // the event pole: the library itself broke
    }
    digest.resize(written);                                    // 32 for SHA-256
    return digest;
}

// Recipe 37 - new AesGcm(key, tagSizeInBytes: 16).Encrypt(nonce, plain, ciphertext, tag)
using Key   = std::array<std::uint8_t, 32>;                    // AES-256: the key size is the algorithm's name
using Nonce = std::array<std::uint8_t, 12>;                    // 96 bits: what GCM and AesGcm both expect
constexpr std::size_t kTagSize = 16;                           // the authentication tag: full length, always

using CipherCtx = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;   // Recipe 7's shape

// The envelope, and the whole of the cross-language contract:
//   nonce (12 bytes) || ciphertext (plain.size() bytes) || tag (16 bytes)
// Every reader - C#, Python, the next version of this plug-in - opens it
// by reading those three lengths back, so the layout is an ICD (Chapter 34).
Bytes seal(const Key& key, const Nonce& nonce, const Bytes& plain) {
    CipherCtx ctx(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    if (!ctx || EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, key.data(), nonce.data()) != 1) {
        throw std::runtime_error("AES-256-GCM init failed");
    }
    Bytes out(nonce.begin(), nonce.end());
    out.resize(nonce.size() + plain.size() + kTagSize);
    std::uint8_t* const ciphertext = out.data() + nonce.size();
    int n = 0;
    if (EVP_EncryptUpdate(ctx.get(), ciphertext, &n, plain.data(), static_cast<int>(plain.size())) != 1 ||
        EVP_EncryptFinal_ex(ctx.get(), ciphertext + n, &n) != 1 ||                 // GCM: no padding, n is 0 here
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, kTagSize, ciphertext + plain.size()) != 1) {
        throw std::runtime_error("AES-256-GCM seal failed");
    }
    return out;
}

// Absence is the verdict: a wrong key, a flipped byte, a truncated envelope
// all come back as nullopt (Recipe 19), and no unauthenticated byte leaves this
// function - DecryptUpdate fills the buffer, DecryptFinal_ex checks the tag, and
// the buffer is returned only past that check, and wiped when it fails.
std::optional<Bytes> open_sealed(const Key& key, const Bytes& sealed) {
    if (sealed.size() < std::tuple_size<Nonce>::value + kTagSize) {
        return std::nullopt;
    }
    const std::uint8_t* const nonce      = sealed.data();
    const std::uint8_t* const ciphertext = nonce + std::tuple_size<Nonce>::value;
    const std::size_t length = sealed.size() - std::tuple_size<Nonce>::value - kTagSize;
    std::array<std::uint8_t, kTagSize> tag{};
    std::copy(sealed.end() - kTagSize, sealed.end(), tag.begin());

    CipherCtx ctx(EVP_CIPHER_CTX_new(), &EVP_CIPHER_CTX_free);
    Bytes plain(length);
    int n = 0;
    if (!ctx || EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, key.data(), nonce) != 1 ||
        EVP_DecryptUpdate(ctx.get(), plain.data(), &n, ciphertext, static_cast<int>(length)) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, kTagSize, tag.data()) != 1 ||
        EVP_DecryptFinal_ex(ctx.get(), plain.data() + n, &n) != 1) {       // the tag check lives HERE
        OPENSSL_cleanse(plain.data(), plain.size());                       // what AesGcm.Decrypt does before it throws
        return std::nullopt;
    }
    return plain;
}

int main() {
    // Recipe 36 against NIST's published vectors: the digest of "abc", and
    // of nothing at all - the second is the one every codebase meets by
    // hashing an empty buffer.
    assert(hex(sha256("abc")) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    assert(hex(sha256("")) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    assert(sha256("abc").size() == 32);

    // Recipe 37 against the GCM specification's test cases 13 and 14: an
    // all-zero key and nonce, and first no plaintext, then sixteen zero
    // bytes. If these two lines hold, the envelope's ciphertext and tag are
    // what AesGcm in .NET produces for the same inputs - the property no
    // round trip can prove.
    const Key zero_key{};
    const Nonce zero_nonce{};
    const Bytes tc13 = seal(zero_key, zero_nonce, {});
    assert(hex(Bytes(tc13.begin() + 12, tc13.end())) == "530f8afbc74536b9a963b4f1c4cb738b");
    const Bytes tc14 = seal(zero_key, zero_nonce, Bytes(16, 0));
    assert(hex(Bytes(tc14.begin() + 12, tc14.end())) ==
           "cea7403d4d606b6e074ec5d3baf39d18" "d0d1c8a799996bf0265b98b5d48ab919");

    // The round trip, with the nonce drawn fresh - never reused under one
    // key, which is GCM's one unforgiving rule.
    Key key{};
    Nonce nonce{};
    assert(RAND_bytes(key.data(), static_cast<int>(key.size())) == 1);
    assert(RAND_bytes(nonce.data(), static_cast<int>(nonce.size())) == 1);
    const std::string_view text = "licence: 2026-12-31, seats: 5";
    const Bytes plain(text.begin(), text.end());
    Bytes sealed = seal(key, nonce, plain);
    assert(sealed.size() == 12 + plain.size() + 16);
    assert(std::equal(nonce.begin(), nonce.end(), sealed.begin()));          // the nonce travels in the clear
    const auto opened = open_sealed(key, sealed);
    assert(opened && *opened == plain);

    // The tamper: one bit anywhere in the envelope, and the verdict is absence.
    sealed[12] ^= 0x01;                                                      // a ciphertext byte
    assert(!open_sealed(key, sealed));
    sealed[12] ^= 0x01;
    sealed.back() ^= 0x80;                                                   // a tag byte
    assert(!open_sealed(key, sealed));
    sealed.back() ^= 0x80;
    Key wrong_key = key;
    wrong_key[0] ^= 0xFF;
    assert(!open_sealed(wrong_key, sealed));                                 // the wrong key
    assert(!open_sealed(key, Bytes(sealed.begin(), sealed.begin() + 20)));   // truncated below nonce + tag
    assert(open_sealed(key, sealed) && *open_sealed(key, sealed) == plain);  // and intact, it still opens
    return 0;
}
