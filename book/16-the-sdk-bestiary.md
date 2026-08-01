## Chapter 16 — The SDK Bestiary

Native SDKs come in a small number of recurring shapes. Learn to recognize the shape and you already know half the SDK before opening its docs — the wrapping strategy, the failure modes, and where the RAII goes. This chapter is the field guide; Chapters 17 and 18 are hands-on training on the two most common shapes.

### Shape 1 — Error codes + out-parameters + owned payloads (desktop-application plug-in SDKs)

Every function returns a status code; results come back through pointers you pass in; some structs carry SDK-allocated payloads with a matching dispose function. This is the classic shape of plug-in APIs for large desktop applications — CAD packages, DAWs, office suites — and of venerable C libraries like **SQLite** and **zlib**, which are worth reading as masterclasses in the style. Your job: check every code, zero-init every struct, guard every payload. **Trained in Chapter 17.**

### Shape 2 — Opaque handles + open/close + callbacks (device and I/O SDKs)

You receive a pointer to a type you cannot see inside (`typedef struct DeviceImpl* DeviceHandle`); a create/open function hands it out, a close/destroy function takes it back, and events arrive through registered C function pointers carrying a `void*` context. This is the shape of essentially every peripheral SDK: **libusb** and **HIDAPI** (USB/HID devices), **PortAudio** and ASIO (audio interfaces), serial-port and camera SDKs, **Vulkan** (`VkDevice`, `VkInstance` — a modern API deliberately built in this classic shape), and printer/scanner vendor SDKs. Your job: a move-only RAII session per handle, and a trampoline bridging the C callback into C++. **Trained in Chapter 18.**

Two hazards specific to this shape, worth knowing before you meet them for real: **callback threading** — real device SDKs often invoke your callback from a driver thread, not yours, so everything the callback touches needs synchronization (the FakeDevice of Chapter 18 calls back synchronously to keep the exercise focused; the chapter notes what changes when it doesn't); and **callback lifetime** — the SDK holds your function pointer and context until you unregister; if the object behind the context dies first, the next event is a use-after-free delivered by the driver.

### Shape 3 — Reference counting (COM and COM-flavored APIs)

Objects expose `AddRef`/`Release` (or retain/release) and you own one reference per acquisition; functions return `HRESULT` status codes; interfaces are queried by ID. This is **COM** — the substrate of huge parts of Windows: Office automation, DirectX, the shell, WinRT underneath its projections — and the retain/release pattern reappears in Core Foundation on Apple platforms. The C++ treatment: never call `Release` by hand; use the ecosystem's RAII smart pointers (`Microsoft::WRL::ComPtr`, `winrt::com_ptr`, `CComPtr`), which are exactly shared_ptr's discipline with a different spelling. If your work touches Windows deeply, this shape deserves dedicated study; recognizing it as "shared_ptr, someone else's implementation" is the starting point.

### Shape 4 — Init/deinit lifecycles and status enums (embedded HALs and middleware)

A global or per-peripheral `X_Init(&config)` / `X_DeInit()` pair, status enums (`HAL_OK`, `HAL_ERROR`, `HAL_BUSY`, `HAL_TIMEOUT`), configuration structs you zero and fill, and callbacks that are actually interrupt handlers. This is the shape of microcontroller vendor HALs (**STM32 HAL**, **ESP-IDF**, Nordic's SDKs) and much industrial middleware (CAN stacks, Modbus libraries). It is Shape 1 wearing work boots: the same discipline applies, with two additions — callbacks may run in interrupt context (minimal work, no allocation, no blocking), and RAII must respect that some resources are singletons whose "ownership" is initialization order.

### Shape 5 — C++-native SDKs (engines and frameworks)

Some SDKs are genuinely C++: **Qt**, **Unreal**, **JUCE**, many game and media engines. Here the vendor ships its own containers, strings, and smart pointers (Chapter 7's "In the wild"), its own object lifetime rules (Qt's parent-child ownership; Unreal's garbage collector for UObjects — yes, a GC in C++), and often its own build layer (moc, UnrealBuildTool). The transition skill: identify which of *their* mechanisms replaces which standard one, use theirs inside their world, and convert at the boundary. Fighting a framework's ownership model with raw standard idioms is a rite of passage best skipped.

### The universal checklist, whatever the shape

For every SDK function you meet, answer four questions before calling it: **Who allocates?** (me, via a struct I fill; or the SDK, via a payload I must release). **Who releases, and with which exact function?** (free/dispose/close/Release are not interchangeable). **What is the failure contract?** (code returned? struct touched or untouched on failure? — Chapter 17's documentation trap). **What threads can this be called on, and what thread calls me back?** Wrap the answers in a guard type, and the rest of your code never thinks about them again. That habit — one small RAII type per SDK resource — is the single highest-leverage practice in native SDK work, and it is what Chapters 17 and 18 drill.

---


<!-- nav:begin -->
[← Chapter 15 — Exercise: The Buffer](15-exercise-the-buffer.md) · [Contents](README.md) · [Chapter 17 — Exercise: The FakeSDK →](17-exercise-the-fakesdk.md)
<!-- nav:end -->
