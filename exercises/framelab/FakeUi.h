// ============================================================================
// FakeUi.h - a C++-native UI framework with its OWN object model: nodes owned
// by their parent, a document the host opens and closes, and the framework's
// own weak handle. DO NOT MODIFY THIS FILE. Treat it as vendor code: read
// it, build on it, obey it.
//
// THE OWNERSHIP MODEL (the framework's programming guide, chapter 2 - read
// it twice):
//   - A Node constructed with a parent is OWNED BY THAT PARENT. The parent
//     deletes every child still attached when it is itself destroyed. Do
//     not delete a parented node yourself unless you mean to remove it
//     early; never hold a second owner of it.
//   - A Node constructed with no parent is YOURS: nothing else will delete
//     it. Give it a parent with SetParent and it becomes the parent's.
//   - The document root is the framework's. FakeUi_CloseDocument deletes
//     the root and, through it, every node still attached - whenever the
//     HOST decides, not when you do.
//   - NodeRef is the framework's weak handle: it observes a node and reads
//     as null once the node is gone. Use it for any node you do not own.
// ============================================================================
#pragma once
#include <cstddef>

class Node;

// The framework's own smart pointer: a weak observer. Copyable, and never
// an owner - it does not delete, and it cannot keep a node alive.
class NodeRef {
public:
    NodeRef() = default;
    NodeRef(Node* node);                      // observe (nullptr is fine)
    NodeRef(const NodeRef& other);
    NodeRef& operator=(const NodeRef& other);
    ~NodeRef();

    Node* get() const;                        // null once the node is gone
    explicit operator bool() const { return get() != nullptr; }

private:
    friend class Node;
    void Attach(Node* node);
    void Detach();
    Node*    node_ = nullptr;
    NodeRef* next_ = nullptr;                 // the node's list of observers
};

class Node {
public:
    // A node with a parent belongs to it from this line on; a node without
    // one belongs to you.
    explicit Node(Node* parent = nullptr, const char* name = "node");
    virtual ~Node();                          // deletes every child still attached

    Node(const Node&) = delete;               // a node is its place in the tree:
    Node& operator=(const Node&) = delete;    // there is no meaningful copy

    // Reparent. The new parent owns the node from now on; nullptr detaches
    // it, and then it is yours again.
    void SetParent(Node* parent);

    Node*       Parent() const { return parent_; }
    std::size_t ChildCount() const;
    Node*       ChildAt(std::size_t index) const;   // nullptr past the end
    const char* Name() const { return name_; }

private:
    friend class NodeRef;
    void AddChild(Node* child);
    void RemoveChild(Node* child);

    Node*       parent_ = nullptr;
    Node*       first_child_ = nullptr;
    Node*       next_sibling_ = nullptr;
    NodeRef*    observers_ = nullptr;
    const char* name_;
};

// Host-side: the document the plug-in's nodes hang under. Open creates the
// root (the framework's); Close deletes it and every node still attached,
// and the host calls it when the USER closes the document.
Node* FakeUi_OpenDocument();
Node* FakeUi_DocumentRoot();                  // nullptr when no document is open
void  FakeUi_CloseDocument();

// Test-support: how many Node objects are alive right now. The host checks
// this at plug-in unload, after the document is closed - it MUST be zero,
// and it is this framework's leak detector.
std::size_t FakeUi_LiveNodes();
