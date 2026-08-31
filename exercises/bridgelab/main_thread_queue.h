// main_thread_queue.h - the one piece every bridge shares, whatever the
// transport: foreign code never calls the SDK, it posts a job here.
//
// Quoted IN FULL in Chapter 38 ("The queue"). Changing it means updating
// that listing in the same commit - the same discipline the Fake* vendor
// code is held to. Write your own first: the lab's task card asks you to
// build it from the chapter's constraints, and comparing afterwards is
// the point.
#pragma once
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <type_traits>
#include <utility>

class MainThreadQueue {
public:
    // Any thread. The job runs later, on the main thread; the future is how
    // the caller waits for the answer. The whole contract is that a job
    // accepted here is eventually RUN: policy (host busy, no document) lives
    // inside the job, which can answer no - never in the queue, because a
    // job dropped here is a client blocked on a future nothing will set.
    template <class F>
    auto Post(F fn) -> std::future<std::invoke_result_t<F>> {
        using R = std::invoke_result_t<F>;
        auto task = std::make_shared<std::packaged_task<R()>>(std::move(fn));
        std::future<R> fut = task->get_future();
        {
            std::lock_guard<std::mutex> lock(m_);
            jobs_.push([task] { (*task)(); });
        }
        return fut;
    }

    // Main thread only, at a point the host calls safe. Runs everything
    // queued so far, in arrival order, and returns how many jobs ran. A job
    // that pumps the event loop and lands here again nests to zero - the
    // jobs posted meanwhile keep their turn, on the next outer Drain.
    std::size_t Drain() {
        if (draining_) return 0;      // refuse to nest: no job runs inside a job
        draining_ = true;
        std::queue<std::function<void()>> batch;
        {
            std::lock_guard<std::mutex> lock(m_);
            batch.swap(jobs_);
        }
        const std::size_t ran = batch.size();
        while (!batch.empty()) {
            batch.front()();          // a throw stays inside the future:
            batch.pop();              // packaged_task catches it for the caller
        }
        draining_ = false;
        return ran;
    }

private:
    std::mutex m_;
    std::queue<std::function<void()>> jobs_;
    bool draining_ = false;           // main thread only - it needs no lock
};
