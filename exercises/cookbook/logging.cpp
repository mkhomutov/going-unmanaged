// Appendix F, Recipe 15 - print a diagnostic you will actually see.
//
// The two functions below are quoted VERBATIM in book/F-rosetta-cookbook.md:
// editing one means editing the appendix in the same commit (the testlab
// discipline). main() is scaffolding - it captures cerr into a buffer to
// assert the text, so a green run stays silent on stderr.
#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

void report_progress(int done, int total) {
    std::cout << "processed " << done << " of " << total << '\n';    // buffered: fast
}

void report_failure(const std::string& what) {
    std::cerr << "error: " << what << '\n';    // unbuffered: survives a crash
}

int main() {
    std::ostringstream captured;
    std::streambuf* old = std::cerr.rdbuf(captured.rdbuf());
    report_failure("cannot open settings.ini");
    std::cerr.rdbuf(old);
    assert(captured.str() == "error: cannot open settings.ini\n");

    report_progress(3, 10);    // stdout; build_all.sh silences it
    return 0;
}
