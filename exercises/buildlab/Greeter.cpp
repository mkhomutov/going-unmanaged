#include "Greeter.h"
#include <iostream>
#include <utility>

Greeter::Greeter(std::string name) : name_(std::move(name)) {}

void Greeter::Greet() const {
    std::cout << "Hello, " << name_ << "!\n";
}
