// Slicing & polymorphism lab - reference solution (the FIXED version).
#include <iostream>
#include <memory>
#include <vector>

class Shape {
public:
    virtual ~Shape() = default;                    // polymorphic base: MUST
    virtual double Area() const = 0;               // pure virtual = abstract
    virtual const char* Name() const = 0;
};

class Circle final : public Shape {
public:
    explicit Circle(double r) : r_(r) {}
    double Area() const override { return 3.14159265 * r_ * r_; }
    const char* Name() const override { return "Circle"; }
private:
    double r_;
};

class Rect final : public Shape {
public:
    Rect(double w, double h) : w_(w), h_(h) {}
    double Area() const override { return w_ * h_; }
    const char* Name() const override { return "Rect"; }
private:
    double w_, h_;
};

int main() {
    // The polymorphic container: unique_ptr<Base>, never Base by value.
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.push_back(std::make_unique<Circle>(1.0));
    shapes.push_back(std::make_unique<Rect>(2.0, 3.0));

    double total = 0;
    for (const auto& s : shapes) {                 // const auto&: no copy,
        std::cout << s->Name() << " area=" << s->Area() << "\n";
        total += s->Area();                        // virtual dispatch works
    }
    std::cout << "total=" << total << "\n";
    return 0;
}   // unique_ptrs delete through Shape* - virtual ~Shape makes it legal
