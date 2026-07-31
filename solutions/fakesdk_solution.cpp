// FakeSDK exercise - reference solution.
#include "FakeSDK.h"
#include <cassert>
#include <iostream>

// Part A - the RAII guard. Non-copyable, non-movable: it aliases one struct
// for one scope; copying would double-dispose, moving has no use case here.
class ThingDataGuard {
    ThingData& d_;
public:
    explicit ThingDataGuard(ThingData& d) : d_(d) {}
    ~ThingDataGuard() { Thing_DisposeData(&d_); }   // safe even if never filled
    ThingDataGuard(const ThingDataGuard&) = delete;
    ThingDataGuard& operator=(const ThingDataGuard&) = delete;
};

// Part B - the worker. Flat early returns; every code checked; no leaks.
ErrCode SumAllThings(double* total, size_t* skippedCount) {
    if (!total || !skippedCount) return ErrNullParam;   // validate like the SDK does
    *total = 0;
    *skippedCount = 0;

    size_t count = 0;
    ErrCode err = Thing_GetCount(&count);
    if (err != NoErr) return err;

    for (size_t i = 0; i < count; ++i) {
        ThingData data = {};                    // zero-init: values == nullptr
        err = Thing_GetData(i, &data);
        if (err == ErrNoData) {                 // documented: nothing allocated
            ++*skippedCount;                    // on failure -> safe to just skip
            continue;
        }
        if (err != NoErr) return err;           // ditto: nothing to dispose

        ThingDataGuard guard(data);             // from here, disposal guaranteed

        double sum = 0;
        err = Thing_SumValues(&data, &sum);
        if (err != NoErr) return err;           // guard disposes on this exit
        *total += sum;
    }                                           // guard disposes each iteration
    return NoErr;
}

int main() {
    double total; size_t skipped; ErrCode err;

    // Scenario 1: 4 Things, index 2 empty. Hand-computed expectation:
    // thing0: 3 vals 0,1,2        -> 3
    // thing1: 4 vals 10..13       -> 46
    // thing2: skipped
    // thing3: 3 vals 30,31,32     -> 93        total = 142, skipped = 1
    FakeSdk_Setup(4, 2, SIZE_MAX);
    err = SumAllThings(&total, &skipped);
    assert(err == NoErr && skipped == 1 && total == 142.0);
    assert(FakeSdk_LiveAllocations() == 0);
    std::cout << "scenario1 ok: total=" << total << " skipped=" << skipped << "\n";

    // Scenario 2: Thing 2 fails transiently. Things 0,1 were read first -
    // the CRITICAL check is that their payloads were disposed on the abort.
    FakeSdk_Setup(4, SIZE_MAX, 2);
    err = SumAllThings(&total, &skipped);
    assert(err == ErrInternal);
    assert(FakeSdk_LiveAllocations() == 0);     // Finding 10: check VALUES
    std::cout << "scenario2 ok: propagated err=" << err << ", no leaks\n";

    // Scenario 3: empty project. Correct = NoErr, total 0, skipped 0.
    FakeSdk_Setup(0, SIZE_MAX, SIZE_MAX);
    err = SumAllThings(&total, &skipped);
    assert(err == NoErr && total == 0.0 && skipped == 0);
    std::cout << "scenario3 ok: empty project is a valid, zero result\n";

    // Robustness: our own null-param contract.
    assert(SumAllThings(nullptr, &skipped) == ErrNullParam);
    return 0;
}
