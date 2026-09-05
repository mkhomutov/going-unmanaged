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
// torn file behind a crash. The saved file lives one directory BELOW the
// temp directory on purpose: a save_file that put its temp in
// temp_directory_path() instead of beside the file would otherwise be
// indistinguishable from the recipe, and "same directory" is the recipe's
// one claim about volumes.
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
std::string read_all_text(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open: " + path.string());
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();    // one streamed read; no line loop to get wrong
    return buffer.str();
}

// Recipe 9 - File.WriteAllText
void write_all_text(const std::filesystem::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw std::runtime_error("cannot create: " + path.string());
    }
    out << text;
    if (!out.flush()) {
        throw std::runtime_error("write failed: " + path.string());
    }
}

// Recipe 38 - File.Replace: write beside the file, then rename over it
void save_file(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::path tmp = path;
    tmp += ".tmp";                           // += on purpose: a suffix, not a segment - same directory, same volume
    write_all_text(tmp, text);               // Recipe 9: flushed and checked, or it threw and path is untouched
    std::filesystem::rename(tmp, path);      // one atomic step: a reader sees the old file or the new, never half
}

#if !defined(_WIN32)
// The identity of the file behind a name: a rename changes it, an in-place
// write does not. This is how the harness tells Recipe 38 from Recipe 9.
static ino_t inode_of(const std::filesystem::path& p) {
    struct stat st {};
    const int rc = ::stat(p.c_str(), &st);    // outside the assert: NDEBUG must not compile the call away
    assert(rc == 0);
    (void)rc;
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

    // Recipe 38. The file lives in a directory of its own below the temp
    // directory (see the banner); a first save creates it and leaves no temp
    // behind, beside the file or in the temp directory.
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / "cookbook_save_dir";
    fs::remove_all(dir);
    fs::create_directories(dir);
    const fs::path saved = dir / "cookbook_save.txt";
    const fs::path tmp = saved.string() + ".tmp";
    const fs::path elsewhere = fs::temp_directory_path() / "cookbook_save.txt.tmp";
    fs::remove(elsewhere);
    save_file(saved, "v1");
    assert(read_all_text(saved) == "v1");
    assert(!fs::exists(tmp));
    assert(!fs::exists(elsewhere));          // the temp was BESIDE the file, not in the temp directory

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
    assert(read_all_text(saved) == "v2");

    // A crash between the write and the rename: the temp file is torn, and
    // the saved file is still whole. Simulated by doing the first half by
    // hand and never doing the second.
    write_all_text(tmp, "v3 but only ha");
    assert(read_all_text(saved) == "v2");    // untouched
    assert(fs::exists(tmp));                 // the debris a crash leaves

    // The next save overwrites the debris and completes - which it can only
    // do if its temp is the same path as the debris: beside the file.
    save_file(saved, "v3");
    assert(read_all_text(saved) == "v3");
    assert(!fs::exists(tmp));

    fs::remove_all(dir);
    return 0;
}
