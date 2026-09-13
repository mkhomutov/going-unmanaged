## Chapter 44 — Crash at Document Close

The plug-in's panel is built on the host's own C++ framework, and a clean-up pass put the ownership rules this book has spent twenty chapters turning into a reflex onto every object in it. Since then the host crashes when a document closes — on customers' machines, and not on the bench. The framework's programming guide arrives attached, the way it does. The diagnosis is an accounting exercise before it is a code change, and part of working it cold is discovering that the rule you are about to apply is the one that broke it.

### The ticket

> **Crash at document close (since 3.0).** Release 3.0 included an ownership clean-up of the panel code: every `new` paired with a smart pointer, per the review checklist. Since then the host crashes when the user closes a document while the panel is showing. Support's bench cannot reproduce it: their script unloads the plug-in and *then* closes the document, and the developer who did the clean-up says the two orders "should be the same". The framework's programming guide, chapter 2, is attached.

Two facts to hold. The clean-up did what every chapter before this one told it to. And the bench is green in one order and the customer crashes in the other, which means the order is the evidence — [Chapter 32](32-it-crashes-on-exit.md#chapter-32--crash-on-exit)'s lesson, arriving from a different direction.

### The framework's contract

Chapters 17 and 18 handed you C: status codes, opaque handles, a `void*` context. This vendor ships C++ — classes, a virtual destructor, a tree — and with it an object model of its own:

```cpp
--8<-- "exercises/framelab/FakeUi.h"
```

This is Bestiary Shape 5 with the serial numbers filed off: a C++-native framework with its own lifetime rules — a node belongs to its parent, the document belongs to the host, and the framework ships its own weak handle for everything you do not own — the exact posture of a UI toolkit's parent–child ownership or a game engine's collector, stated in four sentences of a guide. Note what the framework does *not* do: nothing in it checks whether you also thought you owned a node. Like the real thing, it deletes what it owns when it decides to, and believes you about the rest.

### The code it happened to

The panel as 3.0 shipped it, with the host's two orders in the `main` below it. Two comments record the porter's reasoning — read them as evidence, because every word of them is true in every other chapter of this book:

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

It compiles clean. In the bench's order — an argument on the command line — it runs clean too, plain or under the full canonical flags, and its counter goes to zero the way a correct program's would:

```text
panel up: 5 live nodes
panel down: 1 live nodes
still live at unload: 0
```

In the customer's order the plain build dies with a segmentation fault before it prints its second line, and the sanitized build says why:

```text
==88106==ERROR: AddressSanitizer: heap-use-after-free on address 0x604000000490 ...
READ of size 8 at 0x604000000490 thread T0
    #0 in std::default_delete<Node>::operator()(Node*) const unique_ptr.h:77
    #2 in std::unique_ptr<Node>::~unique_ptr() unique_ptr.h:259
    #4 in Panel::~Panel() main.cpp:7
    #6 in main main.cpp:38

freed by thread T0 here:
    #0 in _ZdlPv (libclang_rt.asan_osx_dynamic.dylib)
    #1 in Node::~Node() FakeUi.cpp:56
    #2 in Node::~Node() FakeUi.cpp:63
    ...
    #8 in FakeUi_CloseDocument() FakeUi.cpp:128
    #9 in main main.cpp:36

previously allocated by thread T0 here:
    #0 in _Znwm (libclang_rt.asan_osx_dynamic.dylib)
    #1 in std::make_unique<Node, Node*, char const (&) [8], 0>(Node*&&, char const (&) [8]) unique_ptr.h:759
    #2 in Panel::ShowTooltip() main.cpp:15
    #3 in main main.cpp:29

SUMMARY: AddressSanitizer: heap-use-after-free unique_ptr.h:77 in std::default_delete<Node>::operator()(Node*) const
```

Hold the bench's middle line — `panel down: 1 live nodes` — because it is the piece of evidence that looks like a success and is the whole case.

### Try it — before reading on

The ticket card is `exercises/framelab/TASK.md`, with the vendor files beside it (`FakeUi.h`/`.cpp` — read, compile, link, never edit; `panel.h` and `main.cpp` are the fixed reference — no peeking). This ticket is solved by accounting:

1. **Read the guide twice**, then run a **ledger** over the panel on paper: for every node — frame, title, meter, tooltip — who constructed it, who the guide says deletes it, and who the 3.0 code *thinks* deletes it. Count the owners per node before running anything.
2. **Reproduce, in both orders.** Explain from your ledger why one order reaches zero and the other does not — and what `panel down: 1 live nodes` proves about who freed what.
3. **Read the report against Chapters 33 and 35.** Whose code is on the *freed by* stack, whose is on the access stack, and what does the allocation site say that the other two do not? Which owner paid second, and why does the order decide?
4. **The obvious patch** — drop the smart pointers and keep raw pointers to nodes the framework owns. Predict what changes, run both orders, then add one line that reads a node's name *after* the close, and one tooltip that is never shown. Say what the patch traded the crash for.
5. **The real fix: hold what you do not own through the framework's own handle**, own a node with `unique_ptr` exactly until you hand it to a parent, release it on that line, and delete a parented node by hand only where the guide says you may. Acceptance is **both judges, in both orders**: the live-node counter at zero after the close *and* after the panel is gone, whichever comes first, and the sanitizers quiet.
6. **Stretch: the other convention.** Some frameworks in this shape do the opposite — attaching a child does *not* transfer ownership. Rewrite the guide's four sentences that way in a copy and see which lines of your fix change, and which of 3.0's bugs would have been correct code.

### The diagnosis, walked through

<details>
<summary>Show the walkthrough — total your own ledger first</summary>

The ledger, from the guide and nothing else:

| Node | Constructed with | The guide says it is deleted by | 3.0 thinks it is deleted by | Owners |
|---|---|---|---|---|
| frame | the document root as parent | the root, when the document closes | `frame_`'s `unique_ptr` | 2 |
| title, meter | the frame as parent | the frame, when the frame goes | their `unique_ptr`s | 2 each |
| tooltip | the frame as parent, when shown | the frame | `tooltip_` | 2 |

Every node has two owners, and the framework's guide said so in its first sentence: *a node constructed with a parent is owned by that parent.* The `unique_ptr`s are not wrong about *how* to own; they are wrong about *whether* — the ownership they encode was already taken.

Now the order. In the bench's order `~Panel` runs first, and its four `unique_ptr`s delete four nodes; each `~Node` detaches itself from its parent on the way out, so by the time the frame is deleted it has no children left and the root has no frame. `panel down: 1 live nodes` — the root — and the close deletes it. Zero. The second owner *never arrived*: the first one cleaned up so thoroughly that the framework had nothing left to own, and the bench recorded that as correct. It is [Chapter 35](35-still-live-at-unload.md#chapter-35--objects-still-live-at-unload)'s cancellation with the sign flipped — two owners, and in this order the one that pays first leaves nothing for the other to double-pay.

In the customer's order the close runs first. The framework deletes the root, the root deletes the frame, the frame deletes the title, the meter and the tooltip — the `freed by` stack is exactly that recursion, `~Node` calling `~Node`, ending in `FakeUi_CloseDocument`: the vendor's own teardown. Then `~Panel` runs and the first `unique_ptr` deletes a node that no longer exists. Read the access stack: the report is a *read*, inside `default_delete`, before the double-free ASan would otherwise have named — because `~Node` is virtual and `delete` has to read the vtable pointer out of freed memory to find it. Your destructor is on that stack, the framework's on the other, and the allocation site names the tooltip's `make_unique`. Three stacks, three parties, one object: that is what two owners looks like in a report. As in Chapter 35 the crime is an *event* — the second delete — so the culprit line is on the stack; unlike Chapter 35's leak, there is no half of this ticket a report cannot see, which is why the bench's order, where nothing happens twice, produces no report at all.

The obvious patch is the one the checklist forbids: raw pointers. Drop the `unique_ptr`s and the crash is gone in both orders, and two new things are true. A node's name read after the close is a use-after-free through a pointer the framework already invalidated — the report now points at your accessor, with the same `freed by` stack. And a tooltip that was created unparented and never shown is owned by nobody, and the counter says so: `still live at unload: 1`. The patch traded a double owner for no owner, and the two ledgers fail in opposite directions.

</details>

### What the contract actually says

A name for the guide: **parent ownership** — the framework's object model is a tree, the tree owns its nodes, and a node's lifetime is decided by reachability from the root, when the *host* decides, not when your destructor happens to run. The rule that governs every API of this shape is the Bestiary's checklist with the first two questions answered before you ask them: *who allocates* is you, with `new`, and *who releases* is the parent, in its own destructor, with a `delete` you never write. The framework also answers a third question the C SDKs never posed, because it ships an observer of its own — `NodeRef`, a handle that reads null once the node it watched is gone. That is not a convenience. It is the framework telling you what kind of pointer you are allowed to hold to what it owns.

> [!NOTE]
> **Surprise for C# devs:** this is the object model you came from, and that is the trap. A WinForms control's `Controls` collection owned its children, a child was disposed when its parent was, and you never once called `Dispose` on a `Button` you had added to a `Form` — the framework did, at the moment it chose. Every chapter since Chapter 1 has been teaching you to replace that instinct with a single explicit owner, and here the instinct was right all along and the replacement is the bug. A C++-native framework has decided to be your runtime for the objects it manages; inside its world, the rule from [Chapter 1](01-ownership-and-raii.md#chapter-1--ownership-and-raii) still holds — *exactly one owner* — and the framework already is it.

The reflex the ticket confronts is therefore not a wrong reflex but a misapplied one. `unique_ptr` is the right spelling of ownership for everything in this book up to this page; here the question that comes *before* choosing a spelling is whether ownership is yours to spell at all. That question has a two-word answer for every object in a Shape 5 framework — *theirs* or *mine* — and the guide states it per kind of object, in sentences this short. The habit this chapter leaves is the question: **before you wrap it, ask what already owns it.**

### The fix

Patching the four members would be 3.0.1 with more steps: the next panel retakes the same exam, and so does every container of nodes anyone adds later. The fix that closes the ticket is to hold what the framework owns *through the framework's own handle*, own a node only until the moment it is handed over, and let go on that line:

```cpp
--8<-- "exercises/framelab/panel.h"
```

Read it against the ledger. The three parented nodes are `NodeRef`s: the panel watches them and owns nothing, and after the close each reads null instead of dangling. The tooltip is the one node the panel ever owns, and it owns it for exactly two lines — constructed unparented (*yours*, says the guide), handed to the frame (*the parent's*, from that line on) and released, because a `unique_ptr` that outlives the hand-over is the whole ticket in one member. Hiding it early goes through the handle, which is the framework's documented way to remove a node ahead of its parent and is a no-op when the document has already taken it. Nothing in the destructor, and that is the point: there is nothing the panel owns at the moment it dies.

The judge, in full, because both orders *are* the acceptance test:

```cpp
--8<-- "exercises/framelab/main.cpp"
```

Two judges, one per direction, exactly as Chapter 35: the framework's live-node counter catches an owner too few — the tooltip nobody freed, which no sanitizer on this platform would name — and the sanitizers catch an owner too many. And two orders, because a claim about ownership is a claim about what happens *whichever* owner goes first, and one order cannot prove it; that is [Chapter 32](32-it-crashes-on-exit.md#chapter-32--crash-on-exit)'s two link orders again, with a framework where the linker was. `build_all.sh` runs exactly that on every push.

### Pitfalls

- **`unique_ptr` on a node with a parent.** The one line that is this whole ticket. A parented node has an owner; a second one pays second, in whichever order the host chooses, and the bench will choose the order that hides it.
- **The raw-pointer "fix".** Trading a double owner for no owner: a dangling pointer after the close, and a leak for the node that never reached a parent. The framework ships a weak handle precisely so that this is never the alternative.
- **`release()` forgotten at the hand-over.** Owning a node with `unique_ptr` *until* it is parented is right, and it is right only if the hand-over line releases. A `unique_ptr` member "for the tooltip" that survives `SetParent` is the 3.0 bug with a delay on it.
- **Deleting by hand what the guide does not let you.** This framework permits removing a child early with `delete`, and says so; others in the same shape forbid `delete` on a managed object entirely and give you a `Destroy` or a deferred-deletion call instead. The four sentences differ per framework — it is the reason the guide is read twice — and the question is the same every time.
- **A container of them.** `std::vector<std::unique_ptr<Node>>` is this ticket multiplied by the container's size, and it looks even more like the checklist's idea of correct.
- **Testing one order.** A test that unloads the plug-in before it closes the document proves nothing about the customer who closes the document first. Ownership claims need both orders, the way Chapter 32's needed both link orders.

> [!TIP]
> **Key principle:** "Before I wrap an object in a smart pointer, I ask what already owns it. When a framework does, I hold its own handle instead, own a node only until I hand it over, and release on that line — inside their world, their rules, and mine at the boundary."

### In the wild

Shape 5 is built from exactly the pieces this ticket assembled, and the four sentences come out differently per vendor, which is the practical lesson. Qt is this lab almost line for line: a `QObject` with a parent is deleted by the parent, the documentation's own advice for a widget you create before adding it to a layout is a `std::unique_ptr` you `release()` when the layout takes it, `QPointer` is `NodeRef`, and `deleteLater` is the early removal with the framework choosing the moment. Unreal is the same shape with a collector where the parent was: a `UObject` lives while something marked `UPROPERTY()` references it, `TWeakObjectPtr` is the handle you hold to what you do not own, a raw pointer the collector cannot see dangles after the next collection exactly as the raw-pointer patch did after the close, and `delete` on a managed object is not merely wrong but forbidden — the vendor's four sentences say *never*. JUCE, in the same ecosystem as Chapter 36's audio plug-in, chose the *opposite* convention: adding a child component does **not** transfer ownership, the parent holds observers, and a `std::unique_ptr` member is the norm — so the 3.0 panel would have been correct code there, and the fix above would leak. Three frameworks, one question, three answers; the chapter's habit is the question, and the guide is where the answer lives. Find its four sentences first, before the first `new`.
