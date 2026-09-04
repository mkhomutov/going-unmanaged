// Appendix F, Recipes 31 and 32 - a feature flag read once at startup and
// kept as a member, and a [Flags] enum spelled as an enum class with its
// operators.
//
// Features and Processor (Recipe 31), and Channel with its three operators
// (Recipe 32), are quoted VERBATIM in book/F-rosetta-cookbook.md: editing
// one means editing the appendix in the same commit (the testlab
// discipline). main() is scaffolding - it sets the environment, reads the
// flags once, asserts the branch, the read-once, the junk-keeps-the-default
// parse and the bit arithmetic; the refusals the recipes name stay comments.
#include <cassert>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <string_view>

// Recipe 31 - IConfiguration read at startup, once
struct Features {
    bool audit = false;                  // the defaults ARE the off state
    bool fast_path = false;
    int  batch_size = 64;

    // One source among several - Recipe 26's JSON file, the host's
    // preferences API, a command line. Whatever the source, it is read HERE,
    // once, and never again.
    static Features from_environment() {
        Features f;
        if (const char* v = std::getenv("MYPLUGIN_AUDIT"))      f.audit = std::string_view(v) == "1";
        if (const char* v = std::getenv("MYPLUGIN_FAST_PATH"))  f.fast_path = std::string_view(v) == "1";
        if (const char* v = std::getenv("MYPLUGIN_BATCH_SIZE")) {
            const std::string_view s(v);
            std::from_chars(s.data(), s.data() + s.size(), f.batch_size);   // junk: batch_size stays 64 (Recipe 19)
        }
        return f;
    }
};

class Processor {
public:
    explicit Processor(Features features) : features_(features) {}   // read once, kept as a member

    int process(int sample) const {
        if (features_.fast_path) {       // a branch: free, even on the deadline path
            return sample;
        }
        return sample * 2;
    }

private:
    Features features_;
};

// Recipe 32 - [Flags] enum Channel, and HasFlag
enum class Channel : std::uint8_t { None = 0, Left = 1, Right = 2, Sub = 4 };   // [Flags] enum Channel

constexpr Channel operator|(Channel a, Channel b) {
    return static_cast<Channel>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}
constexpr Channel operator&(Channel a, Channel b) {
    return static_cast<Channel>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}
constexpr bool has(Channel set, Channel flag) { return (set & flag) == flag; }   // set.HasFlag(flag)

namespace {
    void set_env(const char* name, const char* value) {
#if defined(_WIN32)
        _putenv_s(name, value);
#else
        setenv(name, value, 1);
#endif
    }
}

int main() {
    // Nothing set: the defaults are the off state.
    const Features off = Features::from_environment();
    assert(!off.audit && !off.fast_path && off.batch_size == 64);
    assert(Processor(off).process(21) == 42);

    // Set, read once, kept: the environment can change afterwards and the
    // Processor does not care - that is the whole point of reading once. (A
    // Processor that re-read the environment in process() fails the last
    // assert here; it was run.) The broken shape the recipe's trap names -
    // the read at the point of use on the deadline path - would pass every
    // assertion in this file and stays book-only.
    set_env("MYPLUGIN_FAST_PATH", "1");
    set_env("MYPLUGIN_BATCH_SIZE", "128");
    const Features on = Features::from_environment();
    assert(on.fast_path && on.batch_size == 128 && !on.audit);
    const Processor fast(on);
    set_env("MYPLUGIN_FAST_PATH", "0");
    assert(fast.process(21) == 21);

    // Junk in the environment keeps the default - from_chars leaves the
    // target untouched on failure, where atoi would have written 0 over 64.
    set_env("MYPLUGIN_BATCH_SIZE", "abc");
    assert(Features::from_environment().batch_size == 64);

    // The [Flags] enum: combine, test, and the type survives the arithmetic.
    constexpr Channel stereo = Channel::Left | Channel::Right;
    static_assert(has(stereo, Channel::Left));
    static_assert(has(stereo, Channel::Right));
    static_assert(!has(stereo, Channel::Sub));
    static_assert(has(stereo | Channel::Sub, Channel::Sub));
    static_assert(!has(Channel::None, Channel::Left));
    static_assert((stereo & Channel::Sub) == Channel::None);
    static_assert(has(Channel::Left, Channel::None));   // like HasFlag(0): always true - test bits, not None
    // The trap the recipe names: an overlap test reads as HasFlag and is
    // wrong for a combined flag - a set holding only Left "has" stereo.
    static_assert((Channel::Left & stereo) != Channel::None);   // the wrong test says yes
    static_assert(!has(Channel::Left, stereo));                 // has() says no
    // The refusal, as a comment because a refusal cannot be compiled:
    // without the operators above, `Channel::Left | Channel::Right` is
    // "invalid operands to binary expression" (clang; GCC says "no match
    // for 'operator|'") - an enum class does not decay to int, which is the
    // feature. A plain `enum` would compile it and hand back an int, and
    // the type would be gone.
    return 0;
}
