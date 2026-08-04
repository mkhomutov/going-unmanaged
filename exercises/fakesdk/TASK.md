# Exercise: The FakeSDK (desktop-plugin style)

Full statement, reference solution, and pitfall analysis: **Chapter 17** of the book.
Do it COLD first — solution afterwards.

Vendor code: `FakeSDK.h` / `FakeSDK.cpp` — read, compile, link, never edit.

Write: `ThingDataGuard` (RAII) + `SumAllThings(double* total, size_t* skippedCount)`.
Prove with three scenarios (hand-computed predictions as comments), assert VALUES,
and `FakeSdk_LiveAllocations()` must be 0 after every scenario.

Build: g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -g FakeSDK.cpp your.cpp -o task
  (or: ../../scripts/check.sh your.cpp fakesdk — from this directory)
  (Windows: ..\..\scripts\check.ps1 your.cpp fakesdk — from a Developer PowerShell)
