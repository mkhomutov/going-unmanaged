#pragma once
#include <cstddef>

class Logger {
public:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void write(const char* line);
    std::size_t lines() const;

private:
    char* buffer_;          // the session log, one heap block
    std::size_t used_ = 0;
    std::size_t lines_ = 0;
};

Logger& TheLogger();    // construct on first use - the fix
