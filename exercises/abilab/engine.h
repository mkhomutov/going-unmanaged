// engine.h - consumable by C, C++, and anything with an FFI
//
// Quoted IN FULL in Chapter 30 ("Technique 3 - an extern \"C\" façade").
// Changing it means updating that listing in the same commit.
//
// Compare it with exercises/fakedevice/FakeDevice.h when you have written your
// own: opaque handle, output parameters, an error code on every function, an
// explicit destroy. The chapter's point is that you land on that shape without
// trying, because it is the only one that survives two compilers.
//
// Error codes, which engine.cpp implements and engine_demo.cpp checks:
//   0 = ok
//   1 = a null parameter (a caller bug, and never a crash)
//   2 = something threw inside; the call did nothing
#ifdef __cplusplus
extern "C" {
#endif
typedef struct EngineImpl* EngineHandle;          // opaque: no layout to disagree about
int Engine_Create(int seed, EngineHandle* out);   // 0 = ok
int Engine_Score(EngineHandle h, int* outScore);
int Engine_Destroy(EngineHandle h);
#ifdef __cplusplus
}
#endif
