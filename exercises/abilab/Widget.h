// Widget.h is included IN FULL by Chapter 30 ("Technique 1 - PIMPL") from the
// marker below: edit there and the page follows. Write your own first: the
// chapter's Try it asks you to PIMPL the Chapter 15 Buffer, and comparing
// afterwards is the point.
// --8<-- [start:listing]
// Widget.h - the only thing your users compile against
#pragma once
#include <memory>
#include <string>

class Widget {
public:
    explicit Widget(std::string name);
    ~Widget();                          // declared here, DEFINED in the .cpp
    Widget(Widget&&) noexcept;          // the same rule applies to move ops
    Widget& operator=(Widget&&) noexcept;
    int Score() const;
private:
    struct Impl;                        // declared, never defined here
    std::unique_ptr<Impl> impl_;
};
// --8<-- [end:listing]
