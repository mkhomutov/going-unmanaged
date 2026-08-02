// Tracer v2 - makes object identity, lifetime, and moves maximally visible.
// Build:  clang++ -std=c++17 -Wall -Wextra tracer.cpp -o tracer
//    or:  cl /std:c++17 /W4 /EHsc tracer.cpp

#include <cstdio>      // std::snprintf
#include <iostream>
#include <string>
#include <utility>
#include <vector>

class Tracer {
public:
    explicit Tracer(std::string n)
        : name_(std::move(n)), id_(++counter_)
    {
        ++alive_;
        Log("constructed");
    }

    ~Tracer() {
        --alive_;
        Log("destroyed");
    }

    // ---- copy: duplicates, source untouched -------------------------------
    Tracer(const Tracer& other)
        : name_(other.name_), id_(++counter_)
    {
        ++alive_;
        Log("copy-CONSTRUCTED from " + other.Label());
    }

    Tracer& operator=(const Tracer& other) {
        // std::string::operator= releases our old buffer internally.
        // With a raw resource, WE would have to release it here. (G.4)
        name_ = other.name_;
        Log("copy-ASSIGNED from " + other.Label());
        return *this;
    }

    // ---- move: steals, source becomes a husk ------------------------------
    Tracer(Tracer&& other) noexcept
        : name_(std::move(other.name_)), id_(++counter_)   // steal FIRST (G.1)
    {
        ++alive_;
        Log("move-CONSTRUCTED, gutting " + other.MarkHusk());
    }

    Tracer& operator=(Tracer&& other) noexcept {
        if (this != &other) {                              // self-move guard (G.5)
            name_ = std::move(other.name_);                // steal
            Log("move-ASSIGNED, gutting " + other.MarkHusk());
        }
        return *this;
    }

    // ---- bookkeeping ------------------------------------------------------
    static void Report() {
        std::cout << "---- " << alive_ << " object(s) still alive, "
                  << counter_ << " ever created ----\n";
    }

private:
    // "name#id @address" - id distinguishes objects sharing a name;
    // address proves identity (RVO: 'temp' and 'd' print the SAME address).
    std::string Label() const {
        return name_ + "#" + std::to_string(id_) + " @" + Addr();
    }

    std::string Addr() const {
        char buf[32];
        std::snprintf(buf, sizeof buf, "%p", static_cast<const void*>(this));
        return buf;
    }

    std::string MarkHusk() {
        std::string was = Label();      // capture identity BEFORE overwriting
        name_ = "husk";                 // tracing sugar only - real moved-from
        return was;                     // strings are simply left empty
    }

    void Log(const std::string& what) const {
        std::cout << Label() << "  " << what << '\n';
    }

    std::string name_;
    int id_;
    inline static int counter_ = 0;     // total ever created
    inline static int alive_   = 0;     // live right now (leak detector)
};

// ---------------------------------------------------------------------------

Tracer MakeTracer() {
    Tracer t("temp");
    return t;                           // watch: NO copy, NO move (RVO)
}

void ByValue(Tracer t) { (void)t; }
void ByRef(const Tracer& t) { (void)t; }

int main() {
    std::cout << "--- singles ---\n";
    Tracer a("a");
    Tracer b = a;                       // copy: b is a NEW object (new id)
    Tracer c = std::move(a);            // move: a becomes a husk

    std::cout << "--- calls ---\n";
    ByValue(b);                         // copy in, destroyed at return
    ByRef(b);                           // SILENCE - binds directly, no object
    ByValue(std::move(b));              // move in - b gutted

    std::cout << "--- RVO ---\n";
    Tracer d = MakeTracer();            // one construction; compare the
                                        // address with d's destructor line!
    std::cout << "--- vector ---\n";
    std::vector<Tracer> v;
    v.push_back(Tracer("v1"));          // temp constructed, moved in, temp dies
    v.push_back(Tracer("v2"));          // + REALLOCATION: v1 moves to new block
                                        // (delete noexcept above -> it COPIES)
    Tracer::Report();                   // everything from main still alive

    std::cout << "--- teardown (reverse order per scope) ---\n";
    return 0;
}   // vector first (its elements), then d, c, b, a

// After main, statics persist; a final Report() via atexit would show 0 alive.
