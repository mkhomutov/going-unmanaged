// Appendix F, Recipes 1, 9 and 38 - read and write a whole file; save one
// without losing the old one.
//
// The recipe functions below are quoted VERBATIM in book/F-rosetta-cookbook.md:
// editing one means editing the appendix in the same commit (the testlab
// discipline). main() is scaffolding, not part of any recipe - it asserts
// what the recipes claim, so build_all.sh keeps the cookbook honest. For
// Recipe 38 the load-bearing assertion is POSIX-only: after a save the name
// must refer to a NEW file (a different inode), because a save that rewrote
// the old file in place would pass every other check here and still leave a
// torn file behind a crash.
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#if !defined(_WIN32)
#include <sys/stat.h>
#endif

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

// Recipe 38 - File.Replace: write beside the file, then rename over it
void save_file(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::path tmp = path;
    tmp += ".tmp";                           // same directory: the rename never leaves the volume
    write_all_text(tmp.string(), text);      // Recipe 9: flushed and checked, or it threw and path is untouched
    std::filesystem::rename(tmp, path);      // one atomic step: a reader sees the old file or the new, never half
}

#if !defined(_WIN32)
// The identity of the file behind a name: a rename changes it, an in-place
// write does not. This is how the harness tells Recipe 38 from Recipe 9.
static ino_t inode_of(const std::filesystem::path& p) {
    struct stat st {};
    assert(::stat(p.c_str(), &st) == 0);
    return st.st_ino;
}
#endif

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

    // Recipe 38. A first save creates the file and leaves no temp behind.
    namespace fs = std::filesystem;
    const fs::path saved = fs::temp_directory_path() / "cookbook_save.txt";
    const fs::path tmp = saved.string() + ".tmp";
    fs::remove(saved);
    fs::remove(tmp);
    save_file(saved, "v1");
    assert(read_all_text(saved.string()) == "v1");
    assert(!fs::exists(tmp));

#if !defined(_WIN32)
    // The claim that matters: a save REPLACES the file rather than rewriting
    // it, so the name refers to a different inode afterwards. Rewrite
    // save_file as write_all_text(path, text) and this is the line that fails.
    const ino_t before = inode_of(saved);
    save_file(saved, "v2");
    assert(inode_of(saved) != before);
#else
    save_file(saved, "v2");
#endif
    assert(read_all_text(saved.string()) == "v2");

    // A crash between the write and the rename: the temp file is torn, and
    // the saved file is still whole. Simulated by doing the first half by
    // hand and never doing the second.
    write_all_text(tmp.string(), "v3 but only ha");
    assert(read_all_text(saved.string()) == "v2");     // untouched
    assert(fs::exists(tmp));                           // the debris a crash leaves

    // The next save overwrites the debris and completes.
    save_file(saved, "v3");
    assert(read_all_text(saved.string()) == "v3");
    assert(!fs::exists(tmp));

    fs::remove(saved);
    return 0;
}
