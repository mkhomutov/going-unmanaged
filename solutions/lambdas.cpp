// Lambda lifetime lab - fixed patterns.
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// A "button" storing a callback - the classic escape route for lambdas.
struct Button {
    std::function<void()> onClick;
    void Click() const { if (onClick) onClick(); }
};

std::function<int()> MakeCounter() {
    int count = 0;
    // BROKEN: return [&count]{ return ++count; };   // dangling: count dies
    return [count]() mutable { return ++count; };    // FIXED: own a copy
}   // 'mutable' because captures are const by default inside the lambda

int main() {
    Button b;
    {
        auto label = std::make_shared<std::string>("Save");
        // BROKEN: b.onClick = [&label]{ ... };      // label dies with scope
        b.onClick = [label] { std::cout << "clicked: " << *label << "\n"; };
    }                       // shared_ptr copy keeps the string alive - by design
    b.Click();

    auto counter = MakeCounter();
    std::cout << counter() << counter() << counter() << "\n";   // 123

    // capture-by-move for expensive/unique things (C++14 init-capture):
    auto big = std::make_unique<std::vector<int>>(1000, 7);
    auto owner = [v = std::move(big)] { return v->size(); };
    std::cout << "owned size=" << owner() << "\n";
    return 0;
}
