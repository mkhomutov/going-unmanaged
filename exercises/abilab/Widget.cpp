// Widget.cpp is included IN FULL by Chapter 30 ("Technique 1 - PIMPL"), like
// the header, from the marker below: edit there and the page follows.
//
// Every definition below has to be HERE rather than in the header, and for one
// reason: Impl is complete only in this translation unit. The destructor and
// the move operations are the ones that catch people, because the compiler
// would happily generate them at the call site, where Impl is not.
// --8<-- [start:listing]
// Widget.cpp - everything real, invisible to callers
#include "Widget.h"
struct Widget::Impl { std::string name; int score = 7; };

Widget::Widget(std::string n) : impl_(std::make_unique<Impl>()) { impl_->name = std::move(n); }
Widget::~Widget() = default;                              // HERE Impl is complete
Widget::Widget(Widget&&) noexcept = default;
Widget& Widget::operator=(Widget&&) noexcept = default;
int Widget::Score() const { return impl_->score; }
// --8<-- [end:listing]
