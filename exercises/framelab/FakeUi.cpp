// FakeUi.cpp - vendor code. DO NOT MODIFY. The framework owns what it says
// it owns and deletes it when it says it will; nothing in here checks
// whether you also thought you owned it.
#include "FakeUi.h"

namespace {
    Node*       g_root = nullptr;
    std::size_t g_live = 0;
}

// --- NodeRef ----------------------------------------------------------------

NodeRef::NodeRef(Node* node) { Attach(node); }
NodeRef::NodeRef(const NodeRef& other) { Attach(other.node_); }
NodeRef& NodeRef::operator=(const NodeRef& other) {
    if (this != &other) {
        Detach();
        Attach(other.node_);
    }
    return *this;
}
NodeRef::~NodeRef() { Detach(); }

Node* NodeRef::get() const { return node_; }

void NodeRef::Attach(Node* node) {
    node_ = node;
    if (node_ != nullptr) {
        next_ = node_->observers_;            // push onto the node's observer list
        node_->observers_ = this;
    }
}

void NodeRef::Detach() {
    if (node_ == nullptr) return;
    NodeRef** link = &node_->observers_;
    while (*link != nullptr && *link != this) {
        link = &(*link)->next_;
    }
    if (*link == this) {
        *link = next_;
    }
    node_ = nullptr;
    next_ = nullptr;
}

// --- Node -------------------------------------------------------------------

Node::Node(Node* parent, const char* name) : name_(name) {
    ++g_live;
    if (parent != nullptr) {
        parent->AddChild(this);
    }
}

Node::~Node() {
    // Every observer reads null from here on: the framework's promise.
    while (observers_ != nullptr) {
        observers_->Detach();
    }
    // Every child still attached goes with its parent.
    while (first_child_ != nullptr) {
        delete first_child_;                  // ~Node removes it from the list
    }
    if (parent_ != nullptr) {
        parent_->RemoveChild(this);
    }
    --g_live;
}

void Node::SetParent(Node* parent) {
    if (parent == parent_) return;
    if (parent_ != nullptr) {
        parent_->RemoveChild(this);
    }
    if (parent != nullptr) {
        parent->AddChild(this);
    }
}

std::size_t Node::ChildCount() const {
    std::size_t n = 0;
    for (Node* c = first_child_; c != nullptr; c = c->next_sibling_) ++n;
    return n;
}

Node* Node::ChildAt(std::size_t index) const {
    Node* c = first_child_;
    while (c != nullptr && index-- > 0) c = c->next_sibling_;
    return c;
}

void Node::AddChild(Node* child) {
    child->parent_ = this;
    child->next_sibling_ = nullptr;
    if (first_child_ == nullptr) {
        first_child_ = child;
        return;
    }
    Node* last = first_child_;
    while (last->next_sibling_ != nullptr) last = last->next_sibling_;
    last->next_sibling_ = child;              // children keep creation order
}

void Node::RemoveChild(Node* child) {
    Node** link = &first_child_;
    while (*link != nullptr && *link != child) {
        link = &(*link)->next_sibling_;
    }
    if (*link == child) {
        *link = child->next_sibling_;
    }
    child->parent_ = nullptr;
    child->next_sibling_ = nullptr;
}

// --- the document -----------------------------------------------------------

Node* FakeUi_OpenDocument() {
    FakeUi_CloseDocument();
    g_root = new Node(nullptr, "document");
    return g_root;
}

Node* FakeUi_DocumentRoot() { return g_root; }

void FakeUi_CloseDocument() {
    delete g_root;                            // and every node still attached
    g_root = nullptr;
}

std::size_t FakeUi_LiveNodes() { return g_live; }
