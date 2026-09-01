// main.cpp - the bridgelab harness: four transport threads against a main
// thread playing host. Chapter 38 quotes the judge and the deterministic
// phases; the client scaffolding between them is described in prose.
#include <atomic>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

#include "bridge_core.h"

// Failures are counted, never thrown: a transport thread reporting through
// an exception would just terminate (Chapter 29). Atomic, because EXPECT
// runs on the client threads too.
static std::atomic<int> g_failures{0};
#define EXPECT(cond)                                                          \
    do {                                                                      \
        if (!(cond)) {                                                        \
            std::fprintf(stderr, "FAILED %s:%d: %s\n", __FILE__, __LINE__,    \
                         #cond);                                              \
            ++g_failures;                                                     \
        }                                                                     \
    } while (0)

// The judge this lab needs: a bounded wait. Two of the chapter's three
// breaks are hangs, and a hang cannot fail this harness - it stops it, and
// CI with it, until some job-level timeout fires naming nothing. So no wait
// in this file is unbounded: every invoke takes a deadline, and a timeout
// is a failed EXPECT with a line number instead of silence.
static CommandResult InvokeChecked(BridgeCore& core, const std::string& name,
                                   const std::string& args,
                                   std::chrono::milliseconds deadline) {
    std::future<CommandResult> fut = core.Invoke(name, args);
    const std::future_status verdict = fut.wait_for(deadline);
    EXPECT(verdict == std::future_status::ready);
    if (verdict != std::future_status::ready)
        return {false, "DEADLINE_EXCEEDED"};
    return fut.get();
}

int main() {
    StubHostAdapter host;      // built on the main thread: the adapter and
    MainThreadQueue queue;     // the core both learn their thread id here
    BridgeCore core(host, queue);

    core.RegisterCommand("get_selection",
        [](IHostAdapter& h, const std::string&) -> CommandResult {
            return {true, h.SelectionJson()};
        });
    core.RegisterCommand("rename_selected",
        [](IHostAdapter& h, const std::string& args) -> CommandResult {
            return h.RunUndoable("Rename to " + args, [&]() -> CommandResult {
                return {true, "renamed"};   // the SDK mutation would go here
            });
        });
    core.Start();              // the registry freezes before the first thread

    // Phase 1 - four clients, kCalls each, while the main thread drains and
    // goes modal one drain in four. Every call must come back before its
    // deadline, as a result or as HOST_BUSY - and each successful mutation
    // must be exactly one undo step.
    constexpr int kClients = 4;
    constexpr int kCalls   = 200;
    const std::chrono::milliseconds kDeadline(10000);

    std::atomic<int> ok{0}, busy{0}, renames{0};
    std::atomic<int> finished{0};      // how the drain loop knows when to stop
    std::vector<std::thread> clients;
    for (int c = 0; c < kClients; ++c) {
        clients.emplace_back([&core, &ok, &busy, &renames, &finished, kDeadline, c] {
            for (int i = 0; i < kCalls; ++i) {
                const bool mutate = i % 2 == 0;
                const CommandResult r = InvokeChecked(core,
                    mutate ? "rename_selected" : "get_selection",
                    "client-" + std::to_string(c), kDeadline);
                EXPECT(r.ok || r.text == "HOST_BUSY");
                if (r.ok) {
                    ++ok;
                    if (mutate) ++renames;
                } else {
                    ++busy;
                }
            }
            ++finished;                // last statement: every future above resolved
        });
    }

    int spin = 0;
    while (finished < kClients) {
        host.SetModal(++spin % 4 == 0);
        if (queue.Drain() == 0)
            std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
    for (auto& c : clients) c.join();
    host.SetModal(false);

    EXPECT(ok + busy == kClients * kCalls);
    EXPECT(host.UndoSteps().size() == static_cast<std::size_t>(renames));

    // Phase 2 - deterministic, from the main thread itself. The first call
    // is the chapter's second break reproduced against the FIX: it cannot
    // go through the queue - nothing is draining any more - so returning at
    // all proves the inline path. Then a refusal, then an unknown command.
    const CommandResult direct = InvokeChecked(core, "get_selection", "", kDeadline);
    EXPECT(direct.ok);
    host.SetModal(true);
    const CommandResult refused = InvokeChecked(core, "rename_selected", "x", kDeadline);
    EXPECT(!refused.ok);
    EXPECT(refused.text == "HOST_BUSY");
    host.SetModal(false);
    const CommandResult unknown = InvokeChecked(core, "no_such_command", "", kDeadline);
    EXPECT(!unknown.ok);

    // Phase 3 - reentrancy. A job that pumps the queue must nest to zero,
    // and the job posted meanwhile must still run one Drain later - kept,
    // not dropped: the queue completes everything it accepts.
    std::future<int> later;
    std::future<std::size_t> pumping = queue.Post([&queue, &later]() -> std::size_t {
        later = queue.Post([] { return 41; });
        return queue.Drain();          // nested: must refuse and run nothing
    });
    EXPECT(queue.Drain() == 1);        // ran the pumping job only
    EXPECT(pumping.wait_for(kDeadline) == std::future_status::ready);
    EXPECT(pumping.get() == 0);
    EXPECT(later.wait_for(std::chrono::milliseconds(0)) ==
           std::future_status::timeout);
    EXPECT(queue.Drain() == 1);        // the kept job runs now
    EXPECT(later.wait_for(kDeadline) == std::future_status::ready);
    EXPECT(later.get() == 41);

    std::printf("bridgelab: %d calls answered (%d ok, %d busy), "
                "%zu undo steps, every wait under its deadline\n",
                kClients * kCalls, ok.load(), busy.load(),
                host.UndoSteps().size());
    return g_failures == 0 ? 0 : 1;
}
