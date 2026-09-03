// The export macro: the one symbol that crosses the boundary wears it, and
// everything else in the module stays hidden (CXX_VISIBILITY_PRESET hidden in
// CMakeLists.txt). Two spellings, one meaning: "this is the plug-in's surface".
#pragma once
#if defined(_WIN32)
#define MONITOR_EXPORT __declspec(dllexport)
#else
#define MONITOR_EXPORT __attribute__((visibility("default")))
#endif
