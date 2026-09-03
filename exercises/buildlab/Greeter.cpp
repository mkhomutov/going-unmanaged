#include "Greeter.h"
#include <iostream>
#include <utility>

Greeter::Greeter(std::string name) : name_(std::move(name)) {}

void Greeter::Greet() const {
    std::cout << "Hello, " << name_ << "!\n";
#ifdef GREETER_AUDIT
    std::cout << audit_tag_ << " greeted " << name_ << '\n';   // only with -DGREETER_AUDIT=ON
#endif
}
