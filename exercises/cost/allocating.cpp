// Appendix L's measurements: what an allocation costs, and what stops paying
// for it.
//
// Not an exercise - these are the numbers the appendix quotes, the way
// exercises/choosing/ holds Appendix H's. arena_alloc(), Arena, and the two
// pmr shapes are included by book/L-what-things-cost.md between their section
// markers: edit here and the page follows.
//
// The instrument is Chapter 36's, and it is the only one that can answer this
// page's question: a replaced global operator new with a counter. A timing
// could not - it would measure this machine, this run and the sanitizers -
// and the copy/move tally of exercises/choosing/ cannot see an allocation at
// all. Both forms of new are replaced, because under ASan the array form does
// not route through the scalar one (the lesson Recipe 49 paid for).
//
// The verdict is CHECK, not assert, for exercises/choosing/'s reason: assert
// compiles to nothing under -DNDEBUG, and a harness that vanishes in Release
// while still printing its success line is worse than none.
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>
#include <vector>

#if __has_include(<memory_resource>)
#  include <memory_resource>
#  define COST_HAS_PMR 1
#endif

namespace {
    std::size_t allocations = 0;
    int failures = 0;

    void check(bool ok, const char* what) {
        if (!ok) {
            std::printf("FAILED: %s\n", what);
            ++failures;
        }
    }
}

void* operator new(std::size_t size) {
    ++allocations;
    if (void* p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) {
    ++allocations;
    if (void* p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

struct Node {
    int id = 0;
    double value = 0.0;
};

// --8<-- [start:arena]
// A bump allocator: one block from the heap, then a pointer that moves. It
// gives back nothing individually and frees everything at once, which is the
// trade - and the reason it fits a frame, a request or a parse and not a
// cache. std::pmr spells this in the standard library (below); write it by
// hand once so the standard one is not magic.
class Arena {
public:
    explicit Arena(std::size_t bytes) : block_(new std::byte[bytes]), size_(bytes) {}

    template <typename T, typename... Args>
    T* Make(Args&&... args) {
        void* at = block_.get() + used_;
        std::size_t left = size_ - used_;
        // std::align does the arithmetic nobody gets right by hand: it moves
        // `at` up to T's alignment and tells you what is left.
        if (!std::align(alignof(T), sizeof(T), at, left)) {
            return nullptr;                       // the block is full: the caller's problem
        }
        used_ = size_ - left + sizeof(T);
        return new (at) T(std::forward<Args>(args)...);   // placement new: no allocation
    }

    std::size_t used() const { return used_; }

private:
    std::unique_ptr<std::byte[]> block_;
    std::size_t size_ = 0;
    std::size_t used_ = 0;
};
// --8<-- [end:arena]

// --8<-- [start:false-sharing]
// Two counters written by two threads. Adjacent, they share a cache line, and
// each write invalidates the other core's copy - the cost that does not appear
// in any profile as itself, only as a subtree that is slower than its
// arithmetic. Padding is the fix, and it is a SIZE decision: the struct grows
// by a whole line per field to stop the sharing.
struct Adjacent {
    long produced = 0;
    long consumed = 0;                          // one line, two writers
};

inline constexpr std::size_t kCacheLine = 64;   // see the appendix: not a universal 64

struct alignas(kCacheLine) Separated {
    alignas(kCacheLine) long produced = 0;
    alignas(kCacheLine) long consumed = 0;      // its own line, by construction
};
// --8<-- [end:false-sharing]

int main() {
    constexpr int kNodes = 500;

    // 1. One heap allocation per object, which is the baseline everyone has.
    {
        const std::size_t before = allocations;
        std::vector<std::unique_ptr<Node>> nodes;
        nodes.reserve(kNodes);                         // the vector's own block
        for (int i = 0; i < kNodes; ++i) {
            nodes.push_back(std::make_unique<Node>(Node{i, i * 0.5}));
        }
        const std::size_t spent = allocations - before;
        check(spent == kNodes + 1, "one allocation per node, plus the vector's block");
        std::printf("  %4zu allocations   %d nodes, one make_unique each\n", spent, kNodes);
    }

    // 2. The same objects out of one block: the count stops depending on how
    //    many there are, which is the whole claim.
    {
        const std::size_t before = allocations;
        Arena arena(kNodes * sizeof(Node) + alignof(Node) * kNodes);
        int made = 0;
        for (int i = 0; i < kNodes; ++i) {
            if (arena.Make<Node>(Node{i, i * 0.5}) != nullptr) ++made;
        }
        const std::size_t spent = allocations - before;
        check(made == kNodes, "every node fitted in the block");
        check(spent == 1, "one allocation for all of them");
        std::printf("  %4zu allocation    %d nodes, one Arena\n", spent, made);
    }

#ifdef COST_HAS_PMR
    // 3. The standard library's spelling of the same idea, and the version
    //    that touches the heap not once: the buffer is a local.
    {
        const std::size_t before = allocations;
        std::byte buffer[kNodes * sizeof(Node) * 2];
        std::pmr::monotonic_buffer_resource pool(buffer, sizeof buffer,
                                                 std::pmr::null_memory_resource());
        std::pmr::vector<Node> nodes(&pool);
        nodes.reserve(kNodes);
        for (int i = 0; i < kNodes; ++i) {
            nodes.push_back(Node{i, i * 0.5});
        }
        const std::size_t spent = allocations - before;
        check(spent == 0, "a pmr container over a stack buffer never reaches the heap");
        check(nodes.size() == static_cast<std::size_t>(kNodes), "and holds them all");
        std::printf("  %4zu allocations   %d nodes, pmr over a stack buffer\n", spent, kNodes);
    }
#else
    std::printf("  ---- <memory_resource> not available; the pmr rows are unchecked here\n");
#endif

    // 4. The layout claim, which is structural rather than timed: adjacent
    //    counters share a line, padded ones cannot.
    check(sizeof(Adjacent) <= kCacheLine, "two adjacent counters fit in one cache line");
    check(sizeof(Separated) >= 2 * kCacheLine, "padding costs a whole line per field");
    check(alignof(Separated) >= kCacheLine, "and the struct starts on one");
    std::printf("  sizeof(Adjacent) = %zu, sizeof(Separated) = %zu\n",
                sizeof(Adjacent), sizeof(Separated));

    if (failures != 0) {
        std::printf("cost: %d FAILED\n", failures);
        return 1;
    }
    std::printf("cost: all measurements hold\n");
    return 0;
}
