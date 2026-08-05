#include "session.h"
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {
    // Device configuration under test. build_all.sh runs BOTH: 1 is the
    // bench's calibrated unit, 0 the field's base model - the crash lived
    // only in the second, and one configuration cannot prove both.
    const bool calibrated = !(argc > 1 && std::atoi(argv[1]) == 0);

    Session s(calibrated);
    for (int i = 1; i <= 8; ++i) {
        s.Ingest(static_cast<double>(i));    // mean 4.5, exactly
    }
    const double report = s.Report();

    const double expected = calibrated ? 4.5 * 0.5 + 1.0 : 4.5;
    if (report != expected) {
        std::printf("FAILED: report %.3f, expected %.3f (calibrated=%d)\n",
                    report, expected, calibrated ? 1 : 0);
        return 1;
    }
    std::printf("session ok: %s device, report %.2f\n",
                calibrated ? "calibrated" : "base-model", report);
    return 0;
}
