// Appendix F, Recipes 1, 9, 38 and 49 - read and write a whole file; save one
// without losing the old one; read a large file without copying it.
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
// one claim about volumes. For Recipe 49 the judge is Chapter 36's
// instrument: a replaced operator new counts heap allocations across the
// mapping of a four-megabyte file, and the count must be zero - the recipe's
// whole claim over a ReadAllBytes - with every byte compared against Recipe
// 1's copy; on POSIX the file is then deleted under the live mapping and
// read on, and the lowest free descriptor is compared before and after, so
// a close left out of the constructor is seen. The delete-under-mapping
// assertion runs on Windows too: the STL's remove asks for POSIX delete
// semantics there (NTFS, Windows 10 1709 or later), so the name goes and
// the section keeps the bytes - the old DeleteFile refused a mapped file
// with ERROR_USER_MAPPED_FILE, and a first draft asserted that refusal
// until the buildlab-msvc job showed the runner deleting it. A munmap left out of the destructor is the one
// mistake no judge here sees: LeakSanitizer counts allocations, not
// mappings. Under the buildlab-msvc job's ASan, a replaced operator new
// costs that binary the new/delete mismatch checks (Microsoft documents
// the trade); the count is worth it, and no other TU the job builds
// replaces them.
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
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

// Recipe 49 - MemoryMappedFile.CreateFromFile: a file's bytes as a view,
// mapped rather than read. Pages arrive as they are touched and leave with
// the object; nothing is copied into the heap, and the file may be closed -
// or, on POSIX, deleted - the moment the mapping exists.
class MappedFile {
public:
    explicit MappedFile(const std::filesystem::path& path) {
#if defined(_WIN32)
        HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) throw std::runtime_error("cannot open " + path.string());
        LARGE_INTEGER size{};
        if (!GetFileSizeEx(file, &size)) { CloseHandle(file); throw std::runtime_error("cannot size " + path.string()); }
        if (size.QuadPart == 0) { CloseHandle(file); return; }      // a zero-length mapping is refused: an empty view instead
        HANDLE mapping = CreateFileMappingW(file, nullptr, PAGE_READONLY, 0, 0, nullptr);
        CloseHandle(file);                                           // the mapping object holds its own reference
        if (mapping == nullptr) throw std::runtime_error("cannot map " + path.string());
        view_ = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
        CloseHandle(mapping);                                        // and the view holds its own
        if (view_ == nullptr) throw std::runtime_error("cannot view " + path.string());
        size_ = static_cast<std::size_t>(size.QuadPart);
#else
        const int fd = ::open(path.c_str(), O_RDONLY);
        if (fd < 0) throw std::runtime_error("cannot open " + path.string());
        struct stat st{};
        if (::fstat(fd, &st) != 0) { ::close(fd); throw std::runtime_error("cannot size " + path.string()); }
        if (st.st_size == 0) { ::close(fd); return; }                // mmap of length 0 is EINVAL: an empty view instead
        void* view = ::mmap(nullptr, static_cast<std::size_t>(st.st_size), PROT_READ, MAP_PRIVATE, fd, 0);
        ::close(fd);                                                 // the mapping keeps its own reference to the file
        if (view == MAP_FAILED) throw std::runtime_error("cannot map " + path.string());
        view_ = view;
        size_ = static_cast<std::size_t>(st.st_size);
#endif
    }
    ~MappedFile() {
        if (view_ == nullptr) return;
#if defined(_WIN32)
        UnmapViewOfFile(view_);
#else
        ::munmap(const_cast<void*>(view_), size_);
#endif
    }
    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    // A view into the mapping: valid exactly as long as this object is.
    std::string_view bytes() const { return {static_cast<const char*>(view_), size_}; }

private:
    const void* view_ = nullptr;
    std::size_t size_ = 0;
};

// The harness's judge for Recipe 49: a replaced operator new, Chapter 36's
// instrument, so "no copy" is a count and not a claim. Scaffolding.
static std::size_t g_allocations = 0;
void* operator new(std::size_t size) {
    ++g_allocations;
    if (void* p = std::malloc(size)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return operator new(size); }   // the array form too: under ASan it does not route through the scalar one
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

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

    // Recipe 49. Four megabytes, every byte its index, mapped and compared
    // against Recipe 1's copy - and the mapping allocates nothing on the heap,
    // which is the recipe's claim over a ReadAllBytes: the count is zero.
    const fs::path big = fs::temp_directory_path() / "cookbook_mapped.bin";
    {
        std::string pattern(4u << 20, '\0');
        for (std::size_t i = 0; i < pattern.size(); ++i) pattern[i] = static_cast<char>(i % 251);
        std::ofstream(big, std::ios::binary) << pattern;
    }
    const std::string copied = read_all_text(big);
    {
        const std::size_t allocs_before = g_allocations;
        MappedFile mapped(big);
        const std::size_t during = g_allocations - allocs_before;
        assert(mapped.bytes().size() == copied.size());
        assert(mapped.bytes() == copied);                           // every byte, through the mapping
        assert(during == 0);                                        // and not one heap allocation to get them
        // The mapping holds its own reference to the file: the name can go
        // and the bytes stay. On POSIX by design; on Windows because the
        // STL's remove uses POSIX delete semantics where the volume allows
        // (the msvc job's NTFS runner does).
        fs::remove(big);
        assert(!fs::exists(big));
        assert(mapped.bytes().substr(1000, 5) == std::string_view(copied).substr(1000, 5));
    }
    fs::remove(big);

    // An empty file is an empty view, not a failed mmap (length 0 is EINVAL).
    const fs::path empty = fs::temp_directory_path() / "cookbook_mapped_empty.bin";
    std::ofstream(empty, std::ios::binary).close();
    {
        MappedFile mapped(empty);
        assert(mapped.bytes().empty());
    }
    fs::remove(empty);

    // A missing file throws, like Recipe 1.
    threw = false;
    try {
        MappedFile mapped("no_such_file_anywhere.bin");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);

#if !defined(_WIN32)
    // The descriptor is closed the moment the mapping exists: the lowest free
    // descriptor is the same before and after a mapping lived (shm.cpp's judge).
    const int fd_before = ::open("/dev/null", O_RDONLY);
    ::close(fd_before);
    {
        std::ofstream(empty, std::ios::binary) << "x";
        MappedFile mapped(empty);
        assert(mapped.bytes() == "x");
        const int fd_during = ::open("/dev/null", O_RDONLY);
        ::close(fd_during);
        assert(fd_during == fd_before);                             // closed before the view was handed out, not after
    }
    fs::remove(empty);
#endif
    return 0;
}
