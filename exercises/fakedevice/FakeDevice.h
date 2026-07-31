// ============================================================================
// FakeDevice.h - a miniature peripheral-device SDK in the classic C idiom:
// opaque handles, open/close lifecycle, and callbacks with a void* context.
// This is the shape of libusb, HIDAPI, PortAudio, serial-port and most
// vendor device SDKs. DO NOT MODIFY. Read it, wrap it, obey it.
// ============================================================================
#pragma once
#include <cstddef>

using DevErr = int;
constexpr DevErr DevOk        = 0;
constexpr DevErr DevNullParam = 1;
constexpr DevErr DevNotFound  = 2;   // no device with that name
constexpr DevErr DevClosed    = 3;   // operation on a closed/invalid handle
constexpr DevErr DevBusy      = 4;   // open() on an already-open device

// Opaque handle: you get a pointer to a type you cannot see inside.
// The SDK owns the memory behind it; you own the OBLIGATION to Close it.
struct DeviceImpl;
using DeviceHandle = DeviceImpl*;

// The C callback idiom: a plain function pointer plus a caller-supplied
// context pointer, passed back verbatim on every invocation. This pair is
// how C APIs deliver events into YOUR code - no closures exist in C.
using SampleCallback = void(*)(int sample, void* userContext);

// Open a device by name ("sensor0".."sensor3" exist). On success writes a
// handle you MUST eventually pass to Device_Close exactly once.
DevErr Device_Open(const char* name, DeviceHandle* outHandle);

// Close and invalidate the handle. Safe to call with null (*no-op*).
// Double-close of the same handle is an error your wrapper must prevent.
DevErr Device_Close(DeviceHandle h);

// Register (or clear, with nullptr) the sample callback for this device.
// The context pointer is stored verbatim and handed back on every sample.
DevErr Device_SetCallback(DeviceHandle h, SampleCallback cb, void* userContext);

// Ask the device to deliver its pending samples NOW, synchronously, by
// invoking the registered callback once per sample on THIS thread.
// (Real SDKs often call back from a driver thread - see the chapter notes.)
DevErr Device_Poll(DeviceHandle h);

// Test-support: number of handles currently open. Must be 0 when you finish.
size_t FakeDevice_OpenHandles();
// Test-support: preload N pending samples (values 100, 101, ...) on a device.
DevErr FakeDevice_InjectSamples(DeviceHandle h, size_t n);
