#pragma once
#include "FakeUi.h"
#include <memory>

// The plug-in's panel, built on the framework's object model rather than
// against it. Every node under the document root is the ROOT'S: the panel
// observes them through the framework's own weak handle and owns nothing it
// did not create unparented. The one node it does own outright - a tooltip
// that is shown, and therefore parented, only on demand - is a unique_ptr
// until the moment the framework takes it, and released at that moment.
class Panel {
public:
    // Builds the panel's nodes under `root`. The root owns them from the
    // line they are constructed on; the members below only watch.
    explicit Panel(Node* root)
        : frame_(new Node(root, "panel")),
          title_(new Node(frame_.get(), "title")),
          meter_(new Node(frame_.get(), "meter")) {}

    ~Panel() = default;    // nothing to delete: the framework owns the tree,
                           // and the tooltip is either the frame's or still ours

    // Shows the tooltip: created unparented (ours), then handed to the
    // frame - and RELEASED, because a node with a parent has exactly one
    // owner and it is not this unique_ptr any more.
    void ShowTooltip() {
        if (tooltip_ || !frame_) return;
        std::unique_ptr<Node> tip = std::make_unique<Node>(nullptr, "tooltip");
        tip->SetParent(frame_.get());        // the frame owns it from here...
        tooltip_ = tip.release();            // ...so the unique_ptr must let go
    }

    // Hides the tooltip early - the one case where deleting a parented node
    // by hand is the framework's documented move - or does nothing if the
    // framework has already taken it with the document.
    void HideTooltip() {
        delete tooltip_.get();               // ~Node detaches it from the frame
    }

    bool  Alive() const { return static_cast<bool>(frame_); }
    Node* Title() const { return title_.get(); }
    Node* Meter() const { return meter_.get(); }
    Node* Tooltip() const { return tooltip_.get(); }

private:
    NodeRef frame_;      // the framework's handle: null once the document closes
    NodeRef title_;
    NodeRef meter_;
    NodeRef tooltip_;
};
