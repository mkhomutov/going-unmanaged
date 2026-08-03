#include "logger.h"
#include <cstdio>

int main() {
    TheLogger().write("main: doing the day's work");
    std::printf("%zu lines logged\n", TheLogger().lines());
    return 0;
}
