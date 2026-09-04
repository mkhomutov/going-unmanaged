#!/usr/bin/env bash
# Check the book's per-platform sanitizer and toolchain claims against the
# machine it runs on.
#
# The book states sanitizer behavior that DIFFERS between the maintainer's
# macOS/arm64 machine and the Linux this repo's CI runs on: exit codes, leak
# detection, and whether a frame carries a column number. A pre-announcement
# audit found several places where one platform's behavior had been written
# down as the rule, and the corrections that followed cite compiler-rt
# documentation for the Linux half because there was no Linux here to run.
#
# This script closes that gap. It runs the demonstrations and asserts what the
# chapters promise, per platform, so a wrong claim fails a build instead of
# waiting for a reader on the other operating system to find it. One section
# is keyed by standard library rather than by OS - Recipe 23's null
# const char* handed to std::string, which libc++ and libstdc++ treat
# differently - because that claim is about the library, and the library is
# whichever one $CXX links, not whichever OS this is.
#
# Note what this does NOT contradict. Deliberately broken programs stay
# book-only "because a green run would mean it stopped working" (ROADMAP,
# Chapter 31's sabotage rule). That rule is about asserting a program SUCCEEDS.
# This script asserts the exact way each one FAILS, which is the same rule
# pointed the other way - and the programs are generated into a temp directory,
# never committed, so solutions/ stays clean and build_all.sh never sees them.
#
#   scripts/check_platform_claims.sh              -> may SKIP if a tool is missing
#   scripts/check_platform_claims.sh --required   -> refuse to skip (CI)
set -euo pipefail
cd "$(dirname "$0")/.."

REQUIRED=0
while [ $# -gt 0 ]; do
    case "$1" in
        --required) REQUIRED=1; shift ;;
        *) echo "check_platform_claims.sh: unknown argument: $1" >&2; exit 2 ;;
    esac
done

# Prefer clang++: almost every claim below is about a compiler-rt runtime, and
# the book's transcripts are clang's. (Sections 5, 6 and 7 are the exceptions
# - 5's first two claims and all of 7's are about the linker, 6's about the
# standard library, and each reports what it observed either way.) Fall back to whatever c++
# is, which is what a reader following the chapters would have typed.
if [ -n "${CXX:-}" ]; then :
elif command -v clang++ >/dev/null 2>&1; then CXX=clang++
else CXX=c++
fi

OS=$(uname -s)
ARCH=$(uname -m)
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT

FAILED=0
STDLIB="stdlib not probed"
pass() { echo "  ok   $1"; }
fail() { echo "  FAIL $1"; FAILED=1; }
skip() {
    if [ "$REQUIRED" = 1 ]; then
        echo "  FAIL $1 (--required was given)"; FAILED=1
    else
        echo "  SKIPPED - $1 (CI runs this for real)"
    fi
}

echo "platform: $OS/$ARCH, compiler: $CXX"

# --- the demonstrations, generated rather than committed ---------------------
cat > "$OUT/double_free.cpp" <<'EOF'
int main() {
    int* p = new int[3];
    delete[] p;
    delete[] p;
    return 0;
}
EOF

cat > "$OUT/leak.cpp" <<'EOF'
int main() {
    int* p = new int[10];
    p[0] = 1;
    return p[0] - 1;      // leaks p; exits 0 on its own
}
EOF

# Recipe 34 / Appendix E: a frame that does not fit the thread's stack, with
# the stack sized by hand (pthread, 512 KB) so the overshoot is the same on
# every platform. Two sizes, because the claim is that AddressSanitizer's
# NAME for the crash is not a function of the overshoot: 600 KB overshoots by
# a little, 1 MB by half a megabyte - past the guard page, and no further,
# because the zeroing write walks every byte below the stack until it faults,
# and a walk of megabytes can scribble over the runtime's own mappings and
# hang the report (a 4 MB frame did, on CI).
for KB in 600 1024; do
cat > "$OUT/frame_$KB.cpp" <<EOF
#include <array>
#include <cstdint>
#include <cstdio>
#include <pthread.h>
struct Big { std::array<std::uint8_t, $KB * 1024> bytes{}; };
void* worker(void*) { Big b; b.bytes[0] = 1; std::printf("ran %d\\n", b.bytes[0]); return nullptr; }
int main() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 512 * 1024);
    pthread_t t;
    pthread_create(&t, &attr, worker, nullptr);
    pthread_join(t, nullptr);
    return 0;
}
EOF
done

cat > "$OUT/race.cpp" <<'EOF'
#include <thread>
int g = 0;
int main() {
    std::thread t([]{ for (int i = 0; i < 100000; ++i) g++; });
    for (int i = 0; i < 100000; ++i) g++;
    t.join();
    return 0;
}
EOF

# Chapter 27's ODR diamond, verbatim in structure: one struct name, two
# definitions, one program. GetTimeout is inline, so it lands in both object
# files as a mergeable symbol and the linker keeps whichever it sees first.
#
# The two headers below are Chapter 27's own listings, byte for byte, banner
# comment and all - check_verbatim.sh holds them to the page, so editing either
# side alone fails the book job. The .cpp that include them are this script's
# own; the chapter names them in its transcript but never lists them.
#
# The filenames are that transcript's (`c++ libpart.o main.o -o demo`), and NOT
# odr_*.cpp - deliberately. Section 5 proves the linker stayed quiet by grepping
# its output for the word "odr", and a linker names the object files in the
# diagnostics it does print, so an odr_*.o here would make any unrelated warning
# read as an ODR report. Do not rename these back.
cat > "$OUT/v1.h" <<'EOF'
// v1.h
struct Config { int timeout; };
inline int GetTimeout(const Config& c) { return c.timeout; }
EOF

cat > "$OUT/v2.h" <<'EOF'
// v2.h
struct Config { int retries; int timeout; };   // the new field went FIRST
inline int GetTimeout(const Config& c) { return c.timeout; }   // byte-identical to v1's
EOF

cat > "$OUT/libpart.cpp" <<'EOF'
#include "v1.h"
#include <cstdio>
void LibReport() {
    Config c;                 // v1: ONE int, four bytes
    c.timeout = 30;
    std::printf("v1 lib  sees: %d\n", GetTimeout(c));
}
EOF

cat > "$OUT/main.cpp" <<'EOF'
#include "v2.h"
#include <cstdio>
void LibReport();
int main() {
    Config c;                 // v2: TWO ints, eight bytes
    c.retries = 999;
    c.timeout = 30;
    std::printf("v2 caller sees: %d\n", GetTimeout(c));
    LibReport();
    return 0;
}
EOF

# Run a program and report its exit code, without set -e killing the script.
run_rc() {
    local prog="$1" logfile="$2"; shift 2
    local rc=0
    "$prog" > "$logfile" 2>&1 || rc=$?
    echo "$rc"
}

# The same, with a deadline: a program that corrupts the sanitizer runtime on
# its way to a fault can hang inside the report (section 8's 4 MB frame did,
# on CI), and a hung check stops the job rather than failing it - Chapter
# 38's rule about waits, applied to this script. 124 on timeout, like
# coreutils' timeout, which macOS does not ship.
run_rc_bounded() {
    local prog="$1" logfile="$2" limit="${3:-60}"
    local rc=0 pid waited=0
    "$prog" > "$logfile" 2>&1 &
    pid=$!
    while kill -0 "$pid" 2>/dev/null && [ "$waited" -lt "$limit" ]; do
        sleep 1
        waited=$((waited + 1))
    done
    if kill -0 "$pid" 2>/dev/null; then
        kill -9 "$pid" 2>/dev/null || true
        wait "$pid" 2>/dev/null || true
        echo 124
        return
    fi
    wait "$pid" || rc=$?
    echo "$rc"
}

# --- 1. AddressSanitizer's exit code ----------------------------------------
# Chapter 28: "on macOS the sanitizer runtime calls abort() after printing; on
# Linux it defaults to _exit(1)". Chapter 31 says the same for UBSan's
# -fno-sanitize-recover. The mechanism is compiler-rt's common flag
# abort_on_error, which defaults to true on Darwin and false elsewhere.
echo "== asan exit code =="
if $CXX -std=c++17 -g -fsanitize=address "$OUT/double_free.cpp" -o "$OUT/double_free" 2>/dev/null; then
    RC=$(run_rc "$OUT/double_free" "$OUT/asan.log")
    case "$OS" in
        Darwin) WANT=134; WHY="sanitizer runtime aborts (SIGABRT)" ;;
        Linux)  WANT=1;   WHY="exits with the runtime's default exitcode" ;;
        *)      WANT="";  WHY="" ;;
    esac
    if [ -z "$WANT" ]; then
        skip "no documented expectation for $OS"
    elif [ "$RC" = "$WANT" ]; then
        pass "exit $RC on $OS - $WHY   [Ch 28, Ch 31]"
    else
        fail "expected exit $WANT on $OS ($WHY), got $RC   [Ch 28, Ch 31]"
    fi
    grep -q "attempting double-free" "$OUT/asan.log" \
        || fail "no 'attempting double-free' in the report   [Ch 28]"
else
    skip "AddressSanitizer cannot build a trivial program with $CXX"
fi

# --- 2. Column numbers in an ASan frame -------------------------------------
# Chapter 31: "macOS symbolizes with atos, which stops at the line, while Linux
# uses llvm-symbolizer, which supplies columns to every sanitizer alike. So on
# Linux the missing *stack* is the tell, not the column."
echo "== asan frame columns =="
if [ ! -s "$OUT/asan.log" ]; then
    skip "no ASan report to read"
elif ! grep -qE 'double_free\.cpp:[0-9]+' "$OUT/asan.log"; then
    skip "ASan produced no symbolized source locations at all"
elif [ "$OS" = Linux ] && [ -z "${ASAN_SYMBOLIZER_PATH:-}" ] \
     && ! command -v llvm-symbolizer >/dev/null 2>&1; then
    # The chapter's claim is specifically about llvm-symbolizer. Without it the
    # runtime falls back to addr2line, which gives no columns - that would be a
    # fact about this machine, not a counter-example to the book.
    skip "llvm-symbolizer not on PATH, so the column claim is untestable here"
else
    if grep -qE 'double_free\.cpp:[0-9]+:[0-9]+' "$OUT/asan.log"; then
        HAS_COL=1
    else
        HAS_COL=0
    fi
    case "$OS" in
        Darwin)
            [ "$HAS_COL" = 0 ] \
                && pass "no columns on $OS - atos stops at the line   [Ch 31]" \
                || fail "expected NO column on $OS, found one   [Ch 31]" ;;
        Linux)
            [ "$HAS_COL" = 1 ] \
                && pass "columns present on $OS via llvm-symbolizer   [Ch 31]" \
                || fail "expected a column on $OS, found none   [Ch 31]" ;;
        *)  skip "no documented expectation for $OS" ;;
    esac
fi

# --- 3. LeakSanitizer ---------------------------------------------------------
# CLAUDE.md's platform NOTE, Chapter 31, and Finding 10: LSan is not supported
# on macOS/arm64, so a leaking program under ASan reports nothing there and a
# clean run says nothing about leaks. Chapter 24's Day 2 depends on this - the
# non-virtual-destructor break is a leak, and "your platform may simply never
# report it".
echo "== leaksanitizer =="
if $CXX -std=c++17 -g -fsanitize=address "$OUT/leak.cpp" -o "$OUT/leak" 2>/dev/null; then
    RC=$(run_rc "$OUT/leak" "$OUT/leak.log")
    if grep -qi "detected memory leaks\|LeakSanitizer" "$OUT/leak.log"; then
        SAW_LEAK=1
    else
        SAW_LEAK=0
    fi
    case "$OS/$ARCH" in
        Darwin/arm64)
            [ "$SAW_LEAK" = 0 ] \
                && pass "no leak report on macOS/arm64, exit $RC - LSan unsupported   [Ch 31, Finding 10]" \
                || fail "macOS/arm64 reported a leak; CLAUDE.md and Ch 31 say it cannot" ;;
        Linux/*)
            [ "$SAW_LEAK" = 1 ] \
                && pass "leak reported on Linux, exit $RC   [Ch 24 Day 2, Ch 31]" \
                || fail "expected a LeakSanitizer report on Linux, got none (exit $RC)" ;;
        *)  skip "the book's claim is scoped to macOS/arm64 and Linux, not $OS/$ARCH" ;;
    esac
else
    skip "AddressSanitizer cannot build the leak program with $CXX"
fi

# --- 4. ThreadSanitizer's exit code ------------------------------------------
# Chapter 29: "On macOS the run then aborts, exit 134; on Linux TSan's defaults
# are halt_on_error=0 and exitcode=66, so the program carries on and exits 66".
# Probe by doing, as build_all.sh does: TSan can be installed and still fail to
# start, because its shadow mapping collides with the host's ASLR entropy.
echo "== tsan exit code =="
printf 'int main() { return 0; }\n' > "$OUT/tsan_probe.cpp"
if $CXX -std=c++17 -fsanitize=thread "$OUT/tsan_probe.cpp" -o "$OUT/tsan_probe" >/dev/null 2>&1 \
   && "$OUT/tsan_probe" >/dev/null 2>&1; then
    $CXX -std=c++17 -g -fsanitize=thread "$OUT/race.cpp" -o "$OUT/race"
    RC=$(run_rc "$OUT/race" "$OUT/tsan.log")
    case "$OS" in
        Darwin) WANT=134; WHY="the run aborts" ;;
        Linux)  WANT=66;  WHY="halt_on_error=0, exitcode=66" ;;
        *)      WANT="";  WHY="" ;;
    esac
    if [ -z "$WANT" ]; then
        skip "no documented expectation for $OS"
    elif [ "$RC" = "$WANT" ]; then
        pass "exit $RC on $OS - $WHY   [Ch 29]"
    else
        fail "expected exit $WANT on $OS ($WHY), got $RC   [Ch 29]"
    fi
    grep -q "ThreadSanitizer: data race" "$OUT/tsan.log" \
        || fail "no 'ThreadSanitizer: data race' in the report   [Ch 29]"
else
    skip "no usable ThreadSanitizer here"
fi

# --- 5. Chapter 27's ODR diamond: link order decides, silently ---------------
# SPOILER, if you are working through Chapter 27. Its "Try it" asks you to
# predict which of the two link orders AddressSanitizer catches, and says that
# working out why the asymmetry falls that way is the whole exercise. What
# follows is that answer, in assertion form. Go and predict first - this file
# will still be here.
#
# Three claims, and the first two are about the LINKER rather than compiler-rt,
# which is why this section prints what it observed either way. Chapter 27:
# "The linker says nothing at all - it exits 0 with no diagnostic"; "Which one
# survives depends on link order"; and, under this handbook's own flags, "the
# sanitizer catches exactly one of them ... The loud direction gets caught; the
# quiet one ships."
#
# Which definition survives is unspecified, not guaranteed - so the assertion
# here is deliberately NOT the chapter's exact numbers (one of them is
# uninitialized garbage that varies run to run). It is the structural claim: the
# two orders DISAGREE, and exactly one of them is caught. Both values compared
# below are initialized ones, so a difference is real and not luck.
echo "== odr link order =="
# One pair of object files; only the order they are handed to the linker
# changes below. -O0 matters, and is pinned on every build in this section
# rather than left to the driver's default: at -O2 the compiler may inline both
# copies and hide the whole thing, which Chapter 27 says in as many words. A
# CXX carrying an optimization flag would otherwise stop the demonstration
# demonstrating, and (c) would report it as the chapter being wrong.
ODR_OK=1
$CXX -std=c++17 -g -O0 -c "$OUT/libpart.cpp" -o "$OUT/libpart.o" 2>/dev/null || ODR_OK=0
$CXX -std=c++17 -g -O0 -c "$OUT/main.cpp"    -o "$OUT/main.o"    2>/dev/null || ODR_OK=0
if [ "$ODR_OK" = 0 ]; then
    skip "cannot compile the ODR demonstration with $CXX"
else
    # (a) both orders link with no diagnostic - ill-formed, no diagnostic required
    LINK_QUIET=1
    $CXX "$OUT/libpart.o" "$OUT/main.o" -o "$OUT/demo_libfirst"  > "$OUT/link_libfirst.log"  2>&1 || LINK_QUIET=0
    $CXX "$OUT/main.o" "$OUT/libpart.o" -o "$OUT/demo_mainfirst" > "$OUT/link_mainfirst.log" 2>&1 || LINK_QUIET=0
    if [ "$LINK_QUIET" = 0 ]; then
        fail "a link order failed to link at all   [Ch 27]"
        # The EXIT trap takes $OUT with it, so the linker's own words reach the
        # log here or nowhere. A failure naming no cause is a failure someone
        # has to reproduce before they can read it.
        for order in libfirst mainfirst; do
            if [ -s "$OUT/link_$order.log" ]; then
                echo "       ($order)"
                sed 's/^/       /' "$OUT/link_$order.log"
            fi
        done
    elif grep -qiE "odr|duplicate symbol|multiple definition" "$OUT/link_libfirst.log" "$OUT/link_mainfirst.log"; then
        fail "the linker diagnosed it; Ch 27 says it says nothing at all   [Ch 27]"
    else
        pass "both link orders linked silently, exit 0   [Ch 27]"
    fi

    # (b) the two orders disagree. Compare only the v2 caller's line: it is 999
    # in one order and 30 in the other, both initialized. The v1 lib line is the
    # overread, so it is never compared.
    if [ -x "$OUT/demo_libfirst" ] && [ -x "$OUT/demo_mainfirst" ]; then
        # Through run_rc, never $( ): these two are the UNSANITIZED demo and one
        # order overreads a stack object, so a nonzero exit is a thing that can
        # happen here. Inside a command substitution that would take set -e with
        # it and kill the script mid-run - no verdict for this check, none for
        # (c), and no closing line either way.
        RC_LIB=$(run_rc "$OUT/demo_libfirst"  "$OUT/run_libfirst.log")
        RC_MAIN=$(run_rc "$OUT/demo_mainfirst" "$OUT/run_mainfirst.log")
        A=$(sed -n 's/^v2 caller sees: //p' "$OUT/run_libfirst.log")
        B=$(sed -n 's/^v2 caller sees: //p' "$OUT/run_mainfirst.log")
        if [ "$RC_LIB" != 0 ] || [ "$RC_MAIN" != 0 ]; then
            fail "a demo exited nonzero without the sanitizers ($RC_LIB and $RC_MAIN); Ch 27 says both produce a working-looking program   [Ch 27]"
        elif [ -z "$A" ] || [ -z "$B" ]; then
            skip "the demonstration printed nothing to compare"
        elif [ "$A" != "$B" ]; then
            pass "the orders disagree: caller saw $A then $B, same source   [Ch 27]"
        else
            fail "both orders printed $A; this linker does not pick by order, so Ch 27's transcript is toolchain-specific and should say so   [Ch 27]"
        fi
    else
        # Reachable only when (a) already failed - but silence here would be a
        # check absent from the run rather than one recorded as not run, and a
        # skip read as a pass is how the wrong claims survived the first time.
        skip "the link produced no binaries, so the two orders cannot be compared"
    fi

    # (c) under the canonical flags, exactly one order is caught. Same shape as
    # (a) - one pair of objects, linked twice - rather than handing the driver
    # both sources per order, which compiled each file twice and left a reader
    # room to wonder whether the COMPILE order mattered too. It does not: below,
    # only the link line differs between the two.
    if $CXX -std=c++17 -g -O0 -fsanitize=address,undefined \
            -c "$OUT/libpart.cpp" -o "$OUT/san_libpart.o" 2>/dev/null \
       && $CXX -std=c++17 -g -O0 -fsanitize=address,undefined \
            -c "$OUT/main.cpp" -o "$OUT/san_main.o" 2>/dev/null \
       && $CXX -fsanitize=address,undefined \
            "$OUT/san_libpart.o" "$OUT/san_main.o" -o "$OUT/san_libfirst" 2>/dev/null \
       && $CXX -fsanitize=address,undefined \
            "$OUT/san_main.o" "$OUT/san_libpart.o" -o "$OUT/san_mainfirst" 2>/dev/null; then
        RC_SAN_LIB=$(run_rc "$OUT/san_libfirst"  "$OUT/san_libfirst.log")
        RC_SAN_MAIN=$(run_rc "$OUT/san_mainfirst" "$OUT/san_mainfirst.log")
        if [ "$RC_SAN_LIB" = 0 ] && [ "$RC_SAN_MAIN" != 0 ]; then
            if grep -q "stack-buffer-overflow" "$OUT/san_mainfirst.log"; then
                pass "exactly one order caught (exit $RC_SAN_MAIN, stack-buffer-overflow); the quiet one exited 0   [Ch 27]"
            else
                fail "the caught order did not report stack-buffer-overflow   [Ch 27]"
            fi
            # Chapter 27 claims two things about this report, and the second is
            # the one a reader acts on: "ASan aborts with a clear report naming
            # the function", "a stack overread naming GetTimeout". A report that
            # says stack-buffer-overflow and names no function of theirs sends
            # them looking in the wrong file. An UNSYMBOLIZED report is a fact
            # about this machine rather than a counter-example, so it skips
            # rather than fails - the same distinction section 2 draws for
            # llvm-symbolizer.
            if grep -q "GetTimeout" "$OUT/san_mainfirst.log"; then
                pass "the report names GetTimeout, not just the overread   [Ch 27]"
            elif ! grep -qE '\.(cpp|h):[0-9]+' "$OUT/san_mainfirst.log"; then
                skip "the ASan report is unsymbolized, so the naming claim is untestable here"
            else
                fail "the report is symbolized but does not name GetTimeout; Ch 27 says it names the function   [Ch 27]"
            fi
        elif [ "$RC_SAN_LIB" = 0 ] && [ "$RC_SAN_MAIN" = 0 ]; then
            fail "neither order was caught; Ch 27 says the sanitizer catches exactly one   [Ch 27]"
        else
            fail "expected the quiet order to exit 0 and the loud one to abort, got $RC_SAN_LIB and $RC_SAN_MAIN   [Ch 27]"
        fi
    else
        skip "sanitizers cannot build the ODR demonstration with $CXX"
    fi
fi

# --- 6. a null const char* handed to std::string ----------------------------
# Recipe 23's trap: `std::string name = Thing_GetName(h);` when the C API
# returns null for "unnamed". The standard says undefined behavior, and the
# two standard libraries the book runs on do different things with it - the
# recipe names both. Keyed by which library compiled the program rather than
# by OS: clang on Linux uses libstdc++ by default, so OS would be the wrong
# key, and a claim about a library should be tested against the library.
echo "== null const char* to std::string =="
cat > "$OUT/nullstr.cpp" <<'EOF'
#include <cstdio>
#include <stdexcept>
#include <string>
const char* Thing_GetName() { return nullptr; }   // a C API's "unnamed"
int main() {
#if defined(_LIBCPP_VERSION)
    std::puts("lib: libc++");
#elif defined(__GLIBCXX__)
    std::puts("lib: libstdc++");
#else
    std::puts("lib: other");
#endif
    std::fflush(stdout);                            // the next line may not return
    try {
        std::string name = Thing_GetName();         // the trap, verbatim
        std::printf("constructed: %zu\n", name.size());
    } catch (const std::logic_error& e) {
        std::printf("threw logic_error: %s\n", e.what());
    }
    return 0;
}
EOF
if $CXX -std=c++17 -g -O0 "$OUT/nullstr.cpp" -o "$OUT/nullstr" 2>/dev/null; then
    RC=$(run_rc "$OUT/nullstr" "$OUT/nullstr.log")
    LIB=$(sed -n 's/^lib: //p' "$OUT/nullstr.log")
    STDLIB=${LIB:-stdlib unknown}
    case "$LIB" in
        libc++)
            if [ "$RC" != 0 ] && ! grep -q "^constructed" "$OUT/nullstr.log"; then
                pass "libc++ dies inside the constructor (exit $RC), nothing to catch   [Recipe 23]"
            else
                fail "libc++ survived a null const char* (exit $RC); Recipe 23 says it dies in the constructor   [Recipe 23]"
            fi ;;
        libstdc++)
            if [ "$RC" = 0 ] && grep -q "^threw logic_error" "$OUT/nullstr.log"; then
                pass "libstdc++ throws std::logic_error   [Recipe 23]"
            else
                fail "libstdc++ did not throw logic_error (exit $RC); Recipe 23 says it does   [Recipe 23]"
            fi ;;
        *)
            skip "an unrecognized standard library; Recipe 23 speaks only for libc++ and libstdc++" ;;
    esac
else
    skip "cannot compile the null-string demonstration with $CXX"
fi

# --- 7. a macro that changes a layout in one TU only -------------------------
# Chapter 26's compile-time-switches section: Chapter 27's diamond with a
# preprocessor define for a cause. session.h is quoted in the chapter and
# pinned to this heredoc by check_verbatim.sh. Three claims: both link orders
# link silently, the two orders disagree at -O0, and - unlike Chapter 27 -
# NEITHER order is caught by the sanitizers, because in THIS program the
# object is built in the larger layout, so every read is inside it (see (c)).
# Like section 5, these are linker claims and hold on every platform alike.
echo "== macro odr =="
cat > "$OUT/session.h" <<'EOF'
// session.h
#pragma once
struct Session {
    int id;
#ifdef AUDIT
    int audit_count;     // present only where AUDIT is defined
#endif
    int timeout;
};
inline int GetTimeout(const Session& s) { return s.timeout; }
EOF
cat > "$OUT/audit_lib.cpp" <<'EOF'
// compiled WITHOUT -DAUDIT: an 8-byte Session
#include "session.h"
int LibTimeout(const Session& s) { return GetTimeout(s); }
EOF
cat > "$OUT/audit_main.cpp" <<'EOF'
// compiled WITH -DAUDIT: a 12-byte Session
#include "session.h"
#include <cstdio>
int LibTimeout(const Session& s);
int main() {
    Session s{};
    s.id = 7;
    s.timeout = 30;                 // audit_count stays 0
    std::printf("main sees: %d\n", GetTimeout(s));
    std::printf("lib  sees: %d\n", LibTimeout(s));
    return 0;
}
EOF
MACRO_OK=1
$CXX -std=c++17 -g -O0 -I"$OUT" -c "$OUT/audit_lib.cpp" -o "$OUT/audit_lib.o" 2>/dev/null || MACRO_OK=0
$CXX -std=c++17 -g -O0 -I"$OUT" -DAUDIT -c "$OUT/audit_main.cpp" -o "$OUT/audit_main.o" 2>/dev/null || MACRO_OK=0
if [ "$MACRO_OK" = 0 ]; then
    skip "cannot compile the macro-ODR demonstration with $CXX"
else
    # (a) both orders link silently
    MLINK=1
    $CXX "$OUT/audit_lib.o" "$OUT/audit_main.o" -o "$OUT/macro_libfirst"  > "$OUT/macro_link_a.log" 2>&1 || MLINK=0
    $CXX "$OUT/audit_main.o" "$OUT/audit_lib.o" -o "$OUT/macro_mainfirst" > "$OUT/macro_link_b.log" 2>&1 || MLINK=0
    if [ "$MLINK" = 0 ]; then
        fail "a link order failed to link at all   [Ch 26]"
        for order in a b; do
            if [ -s "$OUT/macro_link_$order.log" ]; then
                echo "       (order $order)"
                sed 's/^/       /' "$OUT/macro_link_$order.log"
            fi
        done
    elif grep -qiE "odr|duplicate symbol|multiple definition" "$OUT/macro_link_a.log" "$OUT/macro_link_b.log"; then
        fail "the linker diagnosed it; Ch 26 says it says nothing   [Ch 26]"
    else
        pass "both link orders linked silently, exit 0   [Ch 26]"
    fi
    # (b) the two orders disagree about what main reads
    if [ -x "$OUT/macro_libfirst" ] && [ -x "$OUT/macro_mainfirst" ]; then
        RC_A=$(run_rc "$OUT/macro_libfirst"  "$OUT/macro_run_a.log")
        RC_B=$(run_rc "$OUT/macro_mainfirst" "$OUT/macro_run_b.log")
        A=$(sed -n 's/^main sees: //p' "$OUT/macro_run_a.log")
        B=$(sed -n 's/^main sees: //p' "$OUT/macro_run_b.log")
        if [ "$RC_A" != 0 ] || [ "$RC_B" != 0 ]; then
            fail "a demo exited nonzero ($RC_A and $RC_B); Ch 26 says both run to completion   [Ch 26]"
        elif [ -z "$A" ] || [ -z "$B" ]; then
            skip "the macro-ODR demonstration printed nothing to compare"
        elif [ "$A" != "$B" ]; then
            pass "the orders disagree: main saw $A then $B, same source   [Ch 26]"
        else
            fail "both orders printed $A; this linker does not pick by order, so Ch 26's transcript is toolchain-specific   [Ch 26]"
        fi
    else
        skip "the link produced no binaries, so the two orders cannot be compared"
    fi
    # (c) under the canonical flags, NEITHER order is caught - a claim about
    # THIS program, and the chapter says so: main.cpp, which builds the
    # object, is the -DAUDIT (larger) translation unit on purpose, so every
    # read lands inside it. Build the object in the smaller layout and the
    # larger layout's reader overreads, which ASan does catch - Chapter 27's
    # asymmetry. What is asserted is the silence the chapter's listing
    # produces, not a property of the bug class.
    if $CXX -std=c++17 -g -O0 -fsanitize=address,undefined -I"$OUT" \
            -c "$OUT/audit_lib.cpp" -o "$OUT/san_audit_lib.o" 2>/dev/null \
       && $CXX -std=c++17 -g -O0 -fsanitize=address,undefined -I"$OUT" -DAUDIT \
            -c "$OUT/audit_main.cpp" -o "$OUT/san_audit_main.o" 2>/dev/null \
       && $CXX -fsanitize=address,undefined \
            "$OUT/san_audit_lib.o" "$OUT/san_audit_main.o" -o "$OUT/san_macro_a" 2>/dev/null \
       && $CXX -fsanitize=address,undefined \
            "$OUT/san_audit_main.o" "$OUT/san_audit_lib.o" -o "$OUT/san_macro_b" 2>/dev/null; then
        RC_SA=$(run_rc "$OUT/san_macro_a" "$OUT/san_macro_a.log")
        RC_SB=$(run_rc "$OUT/san_macro_b" "$OUT/san_macro_b.log")
        if [ "$RC_SA" = 0 ] && [ "$RC_SB" = 0 ]; then
            pass "neither order caught: every read was in bounds, both exited 0   [Ch 26]"
        else
            fail "a sanitizer caught an order ($RC_SA and $RC_SB); Ch 26 says neither is caught   [Ch 26]"
        fi
    else
        skip "sanitizers cannot build the macro-ODR demonstration with $CXX"
    fi
fi

# --- 8. a frame that does not fit the stack ---------------------------------
# Recipe 34's trap and Appendix E's stack-overflow entry: the crash is on
# entry to the function, and AddressSanitizer's NAME for it is one of two -
# "stack-overflow" when the faulting write lands within 64 KB of the stack
# pointer (compiler-rt's IsStackOverflow window), a bare SEGV/BUS "on unknown
# address" otherwise - and which one depends on what happens to be mapped
# below the thread's stack, not on the platform or the overshoot. The first
# draft of this section wrote the maintainer's macOS/arm64 answer down as the
# rule; CI's Linux leg then answered the same binary three different ways in
# three runs, which is this script's founding mistake caught by this script.
# So it asserts only what every run on every platform shares: nonzero exit,
# one of those two names, frame #0 in the zeroing routine, and no allocation
# stack - the four things the book claims - and it bounds the run, because a
# frame that walks far below the stack can hang the runtime mid-report.
echo "== stack overflow report names =="
if $CXX -std=c++17 -g -fsanitize=address -pthread "$OUT/frame_600.cpp" -o "$OUT/frame_600" 2>/dev/null \
   && $CXX -std=c++17 -g -fsanitize=address -pthread "$OUT/frame_1024.cpp" -o "$OUT/frame_1024" 2>/dev/null; then
    RC_S=$(run_rc_bounded "$OUT/frame_600" "$OUT/frame_600.log" 60)
    RC_L=$(run_rc_bounded "$OUT/frame_1024" "$OUT/frame_1024.log" 60)
    if [ "$RC_S" = 124 ] || [ "$RC_L" = 124 ]; then
        fail "a frame past a 512 KB stack hung for a minute instead of crashing ($RC_S / $RC_L)   [Recipe 34]"
    elif [ "$RC_S" = 0 ] || [ "$RC_L" = 0 ]; then
        fail "a frame past a 512 KB stack ran to completion ($RC_S / $RC_L); Recipe 34 says it crashes on entry   [Recipe 34]"
    else
        pass "both frames crashed, exit $RC_S and $RC_L   [Recipe 34, App E]"
    fi
    for KB in 600 1024; do
        NAME=$(grep -m1 -oE 'AddressSanitizer: [A-Za-z-]+' "$OUT/frame_$KB.log" | cut -d' ' -f2 || true)
        if printf '%s' "$NAME" | grep -qE '^(stack-overflow|SEGV|BUS)$'; then
            pass "$KB KB on a 512 KB stack: ASan names it $NAME (one of the two the book allows)   [Recipe 34, App E]"
        else
            fail "$KB KB on a 512 KB stack: expected stack-overflow, SEGV or BUS, got '${NAME:-(no ASan line)}'   [Recipe 34, App E]"
        fi
        if grep -qE '#0 .*(memset|bzero)' "$OUT/frame_$KB.log"; then
            pass "$KB KB: frame #0 is the zeroing routine   [Ch 31 symptom index]"
        else
            fail "$KB KB: frame #0 is not memset/bzero: $(grep -m1 -oE '#0 .*' "$OUT/frame_$KB.log" | cut -c1-80)   [Ch 31 symptom index]"
        fi
    done
    if grep -q "allocated by" "$OUT/frame_600.log" "$OUT/frame_1024.log"; then
        fail "a stack-overflow report carried an allocation stack; Recipe 34 says there is none   [Recipe 34]"
    else
        pass "neither report carries an allocation site   [Recipe 34, Ch 31]"
    fi
else
    skip "AddressSanitizer cannot build the stack-overflow demonstrations with $CXX"
fi

echo
if [ "$FAILED" = 0 ]; then
    echo "platform claims OK ($OS/$ARCH, $STDLIB)"
else
    echo "check_platform_claims.sh: a claim in the book does not hold on" >&2
    echo "  $OS/$ARCH with $CXX. Fix the chapter, not this script - each FAIL" >&2
    echo "  above names the claim. Sections 1-4 are per-platform, where the" >&2
    echo "  mistake is one platform's behavior written down as the rule;" >&2
    echo "  sections 5 and 7 are linker claims that hold everywhere alike, so a" >&2
    echo "  failure there means this toolchain differs from the chapter's transcript;" >&2
    echo "  section 6 is per standard library, keyed by the library's macro;" >&2
    echo "  section 8 asserts only what every platform and every run share." >&2
    exit 1
fi
