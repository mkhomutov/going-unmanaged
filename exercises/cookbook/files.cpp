// Appendix F, Recipe 1 - read a whole file into a string.
//
// The recipe function below is quoted VERBATIM in book/F-rosetta-cookbook.md:
// editing it means editing the appendix in the same commit (the testlab
// discipline). main() is scaffolding, not part of the recipe - it asserts
// what the recipe claims, so build_all.sh keeps the cookbook honest.
#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

std::string read_all_text(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open: " + path);
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();    // one streamed read; no line loop to get wrong
    return buffer.str();
}

int main() {
    // Make a file to read, prove the round trip, clean up.
    const std::string path = "cookbook_scratch.txt";
    {
        std::ofstream out(path, std::ios::binary);
        out << "line one\nline two\n";
    }
    assert(read_all_text(path) == "line one\nline two\n");
    std::remove(path.c_str());

    // The if (!in) line is the part File.ReadAllText did for you.
    bool threw = false;
    try {
        read_all_text("no_such_file_anywhere.txt");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
    return 0;
}
