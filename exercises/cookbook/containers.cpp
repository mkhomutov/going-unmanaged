// Appendix F, Recipe 27 - pre-size a collection: List<T>(capacity) is
// reserve, new T[n] is vector(n), and the two are not the same thing.
//
// read_samples() and zeroed() are quoted VERBATIM in
// book/F-rosetta-cookbook.md: editing one means editing the appendix in the
// same commit (the testlab discipline). next_sample() stands for whatever
// produces the elements; main() is scaffolding - it asserts the two numbers
// the recipe is about (size and capacity) on every path, and demonstrates
// the resize-then-push_back trap and the reserve-in-the-loop one. The other
// trap, reserve-then-index, is undefined behavior and stays a comment.
#include <cassert>
#include <cstddef>
#include <vector>

namespace {
    int next_sample() {
        static int value = 100;
        return value++;
    }
}

// Recipe 27 - new List<int>(capacity), and new double[n]
std::vector<int> read_samples(std::size_t expected) {
    std::vector<int> samples;
    samples.reserve(expected);            // List<T>(capacity): room for expected, size still 0
    for (std::size_t i = 0; i < expected; ++i) {
        samples.push_back(next_sample());  // size grows; no reallocation until the room runs out
    }
    return samples;
}

std::vector<double> zeroed(std::size_t n) {
    return std::vector<double>(n);        // new double[n]: n elements, every one 0.0
}

int main() {
    // reserve: the room is there, the elements are not, and the loop never
    // moves the block. The pointer is taken after the first push_back, not
    // before: data() of an EMPTY vector is unspecified, while data() of a
    // non-empty one is &front(), and the standard promises no reallocation
    // on insertions until size would exceed the reserved capacity.
    std::vector<int> probe;
    probe.reserve(8);
    assert(probe.empty());
    assert(probe.capacity() >= 8);
    probe.push_back(0);
    const int* block = probe.data();
    for (int i = 1; i < 8; ++i) {
        probe.push_back(i);
    }
    assert(probe.size() == 8);
    assert(probe.data() == block);        // no reallocation: reserve paid it up front

    const std::vector<int> samples = read_samples(5);
    assert(samples.size() == 5);
    assert(samples[0] == 100 && samples[4] == 104);

    // vector(n): both numbers set, every element value-initialized.
    const std::vector<double> z = zeroed(4);
    assert(z.size() == 4);
    for (double d : z) {
        assert(d == 0.0);
    }

    // The trap the appendix names, demonstrated: resize(n) CONSTRUCTS n
    // elements, so n push_backs after it make 2n - the first n of them zero.
    std::vector<int> doubled;
    doubled.resize(4);
    for (int i = 1; i <= 4; ++i) {
        doubled.push_back(i);
    }
    assert(doubled.size() == 8);
    assert(doubled[0] == 0 && doubled[4] == 1);

    // A reserve no larger than the current capacity is a no-op - the
    // standard says reallocation happens if and only if the argument exceeds
    // the capacity.
    const std::size_t before = probe.capacity();
    probe.reserve(2);
    assert(probe.capacity() == before);

    // And the reserve-in-the-loop mistake, demonstrated: reserve(size() + 1)
    // before every push asks for exactly one more, and both mainstream
    // libraries give exactly what was asked - so the block moves on (nearly)
    // every push, where the amortized doubling would have moved it a
    // handful of times. Counted as data() changes.
    std::vector<int> quadratic;
    std::vector<int> amortized;
    int moves_quadratic = 0;
    int moves_amortized = 0;
    for (int i = 0; i < 64; ++i) {
        const int* was = quadratic.empty() ? nullptr : quadratic.data();
        quadratic.reserve(quadratic.size() + 1);
        quadratic.push_back(i);
        if (quadratic.data() != was) ++moves_quadratic;
        const int* was2 = amortized.empty() ? nullptr : amortized.data();
        amortized.push_back(i);
        if (amortized.data() != was2) ++moves_amortized;
    }
    assert(moves_quadratic >= 32);        // one per push on libc++ and libstdc++
    assert(moves_amortized <= 8);         // doubling: about log2(64) of them

    // The other trap cannot be run: reserve(8) followed by probe2[3] = 42
    // writes into room that holds no element - undefined behavior that
    // AddressSanitizer reports as container-overflow (Chapter 21's report
    // shape) under libc++, which annotates a vector's unused capacity by
    // default; libstdc++ does so only with -D_GLIBCXX_SANITIZE_VECTOR.
    return 0;
}
