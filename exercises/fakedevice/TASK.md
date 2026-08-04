# Exercise: The Device SDK (peripheral style)

Full statement, reference solution, and pitfall analysis: **Chapter 18** of the book.
Do it COLD first — solution afterwards.

Vendor code: `FakeDevice.h` / `FakeDevice.cpp` — read, compile, link, never edit.

Write: move-only `DeviceSession` (RAII, exactly-once close) + `OnSample(std::function<void(int)>)`
bridged through the void* trampoline. Prove: samples arrive; callbacks survive a MOVE of the
session (predict what breaks first!); error paths; `FakeDevice_OpenHandles() == 0` at the end.

Build: g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -g FakeDevice.cpp your.cpp -o task
  (or: ../../scripts/check.sh your.cpp fakedevice — from this directory)
  (Windows: ..\..\scripts\check.ps1 your.cpp fakedevice — from a Developer PowerShell)
