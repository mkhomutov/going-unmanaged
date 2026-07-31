// ============================================================================
// FakeSDK.h - a miniature C-style API in the idiom of the ArchiCAD DevKit.
// DO NOT MODIFY THIS FILE. Treat it as vendor code: read it, wrap it, obey it.
//
// Conventions (same spirit as GSErrCode / API_ElementMemo):
//   - every function returns ErrCode; 0 (NoErr) means success
//   - "Get" functions fill caller-provided structs passed by address
//   - ThingData owns heap allocations made by the SDK; the caller MUST
//     release them with Thing_DisposeData exactly once
//   - passing null pointers is an error (ErrNullParam), not a crash
// ============================================================================
#pragma once
#include <cstddef>
#include <cstdint>   // SIZE_MAX, used as "no index" in FakeSdk_Setup

using ErrCode = int;

constexpr ErrCode NoErr        = 0;
constexpr ErrCode ErrNullParam = 1;   // a required pointer was null
constexpr ErrCode ErrBadIndex  = 2;   // no Thing with that index
constexpr ErrCode ErrNoData    = 3;   // Thing exists but has no payload
constexpr ErrCode ErrInternal  = 4;   // simulated transient failure

// A Thing's payload. 'values' is allocated BY THE SDK inside Thing_GetData;
// the caller owns disposal via Thing_DisposeData. All other fields are inline.
struct ThingData {
    int     id;          // stable identifier of the Thing
    size_t  valueCount;  // number of entries in 'values'
    double* values;      // SDK-allocated array; null until Thing_GetData
};

// How many Things exist in the "project". Never fails if count is non-null.
ErrCode Thing_GetCount(size_t* count);

// Fill 'data' for the Thing at 'index' (0-based).
//   - allocates data->values (caller must dispose)
//   - on ANY failure, 'data' is left untouched and nothing is allocated
// Note: some Things in the project legitimately have no payload and
// return ErrNoData. Others may fail transiently with ErrInternal.
ErrCode Thing_GetData(size_t index, ThingData* data);

// Release the payload of 'data'. Safe on a zeroed struct. After the call,
// data->values is null and valueCount is 0. Calling twice is safe;
// calling on a struct whose 'values' you overwrote by hand is not.
ErrCode Thing_DisposeData(ThingData* data);

// Sum of all entries in data->values. Requires a non-null, filled 'data'.
ErrCode Thing_SumValues(const ThingData* data, double* sum);

// Test-support: configure the fake project. 'failAtIndex' makes
// Thing_GetData return ErrInternal for that index (pass SIZE_MAX for none).
void FakeSdk_Setup(size_t thingCount, size_t emptyIndex, size_t failAtIndex);

// Test-support: how many SDK allocations are currently live.
// After your code runs, this MUST be zero - it is the leak detector.
size_t FakeSdk_LiveAllocations();
