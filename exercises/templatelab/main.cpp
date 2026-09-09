// The judge for Chapter 41's lab. One Session, two policies: the vendor's
// device behind FakeDevice and a recording double, both under the canonical
// flags. Plus the one build that must FAIL: -DTEMPLATELAB_BROKEN_POLICY
// instantiates Session with a policy missing Poll AND calls Pump, and the
// static_assert in session.h must be what refuses it, by name (build_all.sh
// checks the diagnostic's text, the constlab discipline). The Pump call is
// load-bearing: without it the broken policy would compile clean even with
// the static_assert deleted, because Pump is only compiled when used.
#include "policies.h"
#include "session.h"
#include "util.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <type_traits>
#include <vector>

#ifdef TEMPLATELAB_BROKEN_POLICY
// A policy that forgot Poll: the build must be refused with a sentence
// naming the shape, not with the instantiation chain.
struct HalfSdk {
    using Handle = int;
    static Handle Open(const char*) { return 1; }
    static void Close(Handle) {}
};
std::size_t Probe() {
    Session<HalfSdk> broken("x");
    return broken.Pump();                       // the use that would otherwise be the first error
}
#endif

// Compile-time claims about the type, per instantiation: the seam costs no
// virtual call (no vtable pointer, so the object is exactly its members)
// and moves without throwing (Chapter 6's noexcept, checked rather than hoped).
// --8<-- [start:nothrow-move-assert]
static_assert(std::is_nothrow_move_constructible_v<Session<RecordingSdk>>);
static_assert(std::is_nothrow_move_constructible_v<Session<FakeDeviceSdk>>);
static_assert(!std::is_polymorphic_v<Session<RecordingSdk>>);
static_assert(HasSdkShape<RecordingSdk>::value && HasSdkShape<FakeDeviceSdk>::value);
static_assert(!HasSdkShape<int>::value);
// --8<-- [end:nothrow-move-assert]

// The three type computations of Chapter 41's "asking the compiler" section,
// asserted rather than described. Nothing here runs: every one of these is
// answered before the program exists, which is the whole point of the
// section and the reason the judge for it is a static_assert.
// --8<-- [start:type-computations]
// A static member's type, named without retyping it - and renamed with it if
// the policy ever changes. Chapter 10's `decltype(&Device_SetCallback)` is
// this, applied to a vendor's function pointer.
using ScriptTable = decltype(RecordingSdk::scripts);
static_assert(std::is_same_v<ScriptTable, std::vector<std::vector<int>>>);

// decltype answers with the expression's own type, reference and all;
// `auto` answers with the type you would get by copying it. On a RETURN that
// difference is a whole class of accidental-copy bug, and this is its shape:
int shared_counter = 0;
int& counter_by_ref() { return shared_counter; }

auto by_auto() { return counter_by_ref(); }                     // int  - the & is gone
decltype(auto) by_decltype_auto() { return counter_by_ref(); }  // int& - it is not

static_assert(std::is_same_v<decltype(by_auto()), int>);
static_assert(std::is_same_v<decltype(by_decltype_auto()), int&>);

// And the parenthesis rule, which surprises everyone once: decltype of a
// NAME is that name's declared type; decltype of an expression that happens
// to be one name in parentheses is a reference, because that is what the
// expression is.
static_assert(std::is_same_v<decltype(shared_counter), int>);
static_assert(std::is_same_v<decltype((shared_counter)), int&>);

// std::decay_t IS what `auto` does, spelled out: strip the reference, strip
// const and volatile, turn an array into a pointer and a function into a
// pointer to it. Reach for it when a deduced T must be STORED - in a member,
// in a container, in a std::function - because none of those can hold a
// reference or an array the way a parameter can.
static_assert(std::is_same_v<std::decay_t<const int&>, int>);
static_assert(std::is_same_v<std::decay_t<int(&)[8]>, int*>);
static_assert(std::is_same_v<std::decay_t<int(int)>, int (*)(int)>);
// --8<-- [end:type-computations]

int main() {
    // The double: scripted samples, no device, and the counts prove RAII.
    // Side effects stay outside the asserts (Recipe 24's trap).
    RecordingSdk::scripts = {{1, 2, 3}, {40}};
    {
        Session<RecordingSdk> a("scripted-a");
        Session<RecordingSdk> b("scripted-b");
        const std::size_t na = a.Pump();
        const std::size_t nb = b.Pump();
        assert(na == 3 && nb == 1);
        assert((a.Samples() == std::vector<int>{1, 2, 3}));
        Session<RecordingSdk> moved = std::move(a);      // the husk closes nothing...
        assert(!a.IsOpen() && moved.IsOpen() && moved.Samples().size() == 3);
        const std::size_t husk = a.Pump();               // ...and pumps nothing, quietly
        assert(husk == 0);
    }
    assert(RecordingSdk::open_count == 2 && RecordingSdk::close_count == 2);

    // The vendor's device through the same Session, unchanged.
    {
        Session<FakeDeviceSdk> s("sensor0");
        assert(s.IsOpen());
        FakeDevice_InjectSamples(s.Raw(), 2);
        const std::size_t n = s.Pump();
        assert(n == 2);
        assert((s.Samples() == std::vector<int>{100, 101}));
    }
    assert(FakeDevice_OpenHandles() == 0);

    // util.h: the three templates, asserted.
    assert(Describe(42) == "42");
    assert(Describe('A') == "A");                      // not "65": the char branch
    assert(Describe("text") == "text");
    assert(Describe(std::string("s")) == "s");
    assert(Join(", ", 1, "two", 3.5) == "1, two, 3.500000");
    assert(Join("-") == "");
    Ring<int, 3> ring;
    for (int i = 1; i <= 5; ++i) ring.Push(i);
    assert(ring.Size() == 3 && ring.Oldest() == 3);
    static_assert(sizeof(Ring<float, 8>) >= 8 * sizeof(float));   // inline storage, no heap block

    std::printf("templatelab ok: one Session, two policies, three utilities\n");
    return 0;
}
