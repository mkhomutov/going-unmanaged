// Appendix F, Recipe 6 - time a call.
//
// report_batch_time() is quoted VERBATIM in book/F-rosetta-cookbook.md:
// editing it means editing the appendix in the same commit (the testlab
// discipline). run_the_batch() stands for whatever is being timed; main()
// is scaffolding.
#include <chrono>
#include <iostream>

void run_the_batch() {
    // Just enough work to be measurable; volatile so the optimizer cannot
    // collapse the loop into one addition (Chapter 29's lesson).
    volatile long long sink = 0;
    for (int i = 0; i < 1000000; ++i) {
        sink = sink + i;
    }
}

void report_batch_time() {
    const auto start = std::chrono::steady_clock::now();
    run_the_batch();    // the code being timed
    const auto elapsed = std::chrono::steady_clock::now() - start;
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    std::cout << ms.count() << " ms\n";
}

int main() {
    report_batch_time();
    return 0;
}
