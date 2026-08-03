#include "logger.h"
#include <cstdio>

namespace {
    constexpr std::size_t kCapacity = 4096;
}

Logger::Logger() : buffer_(new char[kCapacity]) { buffer_[0] = '\0'; }
Logger::~Logger() { delete[] buffer_; }

void Logger::write(const char* line) {
    if (used_ + 256 > kCapacity) {
        return;    // session log full - drop the line, never overflow
    }
    const int n = std::snprintf(buffer_ + used_, kCapacity - used_, "%s\n", line);
    if (n > 0) {
        used_ += static_cast<std::size_t>(n);
        ++lines_;
    }
}

std::size_t Logger::lines() const { return lines_; }

Logger& TheLogger() {
    static Logger logger;    // constructed the first time anyone asks
    return logger;
}
