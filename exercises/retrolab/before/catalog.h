// catalog.h - the native layer's key/value catalog, as it has shipped since
// 2009. Callers in three teams include this header; none of them may change.
//
// The STARTING POINT of Chapter 45's retrofit, included whole by the chapter
// from below this banner. It works, it is green under the flags, and it is
// the class you modernise in a copy of your own - never edit it here.
// --8<-- [start:listing]
#ifndef CATALOG_H
#define CATALOG_H

struct Entry {
    char key[32];
    int  value;
};

class Catalog {
public:
    Catalog();
    ~Catalog();

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
    void Grow();

    Entry** entries_;
    int     count_;
    int     capacity_;
};

#endif
// --8<-- [end:listing]
