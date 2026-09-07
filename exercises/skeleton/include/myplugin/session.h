// The public header: the ABI surface of this project (Chapter 30), included
// as <myplugin/session.h> so it cannot collide with a vendor's session.h.
// The namespace mirrors the directory (Chapter 26).
#pragma once

#include <cstddef>

namespace myplugin {

// One measurement session: readings in, a mean out. Small on purpose - it
// exists so the tree has a library, a test and a consumer of the header.
class Session {
public:
    void Ingest(double reading);
    double Mean() const;
    std::size_t Count() const { return count_; }

private:
    double sum_ = 0.0;
    std::size_t count_ = 0;
};

}  // namespace myplugin
