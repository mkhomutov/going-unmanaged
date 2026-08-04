// FakeSDK2.cpp - vendor code. DO NOT MODIFY. Nothing in here validates your
// reference counting; like the real thing, it simply believes you.
#include "FakeSDK2.h"

struct ThingRef {
    int     id;
    size_t  refs;
    double* values;
    size_t  valueCount;
};

namespace {
    ThingRef** g_table  = nullptr;   // the project's own references
    size_t     g_count  = 0;
    size_t     g_active = 0;
    size_t     g_live   = 0;

    ThingRef* make_thing(int id) {
        ThingRef* t = new ThingRef;
        t->id = id;
        t->refs = 1;                 // the project's reference
        t->valueCount = 3;
        t->values = new double[3];
        for (size_t k = 0; k < 3; ++k) {
            t->values[k] = id;       // sum of a Thing's values = 3 * id
        }
        ++g_live;
        return t;
    }

    void release_project() {
        for (size_t i = 0; i < g_count; ++i) {
            Thing_Release(g_table[i]);
        }
        delete[] g_table;
        g_table = nullptr;
        g_count = 0;
    }
}

ErrCode Project_GetCount(size_t* count) {
    if (count == nullptr) return ErrNullParam;
    *count = g_count;
    return NoErr;
}

ErrCode Thing_Acquire(size_t index, ThingRef** out) {
    if (out == nullptr) return ErrNullParam;
    if (g_table == nullptr || index >= g_count) return ErrBadIndex;
    ThingRef* t = g_table[index];
    ++t->refs;                       // retained on the caller's behalf
    *out = t;
    return NoErr;
}

ThingRef* Project_PeekActive() {
    if (g_table == nullptr || g_active >= g_count) return nullptr;
    return g_table[g_active];        // the project's pointer, borrowed
}

size_t Thing_Retain(ThingRef* ref) {
    if (ref == nullptr) return 0;
    return ++ref->refs;
}

size_t Thing_Release(ThingRef* ref) {
    if (ref == nullptr) return 0;
    const size_t left = --ref->refs;
    if (left == 0) {
        delete[] ref->values;
        delete ref;
        --g_live;
    }
    return left;
}

ErrCode Thing_Sum(const ThingRef* ref, double* sum) {
    if (ref == nullptr || sum == nullptr) return ErrNullParam;
    double total = 0.0;
    for (size_t k = 0; k < ref->valueCount; ++k) {
        total += ref->values[k];
    }
    *sum = total;
    return NoErr;
}

void FakeSdk2_Setup(size_t thingCount, size_t activeIndex) {
    release_project();
    g_table = new ThingRef*[thingCount];
    for (size_t i = 0; i < thingCount; ++i) {
        g_table[i] = make_thing(static_cast<int>(i) + 1);
    }
    g_count = thingCount;
    g_active = activeIndex;
}

void FakeSdk2_Shutdown() {
    release_project();
}

size_t FakeSdk2_LiveObjects() {
    return g_live;
}
