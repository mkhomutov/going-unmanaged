// Widget.h - the only thing your users compile against
//
// Quoted IN FULL in Chapter 30 ("Technique 1 - PIMPL"). Changing it means
// updating that listing in the same commit - the same discipline the Fake*
// vendor code is held to. Write your own first: the chapter's Try it asks you
// to PIMPL the Chapter 15 Buffer, and comparing afterwards is the point.
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
