// bridge_core.h - the command registry over the queue: the bridge's public
// surface, and the part that never changes when the transport does.
//
// Quoted IN FULL in Chapter 38 ("The registry"). Changing it means updating
// that listing in the same commit - the same discipline the Fake* vendor
// code is held to. Construct it on the main thread: it learns the thread id
// it must never wait on from where it is built.
#pragma once
#include <cassert>
#include <functional>
#include <future>
#include <map>
#include <string>
#include <thread>
#include <utility>

#include "host.h"
#include "main_thread_queue.h"

class BridgeCore {
public:
    using Handler =
        std::function<CommandResult(IHostAdapter&, const std::string&)>;

    BridgeCore(IHostAdapter& host, MainThreadQueue& queue)
        : host_(host), queue_(queue), main_id_(std::this_thread::get_id()) {}

    // Main thread, before Start - and never after: the map is written here
    // and read from every transport thread, so the freeze IS the lock.
    void RegisterCommand(std::string name, Handler handler) {
        assert(!started_);
        handlers_[std::move(name)] = std::move(handler);
    }

    // The line between wiring and serving. After it, handlers_ is read-only,
    // and concurrent readers of a map no writer touches are legal (Chapter
    // 29: a race needs a writer) - so Invoke needs no lock at all.
    void Start() { started_ = true; }

    // Any thread. The future completes when the main thread has answered -
    // with the result, or with a refusal like HOST_BUSY; never silently not
    // at all. The caller owns the wait, and bounds it (see the harness).
    std::future<CommandResult> Invoke(const std::string& name,
                                      const std::string& args) {
        assert(started_);             // reading started_ is safe: written once,
                                      // before any transport thread existed
        auto it = handlers_.find(name);
        if (it == handlers_.end())
            return Ready({false, "NO_SUCH_COMMAND: " + name});
        // The job captures by value, deliberately: it outlives this frame
        // whenever the caller gives up waiting, and a job must own what it
        // reads - Chapter 22's lesson, now with a thread on each side.
        const Handler& handler = it->second;
        auto job = [this, handler, args]() -> CommandResult {
            if (!host_.IsIdle())
                return {false, "HOST_BUSY"};    // a refusal is a result
            return handler(host_, args);
        };
        // A caller already on the main thread cannot wait for the main
        // thread. Run the job right here instead: this call IS at the safe
        // point, because the main thread is in it and not mid-Drain.
        if (std::this_thread::get_id() == main_id_)
            return Ready(job());
        return queue_.Post(std::move(job));
    }

private:
    static std::future<CommandResult> Ready(CommandResult r) {
        std::promise<CommandResult> done;
        done.set_value(std::move(r));
        return done.get_future();
    }

    IHostAdapter&    host_;
    MainThreadQueue& queue_;
    std::thread::id  main_id_;
    bool             started_ = false;
    std::map<std::string, Handler> handlers_;
};
