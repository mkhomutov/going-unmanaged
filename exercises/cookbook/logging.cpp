// Appendix F, Recipes 15 and 24 - print a diagnostic you will actually see;
// compile a diagnostic out of Release.
//
// The three functions below are quoted VERBATIM in book/F-rosetta-cookbook.md:
// editing one means editing the appendix in the same commit (the testlab
// discipline). main() is scaffolding - it captures cerr into a buffer to
// assert the text, so a green run stays silent on stderr. build_all.sh
// builds this TU twice, the second time with -DNDEBUG, because Recipe 24's
// claim is about what vanishes under that define.
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

// Recipe 24 - [Conditional("DEBUG")]
void check_channel_count([[maybe_unused]] int channels) {          // used only in Debug
    assert(channels > 0 && "a session has at least one channel");   // gone under NDEBUG
#ifndef NDEBUG
    std::cerr << "[debug] channels=" << channels << '\n';           // and so is this block
#endif
}

int main() {
    std::ostringstream captured;
    std::streambuf* old = std::cerr.rdbuf(captured.rdbuf());
    report_failure("cannot open settings.ini");
    std::cerr.rdbuf(old);
    assert(captured.str() == "error: cannot open settings.ini\n");

    report_progress(3, 10);    // stdout; build_all.sh silences it

    // Recipe 24: both spellings compile to nothing under NDEBUG. Plain ifs,
    // because assert itself is the thing under test.
    std::ostringstream debug_lines;
    old = std::cerr.rdbuf(debug_lines.rdbuf());
    check_channel_count(2);
    std::cerr.rdbuf(old);
    int bumps = 0;
    assert(++bumps == 1);      // the trap: a side effect inside an assert
#ifdef NDEBUG
    if (!debug_lines.str().empty() || bumps != 0) return 1;   // both vanished with the define
#else
    if (debug_lines.str() != "[debug] channels=2\n" || bumps != 1) return 1;
#endif
    return 0;
}
