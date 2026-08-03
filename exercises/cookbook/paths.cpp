// Appendix F, Recipes 10-12 - paths: combine, the exists pair, listing.
//
// The recipe functions below are quoted VERBATIM in book/F-rosetta-cookbook.md:
// editing one means editing the appendix in the same commit (the testlab
// discipline). main() is scaffolding - it builds a small directory tree,
// asserts what the recipes claim (the unspecified listing order included),
// and removes the tree again.
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>

// Recipe 10 - Path.Combine
std::filesystem::path log_path(const std::filesystem::path& dir) {
    return dir / "logs" / "app.txt";    // '/' inserts the platform's separator
}

// Recipe 11 - File.Exists / Directory.Exists
namespace fs = std::filesystem;

bool config_present(const fs::path& p) {
    return fs::is_regular_file(p);    // File.Exists: it exists AND is a file
}

bool logs_dir_present(const fs::path& p) {
    return fs::is_directory(p);       // Directory.Exists: exists AND is a directory
}

// Recipe 12 - Directory.GetFiles
std::vector<std::filesystem::path> list_files(const std::filesystem::path& dir) {
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path());
        }
    }
    return files;
}

int main() {
    const fs::path dir = "cookbook_paths_dir";
    fs::create_directories(dir / "logs" / "sub");
    std::ofstream(dir / "logs" / "app.txt").put('\n');
    std::ofstream(dir / "logs" / "boot.txt").put('\n');

    // Recipe 10, and its trap: one character apart, separator or glue.
    assert(log_path(dir) == dir / "logs" / "app.txt");
    fs::path glued = dir;
    glued += "logs";    // string concatenation - no separator
    assert(glued == fs::path("cookbook_paths_dirlogs"));

    // Recipe 11: each check is exists AND the right kind, like its C# name.
    assert(config_present(dir / "logs" / "app.txt"));
    assert(!config_present(dir / "logs"));    // a directory is not File.Exists
    assert(logs_dir_present(dir / "logs"));
    assert(!logs_dir_present(dir / "logs" / "app.txt"));
    assert(!config_present(dir / "nope.txt"));

    // Recipe 12: files only (the sub/ directory is skipped), and the order is
    // unspecified - sorting before comparing is the trap's own advice.
    auto files = list_files(dir / "logs");
    std::sort(files.begin(), files.end());
    assert(files.size() == 2);
    assert(files[0].filename() == "app.txt");
    assert(files[1].filename() == "boot.txt");

    fs::remove_all(dir);
    return 0;
}
