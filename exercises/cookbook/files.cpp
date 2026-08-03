// Appendix F, Recipes 1 and 9 - read and write a whole file.
//
// The recipe functions below are quoted VERBATIM in book/F-rosetta-cookbook.md:
// editing one means editing the appendix in the same commit (the testlab
// discipline). main() is scaffolding, not part of any recipe - it asserts
// what the recipes claim, so build_all.sh keeps the cookbook honest.
#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

// Recipe 1 - File.ReadAllText
std::string read_all_text(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open: " + path);
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();    // one streamed read; no line loop to get wrong
    return buffer.str();
}

// Recipe 9 - File.WriteAllText
void write_all_text(const std::string& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("cannot create: " + path);
    }
    out << text;
    if (!out.flush()) {
        throw std::runtime_error("write failed: " + path);
    }
}

int main() {
    // Round-trip the pair, then clean up.
    const std::string path = "cookbook_scratch.txt";
    write_all_text(path, "line one\nline two\n");
    assert(read_all_text(path) == "line one\nline two\n");
    std::remove(path.c_str());

    // The if (!in) / if (!out) checks are the part the C# API did for you.
    bool threw = false;
    try {
        read_all_text("no_such_file_anywhere.txt");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);

    threw = false;
    try {
        write_all_text("no_such_dir_xyz/f.txt", "x");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
    return 0;
}
