#include "FakeSDK2.h"
#include "ref.h"
#include <cstdio>

int main() {
    FakeSdk2_Setup(5, 2);    // five Things; the third is active

    double total = 0.0;
    double bestSum = -1.0;
    double activeSum = 0.0;
    {
        size_t count = 0;
        Project_GetCount(&count);
        ThingHandle best;
        for (size_t i = 0; i < count; ++i) {
            ThingRef* raw = nullptr;
            if (Thing_Acquire(i, &raw) != NoErr) {
                continue;
            }
            ThingHandle t = ThingHandle::adopt(raw);    // Acquire's +1 is ours
            double sum = 0.0;
            if (Thing_Sum(t.get(), &sum) != NoErr) {
                continue;                    // t still releases on this path
            }
            total += sum;
            if (sum > bestSum) {
                bestSum = sum;
                best = t;                    // a copy: one more claim, retained
            }
        }

        ThingHandle active = ThingHandle::share(Project_PeekActive());
        if (active) {
            Thing_Sum(active.get(), &activeSum);
        }
    }    // every handle returns its references here

    FakeSdk2_Shutdown();     // the host closes the document...
    const size_t live = FakeSdk2_LiveObjects();

    std::printf("total %.1f, best %.1f, active %.1f, live at unload %zu\n",
                total, bestSum, activeSum, live);
    if (total != 45.0 || bestSum != 15.0 || activeSum != 9.0 || live != 0) {
        std::printf("FAILED: the ledger does not balance\n");
        return 1;
    }
    return 0;
}
