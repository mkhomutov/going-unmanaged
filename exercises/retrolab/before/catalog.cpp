// catalog.cpp - the 2009 implementation behind before/catalog.h. Included
// whole by Chapter 45 from below this banner: the raw ownership the retrofit
// moves inside, exactly as it shipped.
// --8<-- [start:listing]
#include "catalog.h"
#include <cstdlib>
#include <cstring>

Catalog::Catalog() : entries_(0), count_(0), capacity_(0) {}

Catalog::~Catalog() {
    Clear();
    delete[] entries_;
}

void Catalog::Grow() {
    int newCap = capacity_ == 0 ? 4 : capacity_ * 2;
    Entry** bigger = new Entry*[newCap];
    for (int i = 0; i < count_; ++i) bigger[i] = entries_[i];
    delete[] entries_;
    entries_ = bigger;
    capacity_ = newCap;
}

void Catalog::Add(const char* key, int value) {
    for (int i = 0; i < count_; ++i) {
        if (std::strcmp(entries_[i]->key, key) == 0) {
            entries_[i]->value = value;      // replace in place: the address stays
            return;
        }
    }
    if (count_ == capacity_) Grow();
    Entry* e = new Entry;
    std::strncpy(e->key, key, sizeof e->key - 1);
    e->key[sizeof e->key - 1] = '\0';
    e->value = value;
    entries_[count_++] = e;
}

int Catalog::Parse(const char* text) {
    // Validate first, so a malformed text adds nothing.
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

const Entry* Catalog::Find(const char* key) const {
    for (int i = 0; i < count_; ++i) {
        if (std::strcmp(entries_[i]->key, key) == 0) return entries_[i];
    }
    return 0;
}

bool Catalog::TryGet(const char* key, int* out) const {
    const Entry* e = Find(key);
    if (!e) return false;
    *out = e->value;
    return true;
}

int Catalog::Count() const { return count_; }

void Catalog::Clear() {
    for (int i = 0; i < count_; ++i) delete entries_[i];
    count_ = 0;
}
// --8<-- [end:listing]
