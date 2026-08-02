// Widget.cpp - everything real, invisible to callers
//
// Quoted IN FULL in Chapter 30 ("Technique 1 - PIMPL"), like the header.
// Changing it means updating that listing in the same commit.
//
// Every definition below has to be HERE rather than in the header, and for one
// reason: Impl is complete only in this translation unit. The destructor and
// the move operations are the ones that catch people, because the compiler
// would happily generate them at the call site, where Impl is not.
#include "Widget.h"
struct Widget::Impl { std::string name; int score = 7; };

Widget::Widget(std::string n) : impl_(std::make_unique<Impl>()) { impl_->name = std::move(n); }
Widget::~Widget() = default;                              // HERE Impl is complete
Widget::Widget(Widget&&) noexcept = default;
Widget& Widget::operator=(Widget&&) noexcept = default;
int Widget::Score() const { return impl_->score; }
