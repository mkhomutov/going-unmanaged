// Appendix F, Recipes 10-12 and 39 - paths: combine, the exists pair,
// listing; create, copy, move and delete, and a whole tree.
//
// The recipe functions below are quoted VERBATIM in book/F-rosetta-cookbook.md:
// editing one means editing the appendix in the same commit (the testlab
// discipline). main() is scaffolding - it builds a small directory tree,
// asserts what the recipes claim (the unspecified listing order included,
// and Recipe 39's trap: dir / "" is dir/, so an empty entry name hands
// remove_all the directory itself), and removes the tree again. The export
// tree starts with a report and NO archive directory, so create_directories
// is load-bearing on the first rotation; and on POSIX the moved report must
// keep its inode, so a copy-and-remove cannot pass as File.Move. Two checks
// are per platform and say so: the cross-volume rename refusal that Recipe
// 38's Why names is the POSIX libraries' behaviour (MSVC copies instead) and
// runs only on Linux, where /dev/shm is a second volume the CI runner has;
// the u8path round trip runs only on Windows, the one platform where a path
// is not made of char and the two constructors differ.
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>
#if !defined(_WIN32)
#include <sys/stat.h>
#endif

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

// Recipe 39 - Directory.CreateDirectory, File.Copy, File.Move, Directory.Delete
void rotate_export(const fs::path& export_dir, const fs::path& fresh_report) {
    fs::create_directories(export_dir / "archive");           // parents included; already there is not an error
    const fs::path current = export_dir / "report.txt";
    if (fs::exists(current)) {
        fs::copy_file(current, export_dir / "archive" / "previous.txt",
                      fs::copy_options::overwrite_existing);  // File.Copy(overwrite: true): both defaults refuse
    }
    fs::rename(fresh_report, current);                        // File.Move onto the name - which REPLACES here, and throws in C#
}

std::uintmax_t purge(const fs::path& dir) {
    return fs::remove_all(dir);    // Directory.Delete(recursive: true): the count removed, 0 if nothing was there
}

static void write(const fs::path& p, const std::string& text) {
    std::ofstream(p, std::ios::binary) << text;
}

static std::string read(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

#if !defined(_WIN32)
// File identity, as in files.cpp: a rename keeps the moved file's inode, a
// copy-and-remove makes a new one. How the harness holds "File.Move".
static ino_t inode_of(const fs::path& p) {
    struct stat st {};
    const int rc = ::stat(p.c_str(), &st);
    assert(rc == 0);
    (void)rc;
    return st.st_ino;
}
#endif

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

    // Recipe 39. The export directory already holds a report and NO archive
    // (an older layout) - so the first rotation must create the parent, the
    // second archives, the third overwrites the archive: the line the
    // default would refuse.
    const fs::path exp = fs::temp_directory_path() / "cookbook_export";
    fs::remove_all(exp);
    fs::create_directories(exp);
    write(exp / "report.txt", "zero");
    const fs::path fresh = fs::temp_directory_path() / "cookbook_fresh.txt";

    write(fresh, "one");
    rotate_export(exp, fresh);                               // archive/ did not exist: create_directories earns its keep
    assert(read(exp / "report.txt") == "one");
    assert(!fs::exists(fresh));                              // moved, not copied
    assert(fs::is_directory(exp / "archive"));
    assert(read(exp / "archive" / "previous.txt") == "zero");
    assert(fs::file_size(exp / "report.txt") == 3);

    write(fresh, "two");
#if !defined(_WIN32)
    const ino_t fresh_inode = inode_of(fresh);
#endif
    rotate_export(exp, fresh);
    assert(read(exp / "report.txt") == "two");
    assert(read(exp / "archive" / "previous.txt") == "one");
#if !defined(_WIN32)
    assert(inode_of(exp / "report.txt") == fresh_inode);      // the SAME file, moved - a copy-and-remove fails here
#endif

    const auto written_two = fs::last_write_time(exp / "report.txt");
    write(fresh, "three");
    rotate_export(exp, fresh);                               // previous.txt exists: overwrite_existing earns its keep
    assert(read(exp / "report.txt") == "three");
    assert(read(exp / "archive" / "previous.txt") == "two");
    // A moved file keeps its own time: "three" was written after "two", so
    // the name's timestamp did not go backwards (>=, because two writes
    // inside one filesystem tick compare equal). Documentation, not a judge.
    assert(fs::last_write_time(exp / "report.txt") >= written_two);

    // copy_file's default matches File.Copy's, and both refuse loudly: an
    // existing target is an error until overwrite is spelled out.
    bool refused = false;
    try {
        fs::copy_file(exp / "report.txt", exp / "archive" / "previous.txt");
    } catch (const fs::filesystem_error& e) {
        refused = (e.code() == std::errc::file_exists);
    }
    assert(refused);

    // The trap: dir / "" is dir/ - the separator and nothing after it - so an
    // entry name that came back empty from a lookup hands remove_all the
    // directory itself. Verified rather than reasoned: two files, a
    // subdirectory and the directory go - four entries, never one.
    const std::string entry;                                 // "" - the lookup found nothing
    assert(exp / entry != exp && (exp / entry).filename().empty());
    assert(purge(exp / entry) == 4);                         // report.txt, archive/, previous.txt, and exp itself
    assert(!fs::exists(exp));
    assert(purge(exp) == 0);                                 // Directory.Delete would throw; remove_all counts zero

#if defined(__linux__)
    // Recipe 38's Why: on the POSIX standard libraries a rename across
    // volumes is refused, not quietly copied (MSVC's copies - it passes
    // MOVEFILE_COPY_ALLOWED - which is why this check is per platform).
    // /dev/shm is a tmpfs on the Linux CI runner, a second volume beside /tmp.
    if (fs::is_directory("/dev/shm")) {
        const fs::path on_shm = "/dev/shm/cookbook_cross_volume.txt";
        const fs::path on_tmp = fs::temp_directory_path() / "cookbook_cross_volume.txt";
        write(on_shm, "x");
        assert(fs::exists(on_shm));                          // a read-only /dev/shm would tell a different story
        std::error_code ec;
        fs::rename(on_shm, on_tmp, ec);
        std::cout << "cross-volume rename: " << (ec ? ec.message() : "succeeded (same volume)") << "\n";
        assert(ec == std::errc::cross_device_link);
        assert(fs::exists(on_shm));                          // nothing moved, nothing copied
        fs::remove(on_shm);
        fs::remove(on_tmp);
    } else {
        std::cout << "cross-volume rename: SKIPPED - no /dev/shm here\n";
    }
#endif

#if defined(_WIN32)
    // Recipe 10's encoding note: on Windows a path is wchar_t, and only
    // u8path reads a std::string as UTF-8. "Grüße" is 5 characters, 7 UTF-8
    // bytes, and must come back as 5 wide characters and the same 7 bytes.
    // u8path is the C++17 spelling (C++20 deprecates it for path(u8"...")),
    // and u8string() returns std::u8string there - hence the byte-wise
    // comparison, which holds under both standards.
    const std::string utf8 = "Gr\xC3\xBC\xC3\x9F" "e";
    const fs::path p = fs::u8path(utf8);
    assert(p.native().size() == 5);
    const auto back = p.u8string();
    assert(back.size() == utf8.size());
    assert(std::equal(back.begin(), back.end(), utf8.begin(),
                      [](auto a, char b) { return static_cast<char>(a) == b; }));
    std::cout << "u8path: 7 UTF-8 bytes -> " << p.native().size() << " wide characters; "
              << "the plain constructor made " << fs::path(utf8).native().size() << "\n";
#endif
    return 0;
}
