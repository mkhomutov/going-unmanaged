// catalog.cpp - the implementation after the retrofit. Chapter 45 includes
// two pieces of it between section markers - the copy and move operations
// (seam 4) and Add/Find (seam 2, the storage that keeps the address promise);
// edit here and the page follows.
#include "catalog.h"
#include <cstdlib>
#include <cstring>
#include <utility>

Catalog::Catalog() = default;

// Seam 1: the destructor no longer has anything to do by hand. Clear() is
// still a public function, because callers call it; it just is not the
// destructor's job any more.
Catalog::~Catalog() = default;

// --8<-- [start:copy-and-move]
// Seam 4: copy is a copy of the ENTRIES, one allocation each, so the two
// catalogs share nothing; move steals the vector and leaves the source
// empty, which is a valid Catalog (Count() == 0). Copy-and-swap makes
// assignment self-safe and exception-safe for free (Chapter 6).
Catalog::Catalog(const Catalog& other) {
    entries_.reserve(other.entries_.size());
    for (const auto& e : other.entries_) {
        entries_.push_back(std::make_unique<Entry>(*e));
    }
}

Catalog& Catalog::operator=(const Catalog& other) {
    Catalog copy(other);
    entries_.swap(copy.entries_);
    return *this;
}

Catalog::Catalog(Catalog&& other) noexcept = default;
Catalog& Catalog::operator=(Catalog&& other) noexcept = default;
// --8<-- [end:copy-and-move]

// --8<-- [start:add]
void Catalog::Add(const char* key, int value) {
    for (auto& e : entries_) {
        if (std::strcmp(e->key, key) == 0) {
            e->value = value;                // replace in place: the address stays
            return;
        }
    }
    auto e = std::make_unique<Entry>();
    std::strncpy(e->key, key, sizeof e->key - 1);
    e->key[sizeof e->key - 1] = '\0';
    e->value = value;
    entries_.push_back(std::move(e));        // the vector may move: the Entry does not
}

// --8<-- [end:add]

int Catalog::Parse(const char* text) {
    // Validate first, so a malformed text adds nothing - unchanged from
    // 2009, because the callers' malformed-input behaviour is part of what
    // they compiled against.
    for (const char* p = text; *p; ) {
        const char* eq = std::strchr(p, '=');
        if (!eq || eq == p || eq - p >= 32) return -1;
        char* end;
        std::strtol(eq + 1, &end, 10);
        if (end == eq + 1 || (*end != ';' && *end != '\0')) return -1;
        p = *end == ';' ? end + 1 : end;
    }
    int added = 0;
    for (const char* p = text; *p; ) {
        const char* eq = std::strchr(p, '=');
        char key[32];
        std::memcpy(key, p, eq - p);
        key[eq - p] = '\0';
        char* end;
        long v = std::strtol(eq + 1, &end, 10);
        Add(key, static_cast<int>(v));
        ++added;
        p = *end == ';' ? end + 1 : end;
    }
    return added;
}

// --8<-- [start:find]
const Entry* Catalog::Find(const char* key) const {
    for (const auto& e : entries_) {
        if (std::strcmp(e->key, key) == 0) return e.get();
    }
    return nullptr;
}

// --8<-- [end:find]

bool Catalog::TryGet(const char* key, int* out) const {
    const Entry* e = Find(key);
    if (!e) return false;
    *out = e->value;
    return true;
}

int Catalog::Count() const { return static_cast<int>(entries_.size()); }

void Catalog::Clear() { entries_.clear(); }
