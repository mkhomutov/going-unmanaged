// Appendix F, Recipe 31 - a feature flag read once at startup and kept as a
// member, and a [Flags] enum spelled as an enum class with its operators.
//
// Features, Processor, Channel and its three operators are quoted VERBATIM
// in book/F-rosetta-cookbook.md: editing one means editing the appendix in
// the same commit (the testlab discipline). main() is scaffolding - it sets
// the environment, reads the flags once, and asserts the branch and the
// bit arithmetic; the refusal the recipe names stays a comment.
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <string_view>

// Recipe 31 - IConfiguration read at startup; [Flags] enum
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
        if (const char* v = std::getenv("MYPLUGIN_BATCH_SIZE")) f.batch_size = std::atoi(v);
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
    // Processor does not care - that is the whole point of reading once.
    set_env("MYPLUGIN_FAST_PATH", "1");
    set_env("MYPLUGIN_BATCH_SIZE", "128");
    const Features on = Features::from_environment();
    assert(on.fast_path && on.batch_size == 128 && !on.audit);
    const Processor fast(on);
    set_env("MYPLUGIN_FAST_PATH", "0");
    assert(fast.process(21) == 21);

    // The [Flags] enum: combine, test, and the type survives the arithmetic.
    constexpr Channel stereo = Channel::Left | Channel::Right;
    static_assert(has(stereo, Channel::Left));
    static_assert(has(stereo, Channel::Right));
    static_assert(!has(stereo, Channel::Sub));
    static_assert(has(stereo | Channel::Sub, Channel::Sub));
    static_assert(!has(Channel::None, Channel::Left));
    static_assert((stereo & Channel::Sub) == Channel::None);
    // The refusal the recipe names, as a comment because a refusal cannot
    // be compiled: without the operators above, `Channel::Left | Channel::Right`
    // is "invalid operands to binary expression" - an enum class does not
    // decay to int, which is the feature. A plain `enum` would compile it
    // and hand back an int, and the type would be gone.
    return 0;
}
