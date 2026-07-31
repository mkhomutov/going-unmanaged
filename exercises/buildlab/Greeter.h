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
};
