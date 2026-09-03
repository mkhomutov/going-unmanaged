## Appendix B — Core Principles (Cheat Sheet)

One line each. If you can say these fluently and back them with code, the concept is yours. Good for a quick re-read before code reviews, design discussions — or any morning.

**Ownership / RAII**

- "Every resource has exactly one clear owner — unique_ptr or stack allocation. I never write raw new/delete."
- "unique_ptr by default; shared_ptr only when I can explain why; weak_ptr breaks cycles — there's no GC to collect them."
- "Acquire in the constructor, release in the destructor, delete the copy operations — that's the RAII wrapper shape."

**Value semantics**

- "C++ is value-semantic by default; I opt into reference semantics explicitly."
- "const auto& in loops by reflex — auto alone copies."
- "Polymorphic objects go behind unique_ptr — storing them by value slices them."

**Virtual dispatch**

- "Non-virtual calls dispatch on the static type, virtual on the dynamic type — via the vtable."
- "Every polymorphic base gets virtual ~Base() = default — deleting through a base pointer otherwise is UB."
- "I mark every override 'override' so the compiler catches signature mismatches."

**Templates**

- "Templates are compile-time code generation — a separate instantiation per type, zero runtime cost."
- "Requirements are implicit pre-C++20; concepts made them explicit like C#'s where clauses."
- "Template definitions live in headers because each translation unit must see the source to instantiate."
- "I'd prefer adding a virtual method over dynamic_cast chains."

**Compilation model**

- "Each .cpp compiles independently as a translation unit; the linker resolves symbols across them."
- "Unresolved external = definition missing at link time; undeclared identifier = declaration missing at compile time."
- "I forward-declare in headers and include in .cpp files to keep build times sane."
- "A header carries declarations, templates and inline bodies; everything else is defined once in a .cpp — and a .cpp includes its own header first, so a header that is not self-sufficient fails where its author is looking."
- "There is no house style until I open the codebase — I read fifty lines before I write one, match what is there, and never spell an identifier with a leading underscore and a capital."

**Build systems**

- "CMake doesn't build my code — it generates the thing that builds my code. Configure, then build: two steps, always."
- "Requirements belong to targets, not to the whole project — PRIVATE for what I need, PUBLIC for what my consumers need too."
- "I list source files, never glob them — a file joining the build should be visible in the diff."

**Dependencies**

- "C++ libraries ship as source because binary compatibility is per-ecosystem and fragile — it depends on the compiler, the standard library, the configuration and the architecture all agreeing — so I build my dependencies with my toolchain and they match by construction."
- "Two versions of one library in a binary is an ODR violation, and it is silent — no resolver, no binding redirect, no diagnostic. One version per binary, decided on purpose."
- "I pin dependencies to a tag or a commit, never a branch — and I write down the version and any local patch next to the vendored code."

**Testing**

- "My test binary runs under Address and UB sanitizers, because the bugs that matter in C++ produce no wrong values — assertions supply the workload, the sanitizer supplies the verdict."
- "Code reachable only from a .cpp with main() cannot be tested — testability is a structural property, so the logic lives in a library and the entry point stays thin."
- "There is no reflection, so there are no runtime mocks — I design the seam first, as an interface or a template parameter, and write the fake myself."

**Concurrency**

- "Every std::thread must be joined or detached before it dies, or the program terminates — I use jthread where C++20 is available, and treat a bare std::thread as a resource needing an owner."
- "A data race in C++ is undefined behavior, not a wrong number — and it can print the right answer every time. I run threaded code under ThreadSanitizer, because there is nothing to assert."
- "A callback that arrives on the SDK's thread can outlive the object it points at — I give it a weak reference and an alive flag, publish the flag, unregister, and never free the context the SDK still holds."

**Authoring a binary boundary**

- "Nothing whose layout my compiler chose may cross a binary boundary — no std::string, no std::vector, no exceptions, in any exported signature."
- "Whoever allocates, frees — so my boundary hands out a Destroy function rather than letting the caller delete."
- "A published vtable is append-only — I add methods at the end or publish a new interface, because inserting one silently rebinds every existing caller to the wrong slot."

**Debugging**

- "The shape of a sanitizer report names the bug class before I read it — three stacks is a use-after-free, one is a leak, and no stack at all with a column number is UBSan."
- "A stack from an optimized build is missing frames — I reproduce at -O0 -g before I believe what a trace does or doesn't say."
- "When I need to know who changed a value, I set a watchpoint — old value, new value, and the frame that did it — instead of adding print statements."
- "A fault address just past null is a member offset — I read it against the struct definition before I read a line of code."
- "A crash report is only as good as the symbol files I archived on release day — I keep the dSYM or PDB for every shipped build, and I symbolicate with inline expansion, because the frame that did it may live inside the frame that is named."

**Static lifetime**

- "A namespace-scope object with a constructor is a bet on link order. I construct on first use, and I touch my dependencies in my constructor — so teardown unwinds in the order I chose, not the order the linker did."

**Wire formats**

- "A struct's layout belongs to my compiler and a frame's belongs to the wire — I never overlay one on the other. I decode at documented offsets with readers that spell the wire's byte order, and the same code is right on every host."

**Deadline code**

- "On a deadline thread I treat the allocator as I/O: the hot path allocates nothing, locks nothing, and blocks on nothing — and I prove it with a counter, because a profiler's mean flatters while a deadline punishes the worst case."

**Choosing signatures and storage**

- "Before I write a signature I ask who owns this, how long it lives, and who may see it — one question, and the four answers are the container, the parameter, the return, and what goes in the collection."
- "The question that picks a parameter's shape is whether it is a polymorphic base, and then whether the function keeps a copy: if it keeps one, take it by value and move; if it only borrows, take it by const& — and count the allocations before I put a sink on a hot path."
- "I return by value and let elision do its job — including whole containers; an out-parameter is a C# habit that costs the caller a declaration and the reader a mystery."
- "A collection holds objects by value until something forces otherwise — slicing, an address that must hold still, or a move too expensive to pay — and when I box the elements I write down which of the three it was."

**Bridging a host**

- "Foreign code never calls the SDK — it posts a job to the main thread's queue and waits, with a deadline, for the answer; and on the main thread itself I run the job inline, because the one thread that drains the queue cannot also wait on it."
- "A bridge completes every job it accepts and bounds every wait — HOST_BUSY is an answer the client can act on, silence is a hang, and a deadline turns a hang into a line number."
- "The vendor's types never appear on my wire — my bridge speaks a domain model it owns, the shim translates, and a host release rebuilds the shim and nothing else."

**Publishing to a managed caller**

- "The managed declaration of my function is written by hand and checked by nothing — so I publish only blittable types, put a size field in front of every struct, and validate it on entry."
- "Nothing I allocate crosses the boundary — the caller hands me a buffer and I tell them how big it needs to be, so which heap frees it is never asked."
- "I write the callback's lifetime window into the header, because it is the only thing that tells a managed author how long to keep their delegate rooted."
**Const-correctness**

- "const describes a path, not an object — so I mark the member functions that do not change observable state, and the read interface appears for free."
- "A const violation is a compile error, never a runtime one — so const-correctness costs nothing at the start of a class and twenty errors in year three, and I pay it at the start."
- "`mutable` is for members that are not part of the value — a cache, a memo, a mutex — and never for anything a caller can observe."

**Modern C++**

- "Capture by copy when a lambda outlives its scope; by reference dangles."
- "Fail-able lookups return optional<T>, not null or sentinels."
- "A closed set of alternatives is a std::variant by value, visited exhaustively; an open set someone else extends is a virtual base behind unique_ptr."
- "string_view for read-only string params — zero copies. But never store one to a temporary."

**STL**

- "vector by default — contiguous memory beats theoretical complexity."
- "map is a tree; unordered_map is the Dictionary equivalent."
- "operator[] on a map inserts on read — I use find (or C++20's contains)."
- "Erasing during iteration: use erase's return value, or C++20's erase_if. And push_back can invalidate everything via reallocation."
- "A pointer into a growable container is a loan that the next reallocation calls in — I store the key and borrow at the point of use, and when my API hands out a pointer, the comment says how long the loan lasts."

**Rule of Five / move semantics**

- "Rule of Zero first: compose from self-managing members and write none of the five. Rule of Five only when holding a raw resource."
- "std::move is just a cast — it grants permission to steal; the move constructor does the stealing."
- "Move = steal the pointer and null out the source, or its destructor double-frees."
- "Move operations get noexcept — otherwise vector copies instead of moving on reallocation."
- "Copy assignment via copy-and-swap: strong exception guarantee, self-assignment safe for free."
- "A named rvalue reference is an lvalue: std::move inside every move operation, never around a returned local, and never on a const."

**OOP mechanics**

- "Members are constructed in the initializer list, in declaration order — const and reference members can only be initialized, never assigned in the body."
- "An interface is an abstract class: all pure virtual, virtual destructor, no data."
- "I keep multiple inheritance to interface-style bases — no data, so no duplicated state; two interfaces sharing an ancestor still make the upcast ambiguous, and needing virtual inheritance to fix that is a design smell."
- "Default inheritance for 'class' is private — I always write ': public Base' explicitly."
- "I mark every single-argument constructor explicit unless I deliberately want the implicit conversion."
- "No universal Object root — no free ToString/Equals; comparison and printing are opt-in."

**Errors, casts, strings, UB**

- "I ask whether a failure is a bug, a value, or an event — a bug gets an assert, a value gets a return the caller must look at, and only a rare non-local failure, or a constructor with no return channel, gets a throw."
- "Whether exceptions exist at all is a property of the build, not a per-function choice — I find out which dialect a codebase speaks before I write my first function in it."
- "Throw by value, catch by const reference — catching by value slices. No finally: RAII is the finally."
- "No exception crosses the add-on boundary — I catch at entry points and translate to error codes. Destructors never throw."
- "A value becomes a throw again only at the top of the program — the one frame with nobody left to hand it to — and never in between."
- "static_cast for conversions I can prove, dynamic_cast to query at runtime; const_cast and reinterpret_cast are code-review question marks."
- "std::string is an encoding-unaware byte buffer — I keep it UTF-8 and convert to/from vendor strings explicitly, naming the encoding."
- "UB means the compiler assumes it never happens — Debug-works-Release-breaks is the signature. I treat warnings as errors, run sanitizers regularly, and reach for AddressSanitizer the moment anything smells like memory corruption."
- "size() is unsigned, so size() - 1 on an empty container is a huge number, not -1 — I compare with < instead of subtracting, because the wrap is legal and my -fsanitize=address,undefined build stays silent about it."
- "Heap use is a deliberate choice in C++ — containers and smart pointers, never bare new."

**C-style SDK specifics**

- "The API is C-flavored: check every error code, zero-init API structs with = {}, pass addresses to be filled in."
- "I wrap every SDK-allocated payload and every opaque handle in an RAII guard so the dispose/close runs on every path."
- "Vendor containers and strings mirror the STL — same concepts and invalidation rules; convert at the boundary, encoding named."
- "A plug-in is a DLL: headers for the compiler, SDK .libs for the linker, the host exports the symbols at runtime."
- "No exception crosses the plug-in boundary, and callbacks registered with the SDK must outlive their registration."
- "In a refcounted API, the function's name tells me whether I own a release — acquired, copied or created means yes; peeked or borrowed means no. I encode each answer once, in a wrapper's named constructors — adopt or share — and after that no line of mine spells Retain or Release."

---


<!-- nav:begin -->
[← Appendix A — Fundamentals Refresher](A-fundamentals-refresher.md) · [Contents](README.md) · [Appendix C — Learning With (and Without) AI Assistants →](C-working-without-ai.md)
<!-- nav:end -->
