// Appendix F, Recipe 7 - wrap a C handle so it frees itself.
//
// The alias and function below are quoted VERBATIM in
// book/F-rosetta-cookbook.md: editing them means editing the appendix in the
// same commit (the testlab discipline). main() is scaffolding - it asserts
// both paths the appendix claims: scope end closes the file, and a failed
// open is a null handle that no deleter ever touches.
#include <cassert>
#include <cstdio>
#include <memory>

using FileHandle = std::unique_ptr<std::FILE, int (*)(std::FILE*)>;

FileHandle open_file(const char* path, const char* mode) {
    return FileHandle(std::fopen(path, mode), &std::fclose);
}

int main() {
    const char* path = "cookbook_handle.txt";
    {
        FileHandle file = open_file(path, "w");
        assert(file);    // fopen failure would be a null handle
        std::fputs("owned\n", file.get());
    }    // scope end IS the fclose - the C# using block, as a type

    // unique_ptr never calls the deleter on null, so a failed open needs
    // no special-casing - test the handle, like the C API taught you.
    FileHandle missing = open_file("no_such_dir_xyz/f.txt", "r");
    assert(!missing);

    std::remove(path);
    return 0;
}
