#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <utility>
#include <vector>

class Buffer {
public:
    explicit Buffer(size_t size)
        : size_(size), data_(new int[size]{})   // {} => zero-initialized (Finding 7)
    {}

    ~Buffer() { delete[] data_; }               // delete[] on nullptr is a safe no-op

    // ---- copy: deep, exception-safe ---------------------------------------
    Buffer(const Buffer& other)
        : size_(other.size_), data_(new int[other.size_])
    {
        std::copy(other.data_, other.data_ + size_, data_);
    }

    Buffer& operator=(const Buffer& other) {    // copy-and-swap (Finding 6)
        Buffer tmp(other);   // ALL throwing work happens here; *this untouched
        Swap(tmp);           // three noexcept pointer exchanges
        return *this;
    }                        // tmp's destructor frees our OLD block

    // ---- move: steal and null out -----------------------------------------
    Buffer(Buffer&& other) noexcept
        : size_(std::exchange(other.size_, 0)),
          data_(std::exchange(other.data_, nullptr))
    {}

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {                   // self-move guard
            delete[] data_;                     // safe HERE: nothing below throws
            size_ = std::exchange(other.size_, 0);
            data_ = std::exchange(other.data_, nullptr);
        }
        return *this;
    }

    // ---- access -------------------------------------------------------------
    size_t Size() const noexcept { return size_; }

    int&       At(size_t i)       { assert(i < size_); return data_[i]; }  // Finding 8
    const int& At(size_t i) const { assert(i < size_); return data_[i]; }

    void Swap(Buffer& other) noexcept {
        std::swap(size_, other.size_);
        std::swap(data_, other.data_);
    }

private:
    size_t size_ = 0;
    int*   data_ = nullptr;
};

int main() {
    Buffer a(5);
    a.At(2) = 42;                    // writable now (Finding 8)

    Buffer b = a;                    // copy ctor: deep copy
    Buffer c(3);
    c = a;                           // copy assignment via copy-and-swap
    std::cout << "c[2]=" << c.At(2) << " (expect 42)\n";

    const Buffer& view = a;
    std::cout << "view[2]=" << view.At(2) << " (const overload)\n";

    Buffer d = std::move(a);         // move ctor: a is now empty husk
    std::cout << "a.Size()=" << a.Size() << " (expect 0)\n";

    c = std::move(d);                // move assignment
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"   // the self-move is the TEST
#endif
    c = std::move(c);                // self-move: must be harmless
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
    std::cout << "c[2]=" << c.At(2) << " after self-move (expect 42)\n";

    std::vector<Buffer> v;
    v.push_back(Buffer(2));
    v.push_back(Buffer(4));          // reallocation: moves (noexcept honest)
    std::cout << "vector ok, sizes " << v[0].Size() << "," << v[1].Size() << "\n";
    return 0;
}
