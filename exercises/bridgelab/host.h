// host.h - the seam between the bridge and the host's SDK, plus the
// stand-in that lets the whole bridge run with no host installed.
//
// Quoted IN FULL in Chapter 38 ("The seam"). Changing it means updating
// that listing in the same commit - the same discipline the Fake* vendor
// code is held to. The stub is deliberately NOT named Fake*: FakeSDK and
// FakeDevice are frozen vendor contracts you consume; this one is yours,
// and the task card expects you to extend it.
#pragma once
#include <cassert>
#include <functional>
#include <string>
#include <thread>
#include <vector>

// One result shape for every command: ok plus a payload, or not-ok plus a
// reason the client can act on. The reasons are part of the wire contract -
// "HOST_BUSY" is documented as retryable, so a client shows "host is busy,
// close the dialog", not a spinner.
struct CommandResult {
    bool ok = false;
    std::string text;
};

// What the SDK looks like from the bridge's point of view. One
// implementation per host version translates these calls into vendor SDK
// calls; nothing else in the bridge ever sees a vendor type.
class IHostAdapter {
public:
    virtual ~IHostAdapter() = default;
    virtual bool IsIdle() const = 0;
    virtual std::string SelectionJson() = 0;
    virtual CommandResult RunUndoable(const std::string& label,
                                      const std::function<CommandResult()>& body) = 0;
};

// The stand-in - what FakeSDK and FakeDevice were for their chapters, a
// double you can run under the sanitizers on a machine with nothing
// installed. Every method opens by asserting the one contract no compiler
// checks: that the call arrived on the thread that built the adapter.
class StubHostAdapter : public IHostAdapter {
public:
    StubHostAdapter() : main_id_(std::this_thread::get_id()) {}

    bool IsIdle() const override {
        AssertMainThread();
        return !modal_;
    }
    std::string SelectionJson() override {
        AssertMainThread();
        return R"(["wall-17","door-3"])";
    }
    CommandResult RunUndoable(const std::string& label,
                              const std::function<CommandResult()>& body) override {
        AssertMainThread();
        CommandResult r = body();
        if (r.ok) undo_.push_back(label);   // one command = one undo step
        return r;
    }

    // Harness controls: the stub can do to you what a real host does.
    void SetModal(bool modal) { AssertMainThread(); modal_ = modal; }
    const std::vector<std::string>& UndoSteps() const {
        AssertMainThread();             // "every method" means every method
        return undo_;
    }

private:
    void AssertMainThread() const {
        // The habit this chapter trains: a function that touches the SDK
        // asserts the thread on its first line.
        assert(std::this_thread::get_id() == main_id_);
    }
    std::thread::id main_id_;
    bool modal_ = false;
    std::vector<std::string> undo_;
};
