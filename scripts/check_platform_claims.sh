#!/usr/bin/env bash
# Check the book's per-platform sanitizer claims against the machine it runs on.
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
# waiting for a reader on the other operating system to find it.
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

# Prefer clang++: every claim below is about a compiler-rt runtime, and the
# book's transcripts are clang's. Fall back to whatever c++ is, which is what a
# reader following the chapters would have typed.
if [ -n "${CXX:-}" ]; then :
elif command -v clang++ >/dev/null 2>&1; then CXX=clang++
else CXX=c++
fi

OS=$(uname -s)
ARCH=$(uname -m)
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT

FAILED=0
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

# Run a program and report its exit code, without set -e killing the script.
run_rc() {
    local prog="$1" logfile="$2"; shift 2
    local rc=0
    "$prog" > "$logfile" 2>&1 || rc=$?
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

echo
if [ "$FAILED" = 0 ]; then
    echo "platform claims OK ($OS/$ARCH)"
else
    echo "check_platform_claims.sh: a claim in the book does not hold on $OS/$ARCH." >&2
    echo "  Fix the chapter, not this script - the whole point is that the book" >&2
    echo "  states one platform's behavior as the rule when it is not." >&2
    exit 1
fi
