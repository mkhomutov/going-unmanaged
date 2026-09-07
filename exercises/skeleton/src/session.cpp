// Own header first (Chapter 12): a header that forgot an include fails here,
// where its author is looking, and not in a consumer.
#include "myplugin/session.h"

namespace myplugin {

void Session::Ingest(double reading) {
    sum_ += reading;
    ++count_;
}

double Session::Mean() const {
    return count_ > 0 ? sum_ / static_cast<double>(count_) : 0.0;
}

}  // namespace myplugin
