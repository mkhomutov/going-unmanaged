# Crash at Document Close — ticket card (Chapter 44)

This lab is a **ticket, not a task**. The plug-in's panel is built on the
host's C++ framework — a framework with an object model of its own — and
a clean-up pass put the ownership rules this book teaches onto every
object in it. Chapter 44 states everything in full and walks the
diagnosis behind a spoiler fold — work the ticket cold first, and work it
with a **ledger**: this ticket is solved by asking, for every node, who
the framework's guide says deletes it and who the code thinks does.

> **Crash at document close (since 3.0).** Release 3.0 included an
> ownership clean-up of the panel code: every `new` paired with a smart
> pointer, per the review checklist. Since then the host crashes when the
> user closes a document while the panel is showing. Support's bench
> cannot reproduce it: their script unloads the plug-in and *then* closes
> the document, and the developer who did the clean-up says the two orders
> "should be the same". The framework's programming guide, chapter 2, is
> the comment block at the top of `FakeUi.h`, beside this card.

**Vendor code:** `FakeUi.h` and `FakeUi.cpp` beside this card are the
framework — read them, compile them, link them, **never edit them** (the
same rule as `fakesdk/`, `fakedevice/` and `comlab/`). Nothing in the
framework checks whether you also thought you owned a node; like the real
thing, it deletes what it owns when it decides to.

**The other files beside this card are the FIXED reference** — `panel.h`
(the panel Chapter 44 builds) and `main.cpp` (the judge, which runs both
orders), kept green by `build_all.sh` on every push. Do not start from
them. Recreate the broken panel below in a scratch directory of your own.

## The code as 3.0 shipped it

`main.cpp`, one file; the panel and the host's two orders:

```cpp
#include "FakeUi.h"
#include <cstdio>
#include <memory>

// The panel as the 3.0 clean-up shipped it: "no raw owning pointers",
// applied to every node exactly as Chapter 1 teaches.
class Panel {
public:
    explicit Panel(Node* root)
        : frame_(std::make_unique<Node>(root, "panel")),
          title_(std::make_unique<Node>(frame_.get(), "title")),
          meter_(std::make_unique<Node>(frame_.get(), "meter")) {}

    void ShowTooltip() {
        tooltip_ = std::make_unique<Node>(frame_.get(), "tooltip");
    }
    void HideTooltip() { tooltip_.reset(); }   // one owner, one reset: the checklist's own example

private:
    std::unique_ptr<Node> frame_;              // every node paired with its owner, as reviewed
    std::unique_ptr<Node> title_;
    std::unique_ptr<Node> meter_;
    std::unique_ptr<Node> tooltip_;
};

int main(int argc, char**) {
    Node* root = FakeUi_OpenDocument();
    Panel* panel = new Panel(root);
    panel->ShowTooltip();
    std::printf("panel up: %zu live nodes\n", FakeUi_LiveNodes());
    if (argc > 1) {                           // the bench: unload the plug-in, then close
        delete panel;
        std::printf("panel down: %zu live nodes\n", FakeUi_LiveNodes());
        FakeUi_CloseDocument();
    } else {                                  // the customer: close the document, then unload
        FakeUi_CloseDocument();
        std::printf("document closed: %zu live nodes\n", FakeUi_LiveNodes());
        delete panel;
    }
    std::printf("still live at unload: %zu\n", FakeUi_LiveNodes());
    return 0;
}
```

## Work the ticket

1. **Read the guide twice.** It is the comment block at the top of
   `FakeUi.h`, and it is four sentences long where it matters. Then run a
   **ledger** over the panel on paper: for every node — frame, title,
   meter, tooltip — who constructed it, who the guide says deletes it, and
   who the 3.0 code *thinks* deletes it. Count the owners per node.
2. **Reproduce, in both orders.** Build the 3.0 panel plain and under the
   handbook's flags (`scripts/check.sh main.cpp framelab bench`, then
   without the argument), and run it with an argument (the bench's order) and
   without (the customer's). Explain, from your ledger, why one order is
   clean down to zero live nodes and the other is not — and what the bench
   order's `panel down: 1 live nodes` line proves about who freed what.
3. **Read the report against Chapters 33 and 35.** Whose code is on the
   *freed by* stack, whose is on the access stack, and what does the
   allocation site tell you that the other two do not? Then say which of
   the two owners paid second, and why the answer depends on the order.
4. **The obvious patch** — drop the smart pointers and keep raw pointers to
   nodes the framework owns. Predict what changes, run both orders, then
   add one line that reads a node's name *after* the document closes — and
   construct the tooltip unparented in the constructor, parenting it only
   in `ShowTooltip`, which this run never calls. Run it again, and read
   the counter. Say what the patch traded the crash for.
5. **The real fix: hold what you do not own through the framework's own
   handle.** `NodeRef` reads null once the node is gone — that is what it
   is for. Own a node with `unique_ptr` exactly until you hand it to a
   parent, and release it on that line; delete a parented node by hand
   only where the guide says you may, and only through a handle that
   tells you whether it is still there. Acceptance is **both judges, in
   both orders**: the framework's live-node counter at zero after the
   close *and* after the panel is gone, whichever comes first, and the
   sanitizers quiet.
6. **Stretch: the other convention.** Some frameworks in this shape do the
   opposite — attaching a child does *not* transfer ownership, and the
   parent holds observers while you hold the `unique_ptr`. Rewrite the
   guide's four sentences that way in a copy of `FakeUi.h` and see which
   lines of your fixed panel change, and which of the 3.0 panel's bugs
   would have been correct code under it. That is the reason the guide is
   read twice: the question is the same for every framework, and the
   answer is not.
