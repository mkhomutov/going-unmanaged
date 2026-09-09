// Appendix F, Recipe 51 - tell the compiler what a function promises.
//
// session_status(), describe(), fatal() and retire_session() are included by
// book/F-rosetta-cookbook.md between the recipe-51 markers: edit here and the
// page follows. main() is scaffolding.
//
// An attribute changes no instruction, so a running program cannot check one.
// What each attribute changes is a DIAGNOSTIC, and the judge is therefore
// three builds that must be REFUSED - Appendix I's constlab discipline,
// applied to warnings instead of to const. scripts/build_all.sh compiles this
// file three more times with -Werror and one -D each, and asserts that the
// compiler says no and says why:
//
//   -DATTR_DISCARD_RESULT   ignores a [[nodiscard]] result   -> "nodiscard"
//   -DATTR_NO_NORETURN      removes [[noreturn]] from fatal  -> "return-type"
//   -DATTR_USE_DEPRECATED   calls the [[deprecated]] one     -> "deprecated"
//
// Take the attribute away and the clean build stays clean, which is exactly
// why the refusals are the check: nothing else here would notice.
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>

#ifdef ATTR_NO_NORETURN
#  define MAYBE_NORETURN
#else
#  define MAYBE_NORETURN [[noreturn]]
#endif

enum class Status { Ok, Busy, Failed };

// --8<-- [start:recipe-51]
// Never comes back. The caller needs no `return` after it, and the compiler
// stops asking about the path that falls off the end.
MAYBE_NORETURN void fatal(const char* why) {
    std::fprintf(stderr, "fatal: %s\n", why);
    std::abort();
}

// The result is the whole point of the call: ignoring it is a warning, and a
// build with -Werror refuses it. This is the compiler enforcing the sentence
// Chapter 8 spends a page on - a failure that is a value has to be looked at.
[[nodiscard]] Status session_status(int id) {
    return id > 0 ? Status::Ok : Status::Failed;
}

const char* describe(Status status, [[maybe_unused]] int verbosity) {
    switch (status) {
        case Status::Ok:
            return "ok";
        case Status::Busy:
            [[fallthrough]];          // deliberate, and said out loud
        case Status::Failed:
            return "not available";
    }
    return "unknown";
}

// Where [[noreturn]] earns its keep: a function with nothing to return on the
// failing path, because the caller's contract is broken and there is no
// sensible value (Chapter 8's assert row, in a codebase that ships with
// NDEBUG). There is no `return` after the call, and
// with the attribute the compiler needs none; without it, it warns that
// control reaches the end of a non-void function - which is the whole
// difference an attribute makes, and the reason one of the refusals below
// removes it.
int required_channel(int configured) {
    if (configured > 0) {
        return configured;
    }
    fatal("channel not configured - the caller's contract, broken");
}

// Still compiles, still links, still ships; every call site gets a warning
// naming the replacement. C#'s [Obsolete], with the message in the same place.
[[deprecated("use session_status")]] Status retire_session(int id) {
    return session_status(id);
}
// --8<-- [end:recipe-51]

int main() {
    // The values, which the attributes do not touch - stated so the listing
    // is a program and not five declarations.
    const Status ok = session_status(1);
    const Status bad = session_status(0);
    assert(ok == Status::Ok);
    assert(bad == Status::Failed);
    assert(std::string(describe(ok, 0)) == "ok");
    assert(std::string(describe(bad, 0)) == "not available");
    assert(std::string(describe(Status::Busy, 0)) == "not available");
    assert(required_channel(3) == 3);

#ifdef ATTR_DISCARD_RESULT
    // Must be refused: the result is [[nodiscard]] and nobody looks at it.
    session_status(1);
#endif
#ifdef ATTR_USE_DEPRECATED
    // Must be refused: the function still exists, and saying so is the point.
    (void)retire_session(1);
#endif
    return 0;
}
