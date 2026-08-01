// Buffer.h - the Chapter 15 reference solution's class, extracted.
//
// It lived next to main() in buffer.cpp until Chapter 28 pointed out that code
// reachable only from a .cpp with an entry point cannot be tested: a test binary
// brings its own main and two will not link. So the class moved here, and both
// buffer.cpp (the demo) and exercises/testlab/buffer_test.cpp (the suite)
// include it. The split IS the chapter's lesson, applied to this repository.
#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <utility>

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
