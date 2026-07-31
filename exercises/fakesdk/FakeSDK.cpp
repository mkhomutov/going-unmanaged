// ============================================================================
// FakeSDK.cpp - vendor-side implementation. Compile it, link it, don't edit it.
// ============================================================================
#include "FakeSDK.h"
#include <cstdint>

namespace {
    size_t g_count = 0;
    size_t g_empty = SIZE_MAX;
    size_t g_fail  = SIZE_MAX;
    size_t g_live  = 0;          // live SDK allocations (leak detector)
}

void FakeSdk_Setup(size_t thingCount, size_t emptyIndex, size_t failAtIndex) {
    g_count = thingCount;
    g_empty = emptyIndex;
    g_fail  = failAtIndex;
    g_live  = 0;
}

size_t FakeSdk_LiveAllocations() { return g_live; }

ErrCode Thing_GetCount(size_t* count) {
    if (!count) return ErrNullParam;
    *count = g_count;
    return NoErr;
}

ErrCode Thing_GetData(size_t index, ThingData* data) {
    if (!data)             return ErrNullParam;
    if (index >= g_count)  return ErrBadIndex;
    if (index == g_fail)   return ErrInternal;
    if (index == g_empty)  return ErrNoData;

    const size_t n = 3 + index % 3;            // 3..5 values per Thing
    double* v = new double[n];
    for (size_t i = 0; i < n; ++i)
        v[i] = static_cast<double>(index * 10 + i);

    data->id = static_cast<int>(1000 + index);
    data->valueCount = n;
    data->values = v;
    ++g_live;
    return NoErr;
}

ErrCode Thing_DisposeData(ThingData* data) {
    if (!data) return ErrNullParam;
    if (data->values) {
        delete[] data->values;
        --g_live;
    }
    data->values = nullptr;
    data->valueCount = 0;
    return NoErr;
}

ErrCode Thing_SumValues(const ThingData* data, double* sum) {
    if (!data || !sum)  return ErrNullParam;
    if (!data->values)  return ErrNoData;
    double s = 0;
    for (size_t i = 0; i < data->valueCount; ++i)
        s += data->values[i];
    *sum = s;
    return NoErr;
}
