// catalog.h - the native layer's key/value catalog, after the retrofit.
// Every declaration a caller can name is unchanged: the three teams' files
// compile against this header byte for byte as they did against the 2009
// one. What is new is what the class always needed and never declared.
//
// The RESULT of Chapter 45's four seams, included whole by the chapter from
// below this banner: edit here and the page follows.
// --8<-- [start:listing]
#ifndef CATALOG_H
#define CATALOG_H

#include <memory>
#include <vector>

struct Entry {
    char key[32];
    int  value;
};

class Catalog {
public:
    Catalog();
    ~Catalog();

    // Seam 4: the copy the class silently had was a shallow one, which is
    // why nobody was allowed to copy a Catalog. Now it is a deep one, and
    // the snapshot feature can.
    Catalog(const Catalog& other);
    Catalog& operator=(const Catalog& other);
    Catalog(Catalog&& other) noexcept;
    Catalog& operator=(Catalog&& other) noexcept;

    // Parses "key=value;key=value;..." into the catalog. Returns the number
    // of entries added, or -1 if the text is malformed (nothing added).
    int Parse(const char* text);

    // Adds or replaces one entry. The catalog keeps its own copy of the key.
    void Add(const char* key, int value);

    // Borrowed: the pointer stays valid until the entry is removed or the
    // catalog is cleared - adding entries does NOT move existing ones.
    const Entry* Find(const char* key) const;

    // C-style: true and *out filled if the key exists.
    bool TryGet(const char* key, int* out) const;

    int  Count() const;
    void Clear();

private:
    // Seam 2: the storage owns. One unique_ptr per entry, not a vector of
    // entries, because Find's promise - the address stays put while the
    // catalog grows - is part of the contract, and a vector<Entry> would
    // break every caller that holds a pointer across an Add without
    // changing a line of their code.
    std::vector<std::unique_ptr<Entry>> entries_;
};

#endif
// --8<-- [end:listing]
