## Chapter 22 — Exercise: Lambda Lifetimes

*Trains: Chapter 10 (captures), Chapter 1 (ownership), Chapter 3 (dangling). Time: ~45 min. The C# contrast is the whole point: C# closures keep captured objects alive via the GC; C++ captures do exactly what you wrote, including dangling.*

### The three tasks

1. `MakeCounter()`: return a lambda that returns 1, 2, 3... on successive calls. The naive `[&count]` version compiles and dangles — demonstrate, then fix.
2. A `Button` struct storing a `std::function<void()>` callback that prints a label — where the label is created in a narrower scope than the click. Make it correct *by ownership design*, not by luck.
3. Capture-by-move: a lambda that *owns* a `unique_ptr` (something copies can't do).

### Reference solutions

<details>
<summary><strong>Show the solutions — do the tasks cold first</strong></summary>

```cpp
// Lambda lifetime lab - fixed patterns.
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// A "button" storing a callback - the classic escape route for lambdas.
struct Button {
    std::function<void()> onClick;
    void Click() const { if (onClick) onClick(); }
};

std::function<int()> MakeCounter() {
    int count = 0;
    // BROKEN: return [&count]{ return ++count; };   // dangling: count dies
    return [count]() mutable { return ++count; };    // FIXED: own a copy
}   // 'mutable' because captures are const by default inside the lambda

int main() {
    Button b;
    {
        auto label = std::make_shared<std::string>("Save");
        // BROKEN: b.onClick = [&label]{ ... };      // label dies with scope
        b.onClick = [label] { std::cout << "clicked: " << *label << "\n"; };
    }                       // shared_ptr copy keeps the string alive - by design
    b.Click();

    auto counter = MakeCounter();
    std::cout << counter() << counter() << counter() << "\n";   // 123

    // capture-by-move for expensive/unique things (C++14 init-capture):
    auto big = std::make_unique<std::vector<int>>(1000, 7);
    auto owner = [v = std::move(big)] { return v->size(); };
    std::cout << "owned size=" << owner() << "\n";
    return 0;
}
```

</details>

### What each fix teaches

**Task 1 — `[count]() mutable`.** Two lessons in five characters. Capture by copy gives the lambda its *own* `count` that lives as long as the lambda does — the closure now owns its state, which is what the C# version was secretly doing via the heap. And `mutable`: captured copies are `const` inside the lambda by default; incrementing one requires opting out. C# has no equivalent because its captures are references to heap cells. Run the broken `[&count]` version too: it may even print plausible numbers (stack memory not yet reused) — dangling that *works in the demo* is the most dangerous kind, and ASan with `-fsanitize=address` flags the stack-use-after-return only with `ASAN_OPTIONS=detect_stack_use_after_return=1` — worth noting, since it is off by default on some toolchains.

**Task 2 — shared ownership as the design, not a workaround.** The callback outlives the scope that created the label, so *someone* must own the label beyond that scope. Capturing the `shared_ptr` by copy makes the lambda a co-owner: the string lives exactly as long as anything that needs it — which is the C# object-lifetime model, opted into deliberately and visibly. The broken `[&label]` version is the single most common real-world lambda bug: callbacks, timers, and async handlers capturing locals by reference.

**Task 3 — init-capture (`[v = std::move(big)]`).** Some things cannot be copied into a closure — `unique_ptr` foremost. C++14 init-captures move them in; the lambda becomes the owner, and consequently the lambda itself is now move-only (storing it requires `std::function` alternatives like `std::move_only_function` in C++23, or just `auto`). This is the pattern for handing expensive or unique resources to deferred work.

**The rule to leave with,** verbatim from Chapter 10: capture by reference only when the lambda cannot outlive the scope; by copy or move when it escapes — stored, returned, or run async. "Escapes" is the operative test: `std::function` members, callback registries, and thread launches are all escape routes.

---

