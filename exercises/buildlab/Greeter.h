// Build-Model Lab starting point (Chapter 23). This trio is designed to be
// broken: build it green first, then apply the chapter's seven breakages one
// at a time and read what each stage of the toolchain says.
#pragma once
#include <string>

class Greeter {
public:
    explicit Greeter(std::string name);
    void Greet() const;

private:
    std::string name_;
#ifdef GREETER_AUDIT
    std::string audit_tag_ = "[audit]";   // a member that exists only under the define:
#endif                                    // the reason the define is PUBLIC (Chapter 26)
};
