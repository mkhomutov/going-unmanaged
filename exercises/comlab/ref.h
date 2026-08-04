#pragma once
#include "FakeSDK2.h"
#include <utility>

// The migration notes' two sentences, encoded as two named constructors.
// adopt() wraps a reference some call already retained on your behalf
// (Thing_Acquire's +1); share() wraps a borrowed pointer by taking a
// reference of its own (Project_PeekActive). After this file, no line of
// the plug-in spells Thing_Retain or Thing_Release again.
class ThingHandle {
public:
    ThingHandle() = default;

    static ThingHandle adopt(ThingRef* raw) {    // "you own one Release"
        return ThingHandle(raw);
    }
    static ThingHandle share(ThingRef* raw) {    // "retain it if you keep it"
        if (raw != nullptr) {
            Thing_Retain(raw);
        }
        return ThingHandle(raw);
    }

    ThingHandle(const ThingHandle& other) : ref_(other.ref_) {
        if (ref_ != nullptr) {
            Thing_Retain(ref_);    // a copy duplicates the CLAIM, not the Thing
        }
    }
    ThingHandle& operator=(const ThingHandle& other) {
        ThingHandle tmp(other);              // copy-and-swap: Chapter 6's shape
        std::swap(ref_, tmp.ref_);
        return *this;                        // tmp's destructor pays our old debt
    }
    ThingHandle(ThingHandle&& other) noexcept : ref_(other.ref_) {
        other.ref_ = nullptr;                // steal and null out - Chapter 6's rule
    }
    ThingHandle& operator=(ThingHandle&& other) noexcept {
        std::swap(ref_, other.ref_);         // other's destructor pays our old debt
        return *this;
    }
    ~ThingHandle() {
        if (ref_ != nullptr) {
            Thing_Release(ref_);             // every path pays, exactly once
        }
    }

    ThingRef* get() const { return ref_; }
    explicit operator bool() const { return ref_ != nullptr; }

private:
    explicit ThingHandle(ThingRef* raw) : ref_(raw) {}

    ThingRef* ref_ = nullptr;
};
