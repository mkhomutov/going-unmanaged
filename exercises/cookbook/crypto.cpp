// Appendix F, Recipes 36, 37, 47 and 48 - hash bytes; seal bytes for a
// reader in C#; derive a key from a password or a secret; sign and verify
// bytes: SHA-256, AES-256-GCM, PBKDF2 and HKDF, and HMAC-SHA-256 through
// OpenSSL's EVP interfaces.
//
// The cookbook's second TU built behind a probe: it needs libcrypto, which
// build_all.sh locates through pkg-config and prints SKIPPED without
// (--require-openssl refuses to skip; CI passes it). Bytes, hex(), sha256(),
// CipherCtx, seal(), open_sealed(), key_from_password(), DeriveCtx,
// key_from_secret(), Mac, MacCtx, hmac_sha256() and verify_hmac_sha256() are
// included by book/F-rosetta-cookbook.md, between their recipe-N section
// markers: edit here and the page follows, and a marker moved is what the page
// shows. main() is scaffolding - it holds the listings to PUBLISHED test
// vectors (NIST's SHA-256 of "abc" and of nothing; the GCM specification's
// test cases 13 and 14; RFC 7914's PBKDF2-HMAC-SHA-256 cases; RFC 5869's first
// HKDF case; RFC 4231's first two HMAC cases), because a round trip proves
// only that the two halves agree with each other, and the question is whether
// they agree with .NET's. Then a round trip with a random nonce, and the
// tampers that must fail. Recipe 48 needs OpenSSL 3's EVP_MAC; the rest builds
// on 1.1.1 as well.
#include <openssl/core_names.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
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

// --8<-- [start:recipe-36]
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
// --8<-- [end:recipe-36]

// Recipe 37 - new AesGcm(key, tagSizeInBytes: 16).Encrypt(nonce, plain, ciphertext, tag)
// --8<-- [start:recipe-37]
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
    if (!ctx || EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, key.data(), nonce) != 1) {
        throw std::runtime_error("AES-256-GCM init failed");           // the library, not the envelope: the event pole
    }
    Bytes plain(length);
    int n = 0;
    if (EVP_DecryptUpdate(ctx.get(), plain.data(), &n, ciphertext, static_cast<int>(length)) != 1 ||
        EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, kTagSize, tag.data()) != 1 ||
        EVP_DecryptFinal_ex(ctx.get(), plain.data() + n, &n) != 1) {       // the tag check lives HERE
        OPENSSL_cleanse(plain.data(), plain.size());                       // what AesGcm.Decrypt does before it throws
        return std::nullopt;
    }
    return plain;
}
// --8<-- [end:recipe-37]

// Recipe 47 - Rfc2898DeriveBytes.Pbkdf2(password, salt, iterations, HashAlgorithmName.SHA256, 32)
//             HKDF.DeriveKey(HashAlgorithmName.SHA256, secret, 32, salt, info)
// --8<-- [start:recipe-47]
Key key_from_password(std::string_view password, const Bytes& salt, int iterations) {
    Key key{};
    if (PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()),
                          salt.data(), static_cast<int>(salt.size()),
                          iterations, EVP_sha256(),
                          static_cast<int>(key.size()), key.data()) != 1) {
        throw std::runtime_error("PBKDF2-HMAC-SHA256 failed");   // the library, not the input: the event pole
    }
    return key;
}

using DeriveCtx = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;   // Recipe 7's shape, again

Key key_from_secret(const Bytes& secret, const Bytes& salt, const Bytes& info) {
    DeriveCtx ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr), &EVP_PKEY_CTX_free);
    Key key{};
    std::size_t length = key.size();
    if (!ctx || EVP_PKEY_derive_init(ctx.get()) != 1 ||
        EVP_PKEY_CTX_set_hkdf_md(ctx.get(), EVP_sha256()) != 1 ||
        EVP_PKEY_CTX_set1_hkdf_salt(ctx.get(), salt.data(), static_cast<int>(salt.size())) != 1 ||
        EVP_PKEY_CTX_set1_hkdf_key(ctx.get(), secret.data(), static_cast<int>(secret.size())) != 1 ||
        EVP_PKEY_CTX_add1_hkdf_info(ctx.get(), info.data(), static_cast<int>(info.size())) != 1 ||
        EVP_PKEY_derive(ctx.get(), key.data(), &length) != 1 || length != key.size()) {
        throw std::runtime_error("HKDF-SHA256 failed");
    }
    return key;
}
// --8<-- [end:recipe-47]

// Recipe 48 - new HMACSHA256(key).ComputeHash(data) / CryptographicOperations.FixedTimeEquals
// --8<-- [start:recipe-48]
using Mac    = std::unique_ptr<EVP_MAC, decltype(&EVP_MAC_free)>;
using MacCtx = std::unique_ptr<EVP_MAC_CTX, decltype(&EVP_MAC_CTX_free)>;

Bytes hmac_sha256(const Bytes& key, const Bytes& data) {
    Mac mac(EVP_MAC_fetch(nullptr, "HMAC", nullptr), &EVP_MAC_free);
    MacCtx ctx(mac ? EVP_MAC_CTX_new(mac.get()) : nullptr, &EVP_MAC_CTX_free);
    char digest[] = "SHA256";                              // a writable char* by signature; a name goes here, the key through init
    const OSSL_PARAM params[] = {OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST, digest, 0),
                                 OSSL_PARAM_construct_end()};
    Bytes tag(EVP_MAX_MD_SIZE);
    std::size_t written = 0;
    if (!ctx || EVP_MAC_init(ctx.get(), key.data(), key.size(), params) != 1 ||
        EVP_MAC_update(ctx.get(), data.data(), data.size()) != 1 ||
        EVP_MAC_final(ctx.get(), tag.data(), &written, tag.size()) != 1) {
        throw std::runtime_error("HMAC-SHA256 failed");
    }
    tag.resize(written);                                   // 32 for SHA-256
    return tag;
}

bool verify_hmac_sha256(const Bytes& key, const Bytes& data, const Bytes& tag) {
    const Bytes expected = hmac_sha256(key, data);
    // CRYPTO_memcmp, never ==: a comparison that stops at the first wrong
    // byte tells an attacker how many bytes were right (FixedTimeEquals).
    // No harness can see this line change - constant time is not a value.
    return expected.size() == tag.size() && CRYPTO_memcmp(expected.data(), tag.data(), tag.size()) == 0;
}
// --8<-- [end:recipe-48]

// Recipe 54 - a signature, where the verifier must NOT be able to sign
// --8<-- [start:recipe-54]
using PrivateKey = std::array<std::uint8_t, 32>;   // Ed25519's seed: kept, never shipped
using PublicKey  = std::array<std::uint8_t, 32>;   // ships with the plug-in, in the clear
using Signature  = std::array<std::uint8_t, 64>;

using PKey  = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;       // Recipe 7's shape
using MdCtx = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

// The public half, derived from the private one - so the two cannot drift.
PublicKey public_key_of(const PrivateKey& secret) {
    PKey key(EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, secret.data(), secret.size()),
             &EVP_PKEY_free);
    PublicKey pub{};
    std::size_t size = pub.size();
    if (!key || EVP_PKEY_get_raw_public_key(key.get(), pub.data(), &size) != 1 || size != pub.size()) {
        throw std::runtime_error("Ed25519 public key derivation failed");
    }
    return pub;
}

// Signing needs the private key. Nothing on the customer's machine has it.
Signature sign_ed25519(const PrivateKey& secret, const Bytes& message) {
    PKey key(EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, secret.data(), secret.size()),
             &EVP_PKEY_free);
    MdCtx ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    Signature signature{};
    std::size_t size = signature.size();
    // No digest argument: Ed25519 is a one-shot over the whole message, which
    // is why this is EVP_DigestSign and not the init/update/final of Recipe 48.
    if (!key || !ctx
        || EVP_DigestSignInit(ctx.get(), nullptr, nullptr, nullptr, key.get()) != 1
        || EVP_DigestSign(ctx.get(), signature.data(), &size,
                          message.data(), message.size()) != 1
        || size != signature.size()) {
        throw std::runtime_error("Ed25519 signing failed");
    }
    return signature;
}

// Verifying needs only the public key - and a wrong answer is a return value,
// not an exception, because a bad signature is a value (Chapter 8).
bool verify_ed25519(const PublicKey& pub, const Bytes& message, const Signature& signature) {
    PKey key(EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr, pub.data(), pub.size()),
             &EVP_PKEY_free);
    MdCtx ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (!key || !ctx
        || EVP_DigestVerifyInit(ctx.get(), nullptr, nullptr, nullptr, key.get()) != 1) {
        throw std::runtime_error("Ed25519 verification could not start");
    }
    return EVP_DigestVerify(ctx.get(), signature.data(), signature.size(),
                            message.data(), message.size()) == 1;
}
// --8<-- [end:recipe-54]

static Bytes bytes_of(std::string_view s) { return Bytes(s.begin(), s.end()); }

// The published vectors below are written as hex, the way the RFC prints them.
template <std::size_t N>
static std::array<std::uint8_t, N> unhex(std::string_view text) {
    std::array<std::uint8_t, N> out{};
    for (std::size_t i = 0; i < N; ++i) {
        out[i] = static_cast<std::uint8_t>(
            std::stoul(std::string(text.substr(i * 2, 2)), nullptr, 16));
    }
    return out;
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

    // Recipe 47 against RFC 7914 section 11's PBKDF2-HMAC-SHA-256 vectors -
    // the first 32 of their 64 bytes, since PBKDF2's first block does not
    // depend on the length asked for - and RFC 5869's first HKDF-SHA-256
    // case, the first 32 of its 42 bytes for the same reason. If these hold,
    // Rfc2898DeriveBytes.Pbkdf2 and HKDF.DeriveKey on the C# side derive the
    // same key from the same inputs.
    {
        const Key k = key_from_password("passwd", bytes_of("salt"), 1);
        assert(hex(Bytes(k.begin(), k.end())) == "55ac046e56e3089fec1691c22544b605f94185216dde0465e68b9d57c20dacbc");
    }
    {
        const Key k = key_from_password("Password", bytes_of("NaCl"), 80000);
        assert(hex(Bytes(k.begin(), k.end())) == "4ddcd8f60b98be21830cee5ef22701f9641a4418d04c0414aeff08876b34ab56");
        // The trap, as a value: the same password with a different count is a
        // different key, and nothing anywhere says so except open_sealed.
        const Key other = key_from_password("Password", bytes_of("NaCl"), 80001);
        assert(other != k);
        assert(!open_sealed(other, seal(k, nonce, plain)));
    }
    {
        const Bytes ikm(22, 0x0b);
        const Bytes hkdf_salt = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c};
        const Bytes info = {0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9};
        const Key k = key_from_secret(ikm, hkdf_salt, info);
        assert(hex(Bytes(k.begin(), k.end())) == "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf");
        // info separates purposes: one secret, two keys.
        assert(key_from_secret(ikm, hkdf_salt, bytes_of("other purpose")) != k);
    }

    // Recipe 48 against RFC 4231's test cases 1 and 2, then its own verifier:
    // a flipped byte, a wrong key, a changed message and a short tag all refuse.
    assert(hex(hmac_sha256(Bytes(20, 0x0b), bytes_of("Hi There"))) ==
           "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
    const Bytes jefe = bytes_of("Jefe");
    const Bytes message = bytes_of("what do ya want for nothing?");
    const Bytes tag = hmac_sha256(jefe, message);
    assert(hex(tag) == "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
    assert(verify_hmac_sha256(jefe, message, tag));
    Bytes flipped = tag;
    flipped[5] ^= 0x01;
    assert(!verify_hmac_sha256(jefe, message, flipped));
    assert(!verify_hmac_sha256(bytes_of("Jeff"), message, tag));
    assert(!verify_hmac_sha256(jefe, bytes_of("what do ya want for nothing"), tag));
    assert(!verify_hmac_sha256(jefe, message, Bytes(tag.begin(), tag.end() - 1)));
    // Recipe 54 against RFC 8032's own Ed25519 vectors (section 7.1, TEST 1
    // and TEST 2). The public keys are DERIVED from the secrets here and
    // must equal the published ones, and the signatures must equal the
    // published ones byte for byte - so this is the RFC checking us, not us
    // checking ourselves. A round trip would pass with a broken
    // implementation that was broken consistently.
    {
        const auto secret1 = unhex<32>("9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60");
        const auto public1 = unhex<32>("d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a");
        const auto sig1 = unhex<64>(
            "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e0652249015"
            "55fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b");
        assert(public_key_of(secret1) == public1);
        assert(sign_ed25519(secret1, Bytes{}) == sig1);
        assert(verify_ed25519(public1, Bytes{}, sig1));

        const auto secret2 = unhex<32>("4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb");
        const auto public2 = unhex<32>("3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c");
        const auto sig2 = unhex<64>(
            "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69d"
            "a085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302aeeb00d291612bb0c00");
        const Bytes message2{0x72};
        assert(public_key_of(secret2) == public2);
        assert(sign_ed25519(secret2, message2) == sig2);
        assert(verify_ed25519(public2, message2, sig2));

        // And the four ways a verifier must say no. The last is the one the
        // recipe exists for: the OTHER key holder's signature is refused,
        // which is what a shared HMAC key cannot promise.
        Bytes tampered = message2;
        tampered[0] = 0x73;
        assert(!verify_ed25519(public2, tampered, sig2));

        Signature bent = sig2;
        bent[0] ^= 0x01;
        assert(!verify_ed25519(public2, message2, bent));

        assert(!verify_ed25519(public1, message2, sig2));
        assert(!verify_ed25519(public2, Bytes{}, sig1));
    }

    return 0;
}
