#!/usr/bin/env bash
# Build and run every reference solution under strict flags + sanitizers.
# This is the repo's core invariant; CI runs the same script.
#
#   scripts/build_all.sh                   -> everything; cmake and TSan may SKIP
#   scripts/build_all.sh --require-cmake   -> also fail if cmake is missing (CI)
#   scripts/build_all.sh --require-tsan    -> also fail if ThreadSanitizer is
#                                             unusable here (CI)
set -euo pipefail
cd "$(dirname "$0")/.."

REQUIRE_CMAKE=0
REQUIRE_TSAN=0
while [ $# -gt 0 ]; do
    case "$1" in
        --require-cmake) REQUIRE_CMAKE=1; shift ;;
        --require-tsan)  REQUIRE_TSAN=1;  shift ;;
        *) echo "build_all.sh: unknown argument: $1" >&2; exit 2 ;;
    esac
done

CXX=${CXX:-g++}
FLAGS="-std=c++17 -Wall -Wextra -fsanitize=address,undefined -g"
# words.cpp uses C++20 (std::erase_if)
FLAGS20="-std=c++20 -Wall -Wextra -fsanitize=address,undefined -g"
OUT=$(mktemp -d)

run() { echo "== $1"; shift; "$@"; }

run "tracer"      $CXX $FLAGS   solutions/tracer.cpp                    -o $OUT/tracer
run "buffer"      $CXX $FLAGS   solutions/buffer.cpp                    -o $OUT/buffer
run "fakesdk"     $CXX $FLAGS   exercises/fakesdk/FakeSDK.cpp solutions/fakesdk_solution.cpp -I exercises/fakesdk -o $OUT/fakesdk
run "device"      $CXX $FLAGS   exercises/fakedevice/FakeDevice.cpp solutions/device_solution.cpp -I exercises/fakedevice -o $OUT/device
# Chapter 29's lab. Built here under ASan/UBSan and again under TSan further
# down: the two do not combine, and a threaded lifetime bug is a use-after-free
# ASan names outright while TSan may say nothing at all - so one build is half
# the check. The chapter's own last pitfall, applied to this script.
run "threaded"    $CXX $FLAGS   exercises/fakedevice/FakeDevice.cpp solutions/device_threaded_solution.cpp -I exercises/fakedevice -o $OUT/threaded
run "words"       $CXX $FLAGS20 solutions/words.cpp                     -o $OUT/words
run "shapes"      $CXX $FLAGS   solutions/shapes.cpp                    -o $OUT/shapes
run "invalid"     $CXX $FLAGS20 solutions/invalid.cpp                   -o $OUT/invalid
run "lambdas"     $CXX $FLAGS   solutions/lambdas.cpp                   -o $OUT/lambdas
# buildlab is exercise scaffolding, not a solution — but its starting point must stay green
run "buildlab"    $CXX $FLAGS   exercises/buildlab/Greeter.cpp exercises/buildlab/main.cpp -o $OUT/buildlab
# Chapter 28's harness and suite, verbatim from the chapter. -I solutions because
# the class under test is the Chapter 15 solution, extracted into solutions/Buffer.h
# so a demo with main() and a test binary with its own can both include it — the
# extraction IS what the chapter teaches. Building it under the canonical flags is
# the chapter's own key principle: the assertions supply the workload, the
# sanitizers supply the verdict.
run "buffer_test" $CXX $FLAGS   exercises/testlab/buffer_test.cpp -I solutions -o $OUT/buffer_test
# Chapter 30's three worked boundaries, each a separate binary of TWO translation
# units - the boundary's implementation, and a caller compiled against the header
# alone. The separation is the subject matter, not a build detail: merged into
# one TU the demos would still pass while proving nothing, because the caller
# could see everything the header hides. What stays book-only is the chapter's
# break-it-first half (the Naive layout break, the inserted vtable entry, the
# relink against a changed Impl): each needs a caller binary that was NOT rebuilt,
# and exists to fail.
run "widget"      $CXX $FLAGS   exercises/abilab/Widget.cpp exercises/abilab/widget_demo.cpp -o $OUT/widget
run "scorer"      $CXX $FLAGS   exercises/abilab/scorer.cpp exercises/abilab/scorer_demo.cpp -o $OUT/scorer
run "engine"      $CXX $FLAGS   exercises/abilab/engine.cpp exercises/abilab/engine_demo.cpp -o $OUT/engine
# Appendix F's recipes, one binary per domain. The recipe functions are quoted
# verbatim in book/F-rosetta-cookbook.md (the testlab discipline: editing one
# means editing the appendix in the same commit), and each main() asserts what
# its recipes claim - so a green run keeps the cookbook true, not just compiling.
run "cb_files"    $CXX $FLAGS   exercises/cookbook/files.cpp            -o $OUT/cb_files
run "cb_strings"  $CXX $FLAGS   exercises/cookbook/strings.cpp          -o $OUT/cb_strings
run "cb_timing"   $CXX $FLAGS   exercises/cookbook/timing.cpp           -o $OUT/cb_timing
run "cb_handles"  $CXX $FLAGS   exercises/cookbook/handles.cpp          -o $OUT/cb_handles
run "cb_lookups"  $CXX $FLAGS   exercises/cookbook/lookups.cpp          -o $OUT/cb_lookups
run "cb_paths"    $CXX $FLAGS   exercises/cookbook/paths.cpp            -o $OUT/cb_paths
run "cb_async"    $CXX $FLAGS   exercises/cookbook/async.cpp            -o $OUT/cb_async
run "cb_events"   $CXX $FLAGS   exercises/cookbook/events.cpp           -o $OUT/cb_events
run "cb_logging"  $CXX $FLAGS   exercises/cookbook/logging.cpp          -o $OUT/cb_logging
# Chapter 32's lab, built TWICE with the translation units in opposite orders.
# The chapter's bug is decided by link order, so the fix's whole claim is that
# order no longer matters - one build proves it compiles, two builds prove the
# claim. The broken 2.4.1 version stays book-only (it exists to fail).
run "exitlab_a"   $CXX $FLAGS   exercises/exitlab/logger.cpp exercises/exitlab/audit.cpp exercises/exitlab/main.cpp -o $OUT/exitlab_a
run "exitlab_b"   $CXX $FLAGS   exercises/exitlab/audit.cpp exercises/exitlab/logger.cpp exercises/exitlab/main.cpp -o $OUT/exitlab_b
# Chapter 33's lab. The committed files are the FIXED state (the broken 2.6.0
# main lives in the lab's TASK.md and the chapter - it exists to fail). One
# build, but run twice below at different hot-plug counts: the fix's claim is
# that container growth stopped mattering, and one count cannot prove a claim
# about all of them.
run "reportlab"   $CXX $FLAGS   exercises/reportlab/registry.cpp exercises/reportlab/main.cpp -o $OUT/reportlab
# Chapter 34's lab. The committed files are the FIXED state (the broken
# overlay parser lives in the lab's TASK.md and the chapter - it exists to
# fail). The binary asserts the decode of the ticket's capture against the
# chapter's hand decode, because the oracle for this lab is the ICD, not a
# sanitizer: nothing in the canonical flags checks padding, byte order, or
# aliasing. The capture's second frame sits at offset 10, off every
# four-byte boundary - decoding at any offset is part of the fix's claim.
run "capturelab"  $CXX $FLAGS   exercises/capturelab/wire.cpp exercises/capturelab/main.cpp -o $OUT/capturelab
# Chapter 35's lab. FakeSDK2.* is vendor code (the 2.0 upgrade of Chapter
# 17's SDK); ref.h and main.cpp are the FIXED port (the broken 2.0 port
# lives in the lab's TASK.md and the chapter - it exists to fail). The
# fix's claim is exact balance - every reference released once, none twice
# - and each direction needs its own judge: the binary asserts the
# vendor's live-object counter reaches 0 after shutdown (a release too
# few), and the sanitizers watch for the opposite (a release too many).
run "comlab"      $CXX $FLAGS   exercises/comlab/FakeSDK2.cpp exercises/comlab/main.cpp -o $OUT/comlab
# Chapter 36's lab. The committed files are the FIXED state (the broken
# 2.1.0 Tick lives in the lab's TASK.md and the chapter - it exists to
# fail). The harness replaces operator new and asserts ZERO heap
# allocations across the session's ticks - the judge for this chapter's
# bug class, because the sanitizers and the warnings are silent on an
# accidental copy, and a timing assert would measure the runner instead
# of the code. Run twice below at different session lengths.
run "perflab"     $CXX $FLAGS   exercises/perflab/meter.cpp exercises/perflab/main.cpp -o $OUT/perflab
# Chapter 37's lab. The committed files are the FIXED state (the broken
# 3.4.0 session.cpp lives in the lab's TASK.md and the chapter - it
# exists to fail, at -O2, so the reader can hold a post-mortem on the
# corpse). Run twice below, once per device configuration: the crash
# lived only in the configuration the test matrix never had.
run "dumplab"     $CXX $FLAGS   exercises/dumplab/session.cpp exercises/dumplab/main.cpp -o $OUT/dumplab
# Chapter 38's lab. The committed files are the FIXED state (the three
# broken shapes live in the lab's TASK.md and the chapter - they exist to
# fail). Only one of the three breaks has a sanitizer; the other two are
# hangs, so the harness's judge is a bounded wait - every invoke carries a
# deadline, and a timeout is a failed check with a line number instead of
# a stopped CI run. Built AGAIN under -fsanitize=thread further down: the
# breaks split across the two builds, and each build alone checks half.
run "bridgelab"   $CXX $FLAGS   exercises/bridgelab/main.cpp            -o $OUT/bridgelab
# Appendix H's measurements. Not an exercise - these are the numbers the
# appendix quotes (what a sink costs against const&, what vector growth
# does to element addresses, and what the copy/move tally cannot see: the
# allocation a by-value sink makes and a const& assignment reuses).
run "ch_passing"  $CXX $FLAGS   exercises/choosing/passing.cpp          -o $OUT/ch_passing
run "ch_storing"  $CXX $FLAGS   exercises/choosing/storing.cpp          -o $OUT/ch_storing
# passing.cpp a SECOND time with elision switched off. The appendix asserts
# a returned temporary at zero moves and a returned named local at "one at
# most", and says the difference is that only the first is guaranteed - but
# with NRVO on, both measure zero and the page's central distinction is
# never exercised. -fno-elide-constructors leaves mandatory C++17 elision
# alone and removes NRVO, so the two return shapes finally cost different
# things. Same idiom as exitlab's two link orders: one build checks half.
run "ch_noelide"  $CXX $FLAGS -fno-elide-constructors exercises/choosing/passing.cpp -o $OUT/ch_noelide

echo "== running =="
$OUT/tracer > /dev/null
$OUT/buffer > /dev/null
$OUT/fakesdk > /dev/null
$OUT/device > /dev/null
# halt_on_error, unlike every line around it: UBSan's default is
# report-and-continue, so a finding here would print and still exit 0 - and this
# binary exists to BE a gate. (The rest of the script predates the point; see
# ROADMAP item 5.)
UBSAN_OPTIONS=halt_on_error=1 $OUT/threaded > /dev/null
$OUT/words exercises/words/words_sample.txt > /dev/null
$OUT/shapes > /dev/null
$OUT/invalid > /dev/null
$OUT/lambdas > /dev/null
$OUT/buildlab > /dev/null
# Not silenced: the tally is the only line in this script that says how MUCH was
# checked, and a non-zero exit is the whole contract between a test binary and
# CI. But green and red want different amounts of output, so the run is captured
# rather than piped: on green, the tally alone; on red, every line - the FAILED
# expression and its file:line, which is the entire reason Chapter 28's CHECK is
# a macro. Piping to `tail -1` would throw exactly that away and leave a CI log
# saying a test failed without saying which.
$OUT/buffer_test > "$OUT/buffer_test.log" || { cat "$OUT/buffer_test.log"; exit 1; }
tail -1 "$OUT/buffer_test.log"
# halt_on_error for the same reason as the threaded binary above: these three
# assert values rather than survival, and a UBSan finding that printed and
# exited 0 would leave the section green.
UBSAN_OPTIONS=halt_on_error=1 $OUT/widget > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/scorer > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/engine > /dev/null
# The cookbook binaries assert values too - same reasoning, same option.
UBSAN_OPTIONS=halt_on_error=1 $OUT/cb_files > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/cb_strings > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/cb_timing > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/cb_handles > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/cb_lookups > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/cb_paths > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/cb_async > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/cb_events > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/cb_logging > /dev/null
# Both link orders of the Chapter 32 lab: surviving exit IS the claim here.
UBSAN_OPTIONS=halt_on_error=1 $OUT/exitlab_a > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/exitlab_b > /dev/null
# The Chapter 33 lab at both extremes: no container growth at all, and growth
# several reallocations deep. Surviving growth IS the claim here.
UBSAN_OPTIONS=halt_on_error=1 $OUT/reportlab 0 > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/reportlab 100 > /dev/null
# The Chapter 34 lab: the golden-capture assert IS the gate here - the
# sanitizers have nothing to say about this chapter's bug class.
UBSAN_OPTIONS=halt_on_error=1 $OUT/capturelab > /dev/null
# The Chapter 35 lab: both judges at once - the counter assert inside the
# binary, the sanitizers around it.
UBSAN_OPTIONS=halt_on_error=1 $OUT/comlab > /dev/null
# The Chapter 36 lab at two session lengths: zero-allocations-per-tick is a
# claim of independence from session length, and one length cannot prove it.
UBSAN_OPTIONS=halt_on_error=1 $OUT/perflab 50 > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/perflab 1000 > /dev/null
# The Chapter 37 lab under both device configurations - the bench's
# calibrated unit and the field's base model. The crash lived only in the
# second, and one configuration cannot prove a claim about both.
UBSAN_OPTIONS=halt_on_error=1 $OUT/dumplab 1 > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/dumplab 0 > /dev/null
# The Chapter 38 lab: the bounded-wait judge inside the binary, the
# sanitizers around it. Modal drains happen mid-run, so the HOST_BUSY
# refusal path is genuinely exercised, not just compiled.
UBSAN_OPTIONS=halt_on_error=1 $OUT/bridgelab > /dev/null
# Appendix H: these check copy/move counts, heap allocations and element
# addresses, so a UBSan finding that printed and exited 0 would leave the
# section green. Their judge is a counting CHECK macro rather than assert,
# so a -DNDEBUG build would still verify - but these runs are the canonical
# ones. The third is the same source with NRVO switched off.
UBSAN_OPTIONS=halt_on_error=1 $OUT/ch_passing > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/ch_storing > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/ch_noelide > /dev/null

# Chapter 26's CMakeLists, configured, built and run both ways. The reference
# file in exercises/buildlab/ is the shape that chapter ENDS on, assembled from
# its snippets, so this holds the chapter's destination to the same standard as
# everything above. What it does not hold: the forms the chapter passes through
# on the way - the first single-executable build, the sanitizer flags before
# they move onto a target of their own - which live in no file at all.
#
# cmake is not part of the toolchain the rest of this script needs, so a
# laptop without it stays green and says so rather than pretending. CI passes
# --require-cmake, which refuses to skip - same bargain as check_mermaid.sh.
echo "== buildlab cmake =="
if command -v cmake > /dev/null 2>&1; then
    CM=build/buildlab-cmake            # under build/, which is gitignored

    # Where the binary lands depends on the generator: a multi-config one
    # (Xcode, Visual Studio) adds a per-config subdirectory, a single-config
    # one does not. The script does not pin a generator - the reader's default
    # is what the chapter is about - so it looks rather than assumes.
    greet_binary() {
        local dir=$1 candidate
        for candidate in "$dir/greet" "$dir/greet.exe" \
                         "$dir/Debug/greet" "$dir/Debug/greet.exe"; do
            if [ -x "$candidate" ]; then echo "$candidate"; return 0; fi
        done
        echo "build_all.sh: built $dir but found no greet executable in it" >&2
        return 1
    }

    # Fresh directories every run: a stale cache is its own class of build bug,
    # and this configures three files, so hermetic costs about a second.
    rm -rf "$CM" "$CM-asan"
    # CMAKE_EXPORT_COMPILE_COMMANDS so the flags can be read back below.
    cmake -S exercises/buildlab -B "$CM" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON > /dev/null
    cmake --build "$CM" --config Debug > /dev/null
    GREET=$(greet_binary "$CM")        # set -e stops here if it found nothing
    "$GREET" > /dev/null
    # The GREETER_SANITIZE switch is only worth having if it still works.
    cmake -S exercises/buildlab -B "$CM-asan" \
        -DCMAKE_BUILD_TYPE=Debug -DGREETER_SANITIZE=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON > /dev/null
    cmake --build "$CM-asan" --config Debug > /dev/null
    GREET_ASAN=$(greet_binary "$CM-asan")
    "$GREET_ASAN" > /dev/null

    # "It configured, built and ran" does NOT prove the switch did anything.
    # Rename the option and cmake merely warns that a -D went unused, exits 0,
    # and this whole section stays green while checking nothing - the exact
    # rot it exists to catch. So read the flags back out of the compile
    # database: instrumented when asked, and NOT instrumented when not.
    for db in "$CM/compile_commands.json" "$CM-asan/compile_commands.json"; do
        if [ ! -f "$db" ]; then
            echo "build_all.sh: no $db. Only the Makefile and Ninja generators" >&2
            echo "  write a compile database, and without one the flags cannot" >&2
            echo "  be verified - re-run with CMAKE_GENERATOR unset, or set to" >&2
            echo "  Ninja. (CI uses the default, which writes one.)" >&2
            exit 1
        fi
    done
    # main.cpp, not just any file: it belongs to `greet`, so a hit proves the
    # flags reached the executable through the INTERFACE target as well.
    if ! grep -q -- '-fsanitize=address.*main\.cpp' "$CM-asan/compile_commands.json"; then
        echo "build_all.sh: GREETER_SANITIZE=ON did not put -fsanitize=address" >&2
        echo "  on main.cpp - the option or the sanitizers target is broken." >&2
        exit 1
    fi
    if grep -q -- '-fsanitize=address' "$CM/compile_commands.json"; then
        echo "build_all.sh: the default configuration is instrumented, so" >&2
        echo "  GREETER_SANITIZE is not a switch." >&2
        exit 1
    fi
    echo "  ok   exercises/buildlab/CMakeLists.txt (default, and GREETER_SANITIZE=ON)"
elif [ "$REQUIRE_CMAKE" = 1 ]; then
    echo "build_all.sh: cmake not found, and --require-cmake was given" >&2
    exit 1
else
    echo "  SKIPPED - cmake not installed (CI runs this for real)"
fi

# Chapter 29's lab under the third sanitizer. TSan cannot be combined with ASan,
# so this is a second build of the same source rather than a longer flag list -
# which is the chapter's point about them made structurally.
#
# Same bargain as the cmake step above, for the same reason: ThreadSanitizer is
# not part of the toolchain the rest of this script needs. It is missing from
# some cross-compilers and 32-bit targets, and on a Linux host it can also be
# present but unable to start, because its shadow mapping collides with the
# host's ASLR entropy. So the probe below both COMPILES and RUNS a trivial
# program - "the flag exists" is not the question, "does an instrumented binary
# start here" is. CI passes --require-tsan, which refuses to skip.
echo "== threaded tsan =="
TFLAGS="-std=c++17 -Wall -Wextra -fsanitize=thread -g"
printf 'int main() { return 0; }\n' > "$OUT/tsan_probe.cpp"
if $CXX $TFLAGS "$OUT/tsan_probe.cpp" -o "$OUT/tsan_probe" > /dev/null 2>&1 \
   && "$OUT/tsan_probe" > /dev/null 2>&1; then
    $CXX $TFLAGS exercises/fakedevice/FakeDevice.cpp \
        solutions/device_threaded_solution.cpp -I exercises/fakedevice \
        -o "$OUT/threaded_tsan"
    # halt_on_error buys a shorter log here, not a failing run - unlike the
    # UBSan line further up, whose default really is report-and-exit-0. TSan
    # fails the run on its own (exitcode=66 on Linux, abort on Darwin), so what
    # this adds is stopping AT the first race rather than letting it scroll past
    # under whatever the program goes on to report.
    TSAN_OPTIONS=halt_on_error=1 "$OUT/threaded_tsan" > /dev/null
    echo "  ok   solutions/device_threaded_solution.cpp under -fsanitize=thread"
    # Chapter 38's lab under the same probe - a second source for this
    # section, not a second section: its registry-race break is TSan-only,
    # so without this build that break has no judge at all.
    $CXX $TFLAGS exercises/bridgelab/main.cpp -o "$OUT/bridgelab_tsan"
    TSAN_OPTIONS=halt_on_error=1 "$OUT/bridgelab_tsan" > /dev/null
    echo "  ok   exercises/bridgelab/main.cpp under -fsanitize=thread"
elif [ "$REQUIRE_TSAN" = 1 ]; then
    echo "build_all.sh: ThreadSanitizer cannot build and run a trivial program" >&2
    echo "  with $CXX, and --require-tsan was given. Re-run the probe by hand to" >&2
    echo "  see why: $CXX $TFLAGS <a main() returning 0>" >&2
    exit 1
else
    echo "  SKIPPED - no usable ThreadSanitizer here (CI runs this for real)"
fi

echo "ALL GREEN"
