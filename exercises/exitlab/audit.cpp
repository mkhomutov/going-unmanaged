#include "logger.h"

class Auditor {
public:
    Auditor()  { TheLogger().write("audit: session opened"); }    // pins the order
    ~Auditor() { TheLogger().write("audit: session closed"); }
};

Auditor g_auditor;
