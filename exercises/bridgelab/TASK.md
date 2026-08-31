# Exercise: The Bridge Lab (serve a foreign client from inside the host)

Full statement and the reasoning behind every constraint: **Chapter 38** of
the book, *Try it*. Do it COLD from the constraint list below — the files
here are the chapter's worked bridge core, and they are the comparison
afterwards, not the starting point. ~2 h.

*Trains: Chapter 38 (thread affinity as the one invariant, the main-thread
queue, refusing work, the bounded wait); Chapter 29's queue promise, paid;
Chapter 28's seam, met a fourth time (FakeSDK, FakeDevice, FakeSDK2, and now
a host adapter you write yourself); Chapter 22's capture-lifetime rule, with
a thread on each side of the lambda.*

## The task

Build the transport-agnostic core of a bridge — no sockets, no third-party
anything; threads stand in for the transport:

1. **A main-thread queue.** Any thread may post a job and gets a future;
   only the main thread drains; a job that pumps the queue from inside a
   job must not recurse; and the queue completes every job it accepts —
   policy lives inside the job, never in the queue.
2. **A host adapter.** An interface the handlers see, and a stand-in
   implementation that asserts the calling thread on every method, records
   an undo label per successful mutation, and can go modal.
3. **A command registry.** Handlers registered by name on the main thread
   before serving starts, then frozen; invocation from any thread returns a
   future; a caller already on the main thread runs the job inline instead
   of waiting.
4. **A harness with no unbounded wait in it.** Client threads invoking
   against a draining main thread that goes modal now and then; every wait
   is a `wait_for` with a deadline, and a timeout is a failed check with a
   line number.

Then break it, three ways, and watch each one die differently.

### Break one — the queue that waits politely

Swap your `Drain` for this one:

```cpp
    // Main thread only. Be considerate: while the host is busy, not now -
    // the jobs can wait until it is free again.
    std::size_t Drain(bool host_is_idle) {
        if (!host_is_idle) return 0;
        std::queue<std::function<void()>> batch;
        {
            std::lock_guard<std::mutex> lock(m_);
            batch.swap(jobs_);
        }
        std::size_t ran = 0;
        while (!batch.empty()) {
            batch.front()();
            batch.pop();
            ++ran;
        }
        return ran;
    }
```

Run your harness with a modal phase. Every job posted during it sits in the
queue, the client sits in `wait_for`, and the deadline check fires — that
is what a support ticket reading "the client shows a spinner forever" looks
like from the inside. Then try the other tempting shape: pop the job while
modal without running it, and see what the client gets instead of a hang —
`std::future_error` (`broken_promise`), thrown on a transport thread that
almost certainly has no handler for it (Chapter 29 says what happens next).

### Break two — waiting for the thread you are on

Delete the inline branch, so every caller takes the queue path:

```cpp
    // Any thread: post the job, hand back the future.
    std::future<CommandResult> Invoke(const std::string& name,
                                      const std::string& args) {
        auto it = handlers_.find(name);
        if (it == handlers_.end())
            return Ready({false, "NO_SUCH_COMMAND: " + name});
        const Handler& handler = it->second;
        return queue_.Post([this, handler, args]() -> CommandResult {
            if (!host_.IsIdle()) return {false, "HOST_BUSY"};
            return handler(host_, args);
        });
    }
```

Invoke from the main thread, between drains. The future can only complete
when the main thread drains, and the main thread is inside `wait_for`. With
the deadline: a timeout at a line number. Without it: a frozen host, and
nothing anywhere naming the cause.

### Break three — the registry race

Remove the `assert(!started_)` and wire one more command in late, from a
thread, while clients are invoking:

```cpp
    // The settings palette wires in one more command - after Start, while
    // four transport threads are already inside handlers_.find:
    std::thread configurer([&core] {
        core.RegisterCommand("export_report",
            [](IHostAdapter& h, const std::string&) -> CommandResult {
                return {true, h.SelectionJson()};
            });
    });
```

Build with `SAN=thread` and read the report: a read in `handlers_.find`
on one thread, a previous write inside the map's rebalancing on another.
The canonical flags stay silent on this one — it needs the third sanitizer,
and it is the only one of the three breaks any sanitizer catches.

## The files here

The chapter's worked core, checked in so you have something to compare
against — and kept green by `scripts/build_all.sh` on every push, under the
canonical flags AND again under `-fsanitize=thread`:

| Piece | File |
|---|---|
| The queue | `main_thread_queue.h` |
| The seam + the stand-in host | `host.h` |
| The registry | `bridge_core.h` |
| The harness and its judge | `main.cpp` |

All four are Chapter 38's listings (the harness is excerpted there; the
scaffolding between the excerpts is only here). Editing any of them means
editing Chapter 38 in the same commit. The judge is the bounded wait:
two of the three breaks above are hangs, and a hang cannot fail a script —
it stops it, and CI with it — so no wait in `main.cpp` is unbounded, and a
timeout fails with a line number instead of stopping the world.

## Build

One translation unit, two builds — the breaks are split across sanitizers,
so one build checks half:

```
../../scripts/check.sh main.cpp
SAN=thread ../../scripts/check.sh main.cpp
```

`check.ps1` accepts the same shape on Windows (MSVC has no TSan, and the
script says so — break three needs a platform that has it).

## Done means

- **No unbounded wait anywhere.** Every `wait_for` has a deadline; a hang
  in any broken variant becomes a failed check naming its line.
- **Every job posted is answered.** Modal phases produce `HOST_BUSY`
  results, not silence — refusal is a result.
- **The stand-in's thread assert never fires**, in either build — and you
  can say why the assert is on the adapter rather than in the queue.
- **The registry freezes before the first transport thread starts**, and
  you can connect that to why `Invoke` needs no lock (Chapter 29: a race
  needs a writer).
- **Green under both builds** — the canonical flags and `SAN=thread` —
  because the three breaks are split across them, and each build alone
  would call half of this lab correct.

## Stretch goals

- **A real transport.** Put a loopback socket server (or named pipe) on a
  thread of its own, parsing `name args` lines into `Invoke` calls —
  without touching `bridge_core.h`. That insulation is the design working.
- **A `batch` command.** One request carrying several commands, run inside
  ONE `RunUndoable` scope — a client's twenty edits as a single undo step,
  which is how a host expects a tool to behave.
