#!/usr/bin/env bash
# Build and run every reference solution under strict flags + sanitizers.
# This is the repo's core invariant; CI runs the same script.
#
#   scripts/build_all.sh                   -> everything; cmake, git, TSan and
#                                             the one C++23 listing may SKIP
#   scripts/build_all.sh --require-cmake   -> also fail if cmake is missing (CI)
#   scripts/build_all.sh --require-git     -> also fail if git cannot build and
#                                             clone a file:// repository (CI)
#   scripts/build_all.sh --require-tsan    -> also fail if ThreadSanitizer is
#                                             unusable here (CI)
#   scripts/build_all.sh --require-expected -> also fail if the compiler has no
#                                             C++23 <expected> (CI)
set -euo pipefail
cd "$(dirname "$0")/.."

REQUIRE_CMAKE=0
REQUIRE_TSAN=0
REQUIRE_GIT=0
REQUIRE_EXPECTED=0
while [ $# -gt 0 ]; do
    case "$1" in
        --require-cmake)    REQUIRE_CMAKE=1;    shift ;;
        --require-tsan)     REQUIRE_TSAN=1;     shift ;;
        --require-git)      REQUIRE_GIT=1;      shift ;;
        --require-expected) REQUIRE_EXPECTED=1; shift ;;
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
# Appendix I's lab. This is the half that must COMPILE; the five that must
# not are a section of their own further down.
run "constlab"    $CXX $FLAGS   exercises/constlab/main.cpp -o $OUT/constlab
# Chapter 27's lab, for the same reason and one more. The three cmake paths far
# below do compile these two files, but with whatever the consumer projects ask
# for — mathlib's -Wall -Wextra are PRIVATE and no consume-*/CMakeLists.txt sets
# a warning or sanitizer flag — so the canonical flags never reach them, and on
# a machine without cmake they are not compiled at all. CONTRIBUTING.md's first
# ground rule is about every contributed .cpp, not every solution.
# MATHLIB_VERSION reaches the library from project(VERSION) via CMake; this
# build is about the flags, so it gets a placeholder that cannot drift from
# anything and cannot be mistaken for a version the lab claims.
run "deplab"      $CXX $FLAGS   exercises/deplab/mathlib/src/mathlib.cpp \
                                exercises/deplab/app/main.cpp \
                                -I exercises/deplab/mathlib/include \
                                -DMATHLIB_VERSION='"flags-only"' -o $OUT/deplab
# Chapter 39's boundary, and the harness that plays the marshaller. Two
# translation units on purpose: main.cpp sees only plugin.h, which is the
# subject - a caller that can see the implementation is not a boundary.
run "interoplab" $CXX $FLAGS   exercises/interoplab/plugin.cpp exercises/interoplab/main.cpp -o $OUT/interoplab
# Chapter 41's lab: one Session over two policies, one of them the real
# FakeDevice - linked from its own directory, as threadlab does. The build
# that must FAIL is a section of its own further down.
run "templatelab" $CXX $FLAGS   exercises/fakedevice/FakeDevice.cpp exercises/templatelab/main.cpp -I exercises/fakedevice -I exercises/templatelab -o $OUT/templatelab
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
# logging.cpp a SECOND time with NDEBUG defined: Recipe 24's claim is that an
# assert - side effect included - compiles to nothing under it, and one build
# cannot prove a claim about two.
run "cb_logging_nd" $CXX $FLAGS -DNDEBUG exercises/cookbook/logging.cpp   -o $OUT/cb_logging_nd
run "cb_alternatives" $CXX $FLAGS exercises/cookbook/alternatives.cpp   -o $OUT/cb_alternatives
run "cb_errors"   $CXX $FLAGS   exercises/cookbook/errors.cpp           -o $OUT/cb_errors
# The one cookbook TU with a dependency. -isystem, not -I: the vendored header
# is the vendor's, and -Wall -Wextra are for our code (CONTRIBUTING's note on
# vendoring a framework header, applied to the first dependency that landed).
run "cb_json"     $CXX $FLAGS   -isystem exercises/third_party exercises/cookbook/json.cpp -o $OUT/cb_json
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
run "cho_passing"  $CXX $FLAGS   exercises/choosing/passing.cpp          -o $OUT/cho_passing
run "cho_storing"  $CXX $FLAGS   exercises/choosing/storing.cpp          -o $OUT/cho_storing
# passing.cpp a SECOND time with elision switched off. The appendix asserts
# a returned temporary at zero moves and a returned named local at "one at
# most", and says the difference is that only the first is guaranteed - but
# with NRVO on, both measure zero and the page's central distinction is
# never exercised. -fno-elide-constructors leaves mandatory C++17 elision
# alone and removes NRVO, so the two return shapes finally cost different
# things. Same idiom as exitlab's two link orders: one build checks half.
run "cho_noelide"  $CXX $FLAGS -fno-elide-constructors exercises/choosing/passing.cpp -o $OUT/cho_noelide

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
# halt_on_error: this one asserts values through a CHECK macro, so a UBSan
# finding that printed and exited 0 would leave the section green.
UBSAN_OPTIONS=halt_on_error=1 $OUT/constlab > /dev/null
$OUT/deplab > /dev/null
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
UBSAN_OPTIONS=halt_on_error=1 $OUT/cb_logging_nd > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/cb_alternatives > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/cb_errors > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/cb_json > /dev/null
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
# The Chapter 39 lab: five value assertions across the boundary, so a UBSan
# finding that printed and exited 0 would leave the section green.
UBSAN_OPTIONS=halt_on_error=1 $OUT/interoplab > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/templatelab > /dev/null
# Appendix H: these check copy/move counts, heap allocations and element
# addresses, so a UBSan finding that printed and exited 0 would leave the
# section green. Their judge is a counting CHECK macro rather than assert,
# so a -DNDEBUG build would still verify - but these runs are the canonical
# ones. The third is the same source with NRVO switched off.
UBSAN_OPTIONS=halt_on_error=1 $OUT/cho_passing > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/cho_storing > /dev/null
UBSAN_OPTIONS=halt_on_error=1 $OUT/cho_noelide > /dev/null

# Appendix I's other judge, and the only place in this script that asserts a
# build FAILS. const's whole subject is mistakes that never reach a binary, so
# a harness which only ever compiles things cannot check the one thing that
# appendix is about. Five violations, each behind its own -D.
#
# Two things stop it being vacuous. The clean build far above must SUCCEED, so
# a typo that breaks the file for an unrelated reason turns this script red
# there rather than passing silently here. And the grep sees the MESSAGE only -
# everything up to and including "error:" is cut away first, because the file
# lives in constlab/ and any path left in the string matches "const" by itself.
# What stays unchecked: that violation N is refused for violation N's reason
# rather than some other const-flavoured one. Pinning each to its compiler's
# exact wording would pin this script to a compiler, which is the worse trade.
echo "== constlab refusals =="
for V in 1 2 3 4 5; do
    MSG=$($CXX -std=c++17 -Wall -Wextra -DCONSTLAB_VIOLATION_$V \
              -c exercises/constlab/main.cpp -o /dev/null 2>&1 \
          | grep -m1 "error:" | sed 's|^.*error:|error:|' || true)
    if [ -z "$MSG" ]; then
        echo "build_all.sh: constlab violation $V COMPILED; it must not." >&2
        exit 1
    fi
    if ! printf '%s' "$MSG" | grep -qiE 'const|read-only'; then
        echo "build_all.sh: constlab violation $V was refused, but not for a" >&2
        echo "  const reason: $MSG" >&2
        exit 1
    fi
done
echo "  ok   five const violations refused, each naming const   [App I]"

# Chapter 41's refusal: a policy missing Poll, and the detection idiom's
# static_assert must be what refuses it - by its own sentence, as the FIRST
# error, not the compiler's "no member named 'Poll'" from inside Pump. The
# broken block in main.cpp CALLS Pump on purpose: a class template's member
# is compiled only when used, so without that call the policy would compile
# clean with the static_assert deleted and this section would judge nothing.
# Same discipline as constlab above, with the same precondition: the clean
# "templatelab" build far above must SUCCEED first, so an unrelated error in
# main.cpp turns the script red there rather than passing silently here.
echo "== templatelab refusal =="
MSG=$($CXX -std=c++17 -Wall -Wextra -DTEMPLATELAB_BROKEN_POLICY \
          -I exercises/fakedevice -I exercises/templatelab \
          -c exercises/templatelab/main.cpp -o /dev/null 2>&1 \
      | grep -m1 "error:" | sed 's|^.*error:|error:|' || true)
if [ -z "$MSG" ]; then
    echo "build_all.sh: templatelab's broken policy COMPILED; it must not." >&2
    exit 1
fi
if ! printf '%s' "$MSG" | grep -q 'Sdk policy needs'; then
    echo "build_all.sh: templatelab's broken policy was refused, but not by the" >&2
    echo "  static_assert - the first error was: $MSG" >&2
    exit 1
fi
echo "  ok   a policy missing Poll refused by the static_assert's own sentence   [Ch 41]"

# Chapter 26's CMakeLists, configured, built and run three ways. The reference
# file in exercises/buildlab/ is the shape that chapter ENDS on, assembled from
# its snippets, so this holds the chapter's destination to the same standard as
# everything above. What it does not hold: the forms the chapter passes through
# on the way - the first single-executable build, the sanitizer flags before
# they move onto a target of their own - which live in no file at all.
#
# cmake is not part of the toolchain the rest of this script needs, so a
# laptop without it stays green and says so rather than pretending. CI passes
# --require-cmake, which refuses to skip - same bargain as check_mermaid.sh.
# Chapter 27's Try it, steps 1-4: one library consumed three ways, and a
# version pin proved rather than demonstrated. Shares the buildlab section's cmake bargain below -
# both live under the same probe, and --require-cmake covers both.
echo "== deplab cmake =="
if command -v cmake > /dev/null 2>&1; then
    DL=build/deplab                    # under build/, which is gitignored
    rm -rf "$DL"; mkdir -p "$DL"

    dep_app() {                        # multi-config generators add a subdir
        local dir=$1 candidate
        for candidate in "$dir/app" "$dir/app.exe" \
                         "$dir/Debug/app" "$dir/Debug/app.exe"; do
            if [ -x "$candidate" ]; then echo "$candidate"; return 0; fi
        done
        echo "build_all.sh: built $dir but found no app executable in it" >&2
        return 1
    }

    # --- path 1: vendored, via add_subdirectory ---
    cmake -S exercises/deplab/consume-vendored -B "$DL/vendored" > /dev/null
    cmake --build "$DL/vendored" --config Debug > /dev/null
    APP=$(dep_app "$DL/vendored")
    "$APP" > /dev/null
    echo "  ok   vendored: add_subdirectory -> $("$APP")"

    # The chapter's step 2 asks the reader to CONFIRM the app names no header
    # path. "It built" cannot see the difference between PUBLIC working and a
    # belt-and-braces include_directories() making it redundant, so grep for
    # the thing that must not be there.
    #
    # Comments stripped first: that file EXPLAINS that it names no include
    # path, so a naive grep matches the sentence saying so - which is exactly
    # what happened the first time this check ran.
    # All THREE consumers, not just the vendored one: the lab's headline claim
    # is that the app cannot tell the three apart, and a judge reading one file
    # cannot say that.
    #
    # Two patterns, because two different things are being spelled. CMake
    # command names are case-insensitive, so INCLUDE_DIRECTORIES is the same
    # call as include_directories and a case-sensitive grep walks straight past
    # it - hence -i on that one. A compiler flag is not case-insensitive, so
    # -I is matched exactly, anchored to the characters a flag can follow so
    # that `add_compile_options(-I...)` and a path smuggled into CMAKE_CXX_FLAGS
    # are caught too. A raw -I is what a C# dev reaching for the compiler writes.
    for DEP_C in vendored fetched installed; do
        DEP_TXT=$(sed 's/#.*//' "exercises/deplab/consume-$DEP_C/CMakeLists.txt")
        if printf '%s\n' "$DEP_TXT" | grep -qiE 'include_directories' \
           || printf '%s\n' "$DEP_TXT" | grep -qE '(^|[[:space:]"(;])-I'; then
            echo "build_all.sh: the $DEP_C app names an include path; Ch 27 step 2" >&2
            echo "  says PUBLIC on the library is what carries it, and the lab says" >&2
            echo "  all three consumers are identical. Remove the line." >&2
            exit 1
        fi
    done
    echo "  ok   no consumer names an include path - PUBLIC carried it   [Ch 27]"

    # And the other half of that claim: the app and the link line are the same
    # in all three, so only the acquisition differs. Stated in TASK.md, in
    # exercises/README.md and in two of the three files' own comments, and
    # asserted nowhere until now - link one consumer against the un-namespaced
    # `mathlib` target and everything still builds, runs and prints green.
    # Two distinct lines across three files is the whole assertion.
    DEP_LINK=$(sed 's/#.*//' exercises/deplab/consume-*/CMakeLists.txt \
        | grep -E 'add_executable|target_link_libraries' \
        | tr -s ' ' | sed 's/[[:space:]]*$//' | sort -u)
    if [ "$(printf '%s\n' "$DEP_LINK" | wc -l | tr -d ' ')" != 2 ]; then
        echo "build_all.sh: the three deplab consumers do not share one app and" >&2
        echo "  one link line. Ch 27's lab says only the acquisition differs." >&2
        echo "  Distinct lines found:" >&2
        printf '%s\n' "$DEP_LINK" | sed 's/^/    /' >&2
        exit 1
    fi
    echo "  ok   all three consumers share one app and one link line   [Ch 27]"

    # --- path 2: fetched, and the tag is the pin ---
    # A throwaway repository outside the worktree: FetchContent needs a real
    # git remote, and a git repo nested in this one would make git status a
    # minefield for the next person. file:// is a real remote to git, so the
    # mechanism is identical to a https:// one.
    #
    # Probe by doing the thing rather than looking for the tool - the repo's
    # rule, and it earns its keep here rather than being ceremony. git can be
    # on PATH and still refuse the clone FetchContent is about to make:
    # protocol.file.allow is a documented hardening (CVE-2022-39253) that a
    # site can set to `never` globally, and then file:// is dead while
    # `command -v git` still says yes. So make a one-commit repository and
    # clone it exactly the way FetchContent will. That is the whole question.
    git_can_fetch() {
        local p rc=0
        p=$(mktemp -d) || return 1
        mkdir -p "$p/src"
        git -C "$p/src" init -q > /dev/null 2>&1 \
            && : > "$p/src/probe" \
            && git -C "$p/src" -c user.email=probe@example.invalid \
                   -c user.name=probe -c commit.gpgsign=false \
                   add -A > /dev/null 2>&1 \
            && git -C "$p/src" -c user.email=probe@example.invalid \
                   -c user.name=probe -c commit.gpgsign=false \
                   commit -qm probe > /dev/null 2>&1 \
            && git clone -q "file://$p/src" "$p/dst" > /dev/null 2>&1 || rc=1
        rm -rf "$p"
        return $rc
    }

    if git_can_fetch; then
        DEPREPO=$(mktemp -d)
        # Cleaned on the way out whatever happens, the way every other mktemp -d
        # in scripts/ is (check.sh, check_platform_claims.sh, check_mermaid.sh).
        # This block runs under set -e with a guarded exit, two cmake configures,
        # two builds and a binary lookup between here and the cleanup, so a
        # single `rm` at the end covered only the path where nothing went wrong -
        # every failure leaked a populated git repository into TMPDIR, one per
        # attempt, exactly when someone was iterating on this section. The trap
        # is released at the end of the block rather than left armed, so it
        # cannot fire for a later section's failure and blame this fixture.
        trap 'rm -rf "$DEPREPO"' EXIT
        # A function rather than a string, and every git call in this block goes
        # through it. Two reasons beyond tidiness. An unquoted string expansion
        # splits on whitespace, so a TMPDIR containing a space would tear the
        # -C argument in half and fail three lines later complaining about HEAD.
        # And a fresh repo inherits the developer's GLOBAL git config: with
        # commit.gpgsign set - which plenty of contributors have - git tries to
        # sign as the fake identity below, finds no key for it, and takes the
        # whole script down; tag.gpgsign turns the bare tags into annotated ones
        # and stops in an editor waiting for a message that never comes.
        depgit() {
            git -C "$DEPREPO" \
                -c user.email=lab@example.invalid -c user.name=deplab \
                -c commit.gpgsign=false -c tag.gpgsign=false "$@"
        }
        cp -R exercises/deplab/mathlib/. "$DEPREPO/"
        depgit init -q
        # Separate statements, not `add && commit`: as the left operand of &&,
        # a failing add is exempt from set -e and the error surfaces later,
        # attached to the wrong command.
        depgit add -A
        depgit commit -q -m "mathlib 1.0.0"
        depgit tag v1.0.0
        # A second version, so there is something for the pin to select. The
        # grep first: if the version line ever stops matching, this sed becomes
        # a no-op, the commit below has nothing to commit, and the whole test
        # would fail confusingly instead of saying why.
        grep -q 'VERSION 1\.0\.0' "$DEPREPO/CMakeLists.txt" || {
            echo "build_all.sh: deplab mathlib no longer declares VERSION 1.0.0," >&2
            echo "  so the FetchContent tag test cannot make a second version." >&2
            exit 1
        }
        sed -i.bak 's/VERSION 1\.0\.0/VERSION 1.1.0/' "$DEPREPO/CMakeLists.txt"
        rm -f "$DEPREPO/CMakeLists.txt.bak"
        depgit commit -q -am "mathlib 1.1.0"
        depgit tag v1.1.0

        DEP_OUT=""
        for TAG in v1.0.0 v1.1.0; do
            cmake -S exercises/deplab/consume-fetched -B "$DL/fetched-$TAG" \
                -DMATHLIB_REPO="file://$DEPREPO" -DMATHLIB_TAG="$TAG" > /dev/null
            cmake --build "$DL/fetched-$TAG" --config Debug > /dev/null
            APP=$(dep_app "$DL/fetched-$TAG")
            DEP_OUT="$DEP_OUT$("$APP")|"
        done

        # The fixture stays alive through the assertions below. Deleting it
        # here, which is what this line used to do, threw away the evidence for
        # the one claim this block exists to make before the claim was checked.
        #
        # Building once proves the mechanism runs. Only building twice proves
        # the TAG is what selected the version - which is the chapter's claim,
        # and the whole reason to pin to a tag rather than a branch.
        #
        # But "the two runs differ" is too weak to be that proof: a pin that
        # resolved to the WRONG commit - a tag off by one, the two commits
        # reordered, a Version() that grew a timestamp - differs too, and would
        # sail through. Each run has to name the version its own tag carries.
        # That subsumes the difference check, since 1.0.0 and 1.1.0 cannot both
        # match one string.
        DEP_A=${DEP_OUT%%|*}; DEP_B=${DEP_OUT#*|}; DEP_B=${DEP_B%%|*}
        for PAIR in "v1.0.0:$DEP_A" "v1.1.0:$DEP_B"; do
            TAG=${PAIR%%:*}; GOT=${PAIR#*:}
            case "$GOT" in
                *"${TAG#v}"*) ;;
                *)  echo "build_all.sh: built at GIT_TAG $TAG, but the app reported" >&2
                    echo "  \"$GOT\", which does not name ${TAG#v}. Ch 27 step 3 says the" >&2
                    echo "  tag selects the version; here it selected something else." >&2
                    exit 1 ;;
            esac
        done
        echo "  ok   fetched: v1.0.0 and v1.1.0 each report their own tag   [Ch 27]"
        rm -rf "$DEPREPO"; trap - EXIT
    elif [ "$REQUIRE_GIT" = 1 ]; then
        echo "build_all.sh: git cannot build and clone a file:// repository here," >&2
        echo "  and --require-git was given. The FetchContent path needs both, and" >&2
        echo "  it is the only check that proves the TAG selects the version." >&2
        echo "  Re-run the probe by hand: git init a directory, commit, then" >&2
        echo "  git clone file://<that directory>. If the clone is what fails," >&2
        echo "  look at protocol.file.allow (git config --get protocol.file.allow)." >&2
        exit 1
    else
        echo "  SKIPPED - no git that can clone file://, so the FetchContent"
        echo "            path cannot run (CI runs this for real)"
    fi

    # --- path 3: installed, found as a config package ---
    # The producing half of find_package: install(EXPORT) plus a generated
    # mathlibConfig.cmake. Nothing in the consuming project names a path -
    # CMAKE_PREFIX_PATH is how a consumer points at an SDK.
    cmake -S exercises/deplab/mathlib -B "$DL/mathlib-build" \
        -DCMAKE_INSTALL_PREFIX="$PWD/$DL/prefix" > /dev/null
    cmake --build "$DL/mathlib-build" --config Debug --target install > /dev/null
    cmake -S exercises/deplab/consume-installed -B "$DL/installed" \
        -DCMAKE_PREFIX_PATH="$PWD/$DL/prefix" > /dev/null
    cmake --build "$DL/installed" --config Debug > /dev/null
    APP=$(dep_app "$DL/installed")
    "$APP" > /dev/null
    echo "  ok   installed: find_package(mathlib CONFIG) -> $("$APP")"
elif [ "$REQUIRE_CMAKE" = 1 ]; then
    echo "build_all.sh: --require-cmake was given but cmake is not on PATH" >&2
    exit 1
else
    echo "  SKIPPED - cmake not installed (CI runs this for real)"
fi

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
    # The third configuration - Chapter 26's compile-time switch - is built
    # here so that all three databases are checked for existence at once.
    rm -rf "$CM-audit"
    cmake -S exercises/buildlab -B "$CM-audit" -DGREETER_AUDIT=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON > /dev/null
    cmake --build "$CM-audit" --config Debug > /dev/null
    GREET_AUDIT=$(greet_binary "$CM-audit")
    for db in "$CM/compile_commands.json" "$CM-asan/compile_commands.json" "$CM-audit/compile_commands.json"; do
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
    # The compile-time switch: Greeter.h has a member behind the define, so it
    # is PUBLIC on greeter and must reach main.cpp (the executable's TU) as
    # well as Greeter.cpp - the compile database is the only place that shows
    # it did - and the binary must print the audit line it compiles in.
    for tu in Greeter main; do
        if ! grep -q -- "-DGREETER_AUDIT=1.*$tu\.cpp" "$CM-audit/compile_commands.json"; then
            echo "build_all.sh: GREETER_AUDIT=ON did not put -DGREETER_AUDIT=1 on" >&2
            echo "  $tu.cpp - the option is not PUBLIC, or not wired at all." >&2
            exit 1
        fi
    done
    if grep -q -- '-DGREETER_AUDIT' "$CM/compile_commands.json"; then
        echo "build_all.sh: the default configuration carries GREETER_AUDIT, so" >&2
        echo "  it is not a switch." >&2
        exit 1
    fi
    # Captured first, then grepped: under pipefail a grep -q that exits at
    # its first match can SIGPIPE a writer that has more to say.
    AUDIT_OUT=$("$GREET_AUDIT")
    PLAIN_OUT=$("$GREET")
    if ! printf '%s\n' "$AUDIT_OUT" | grep -q '^\[audit\]'; then
        echo "build_all.sh: the GREETER_AUDIT build printed no [audit] line" >&2
        exit 1
    fi
    if printf '%s\n' "$PLAIN_OUT" | grep -q '^\[audit\]'; then
        echo "build_all.sh: the default build printed an [audit] line" >&2
        exit 1
    fi
    echo "  ok   exercises/buildlab/CMakeLists.txt (default, GREETER_SANITIZE=ON, GREETER_AUDIT=ON)"
elif [ "$REQUIRE_CMAKE" = 1 ]; then
    echo "build_all.sh: cmake not found, and --require-cmake was given" >&2
    exit 1
else
    echo "  SKIPPED - cmake not installed (CI runs this for real)"
fi

# Chapter 40's lab: three projects, in the order a plug-in shop builds them.
# The SDK drop is installed to a prefix (a header and an archive, no config
# package - the point), the plug-in is configured against that prefix through
# its hand-written find-module, the stand-in host is built against the same
# prefix, and the host loads the module - twice, the second time passing the
# shorter table an older host would, which the plug-in must refuse. Then the
# claim the chapter is built around: the module's export table holds
# Plugin_Entry and NOTHING else of the plug-in's or the SDK's - hidden
# visibility covers what the plug-in compiles, and the linker option covers
# the archive it links. "It loaded" proves neither, so nm reads the table
# back. Last, both binaries are built again with the sanitizer flags injected
# from outside the CMakeLists (the lab's files stay the chapter's) and run.
echo "== pluginlab cmake =="
if command -v cmake > /dev/null 2>&1; then
    PL=build/pluginlab
    rm -rf "$PL"
    cmake -S exercises/pluginlab/sdk -B "$PL/sdk" \
        -DCMAKE_INSTALL_PREFIX="$PWD/$PL/prefix" > /dev/null
    cmake --build "$PL/sdk" --config Debug > /dev/null
    cmake --install "$PL/sdk" --config Debug > /dev/null
    cmake -S exercises/pluginlab/plugin -B "$PL/plugin" \
        -DCMAKE_PREFIX_PATH="$PWD/$PL/prefix" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON > /dev/null
    cmake --build "$PL/plugin" --config Debug > /dev/null
    cmake -S exercises/pluginlab/host -B "$PL/host" \
        -DCMAKE_PREFIX_PATH="$PWD/$PL/prefix" > /dev/null
    cmake --build "$PL/host" --config Debug > /dev/null
    # Where the two binaries landed depends on the generator (Debug/ under a
    # multi-config one) and the platform (.so on Linux AND macOS - a MODULE is
    # never .dylib - or .dll), so look.
    find_module() { find "$1" -type f \( -name 'monitor.so' -o -name 'monitor.dll' \) | head -1; }
    find_host()   { find "$1" -type f \( -name host -o -name host.exe \) -perm -u+x | head -1; }
    MODULE=$(find_module "$PL/plugin")
    HOST=$(find_host "$PL/host")
    if [ -z "$MODULE" ] || [ -z "$HOST" ]; then
        echo "build_all.sh: pluginlab built but the module or the host is missing" >&2
        exit 1
    fi
    # Captured first, then grepped, with the exit status kept apart from the
    # text: under pipefail a host that printed the line and then failed, or
    # never found the entry point, would both read as "the line never arrived".
    HOST_RC=0
    HOST_OUT=$("$HOST" "$MODULE" 2>&1) || HOST_RC=$?
    if [ "$HOST_RC" != 0 ]; then
        echo "build_all.sh: the host exited $HOST_RC loading $MODULE:" >&2
        printf '%s\n' "$HOST_OUT" | sed 's/^/  /' >&2
        exit 1
    fi
    if ! printf '%s\n' "$HOST_OUT" | grep -q 'monitor loaded against hostsdk'; then
        echo "build_all.sh: the host loaded $MODULE but the plug-in's log line never arrived:" >&2
        printf '%s\n' "$HOST_OUT" | sed 's/^/  /' >&2
        exit 1
    fi
    # The other half of the chapter's step 4: a shorter table must be refused
    # (return -1, host exits 1), not read past.
    OLDER_RC=0
    OLDER_OUT=$("$HOST" "$MODULE" --older 2>&1) || OLDER_RC=$?
    if [ "$OLDER_RC" = 0 ] || ! printf '%s\n' "$OLDER_OUT" | grep -q 'plug-in returned -1'; then
        echo "build_all.sh: the plug-in did not refuse an older host's shorter table (exit $OLDER_RC):" >&2
        printf '%s\n' "$OLDER_OUT" | sed 's/^/  /' >&2
        exit 1
    fi
    # The compile database shows the visibility flag reached monitor.cpp...
    if [ ! -f "$PL/plugin/compile_commands.json" ]; then
        echo "build_all.sh: no $PL/plugin/compile_commands.json. Only the Makefile and" >&2
        echo "  Ninja generators write a compile database, and without one the" >&2
        echo "  visibility flag cannot be verified - re-run with CMAKE_GENERATOR" >&2
        echo "  unset, or set to Ninja. (CI uses the default, which writes one.)" >&2
        exit 1
    fi
    if ! grep -q -- '-fvisibility=hidden.*monitor\.cpp' "$PL/plugin/compile_commands.json"; then
        echo "build_all.sh: monitor.cpp was not compiled with -fvisibility=hidden" >&2
        exit 1
    fi
    # ...and the export table shows what that did and did not cover. Names,
    # not a count: on Linux nm -g also lists the C runtime's own _init and
    # friends, which are nobody's surface.
    if command -v nm > /dev/null 2>&1; then
        EXPORTS=$(nm -g --defined-only "$MODULE" 2>/dev/null | awk '{print $NF}')
        if [ "$(printf '%s\n' "$EXPORTS" | grep -c 'Plugin_Entry')" != 1 ]; then
            echo "build_all.sh: $MODULE does not export exactly one Plugin_Entry:" >&2
            printf '%s\n' "$EXPORTS" | sed 's/^/  /' >&2
            exit 1
        fi
        # HostSdk_VersionString is the archive's (the linker option's job);
        # Describe is the plug-in's own, external linkage on purpose (the
        # visibility preset's job on Linux, the export list's on macOS).
        if printf '%s\n' "$EXPORTS" | grep -q 'HostSdk_VersionString\|Describe'; then
            echo "build_all.sh: $MODULE exports a symbol that should be hidden - the" >&2
            echo "  SDK's helper or the plug-in's own function. Chapter 40's finding:" >&2
            printf '%s\n' "$EXPORTS" | sed 's/^/  /' >&2
            exit 1
        fi
        echo "  ok   exercises/pluginlab/: installed, built, loaded, refused an older host;"
        echo "       exports Plugin_Entry, hides HostSdk_VersionString and Describe"
    else
        echo "  ok   exercises/pluginlab/: installed, built, loaded, refused an older host (no nm here to read the exports)"
    fi
    # Once more under the canonical sanitizers, injected from outside: the
    # CMakeLists are the chapter's listings and carry no sanitizer option,
    # and CMAKE_<LANG>_FLAGS plus the linker-flags variables are how a build
    # is instrumented without editing it.
    SAN_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g"
    cmake -S exercises/pluginlab/plugin -B "$PL/plugin-san" \
        -DCMAKE_PREFIX_PATH="$PWD/$PL/prefix" -DCMAKE_CXX_FLAGS="$SAN_FLAGS" \
        -DCMAKE_MODULE_LINKER_FLAGS="$SAN_FLAGS" > /dev/null
    cmake --build "$PL/plugin-san" --config Debug > /dev/null
    cmake -S exercises/pluginlab/host -B "$PL/host-san" \
        -DCMAKE_PREFIX_PATH="$PWD/$PL/prefix" -DCMAKE_CXX_FLAGS="$SAN_FLAGS" \
        -DCMAKE_EXE_LINKER_FLAGS="$SAN_FLAGS" > /dev/null
    cmake --build "$PL/host-san" --config Debug > /dev/null
    MODULE_SAN=$(find_module "$PL/plugin-san")
    HOST_SAN=$(find_host "$PL/host-san")
    SAN_RC=0
    SAN_OUT=$(UBSAN_OPTIONS=halt_on_error=1 "$HOST_SAN" "$MODULE_SAN" 2>&1) || SAN_RC=$?
    if [ "$SAN_RC" != 0 ] || ! printf '%s\n' "$SAN_OUT" | grep -q 'monitor loaded against hostsdk'; then
        echo "build_all.sh: the sanitized host/module pair failed (exit $SAN_RC):" >&2
        printf '%s\n' "$SAN_OUT" | sed 's/^/  /' >&2
        exit 1
    fi
    echo "  ok   exercises/pluginlab/: sanitized build loaded clean"
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

# Chapter 8's chaining listing is std::expected, which is C++23 - the one
# file in the cookbook past the book's own C++17 pin. Same bargain as the
# sections above: a compiler that cannot build it is a fact about a laptop
# and a bug in CI, so --require-expected refuses to skip. The probe IS the
# translation unit: <expected> arrived one release before its and_then and
# transform did in both standard libraries (libstdc++ 12 before 13, libc++
# 16 before 17), so a one-line probe of the header would pass on a compiler
# that then cannot build the listing. The flags are the canonical set with
# one token changed, derived rather than retyped so they cannot drift.
echo "== cookbook expected (c++23) =="
FLAGS23=${FLAGS/-std=c++17/-std=c++23}
if $CXX $FLAGS23 exercises/cookbook/expected.cpp -o "$OUT/cb_expected" > "$OUT/expected_build.log" 2>&1; then
    UBSAN_OPTIONS=halt_on_error=1 "$OUT/cb_expected" > /dev/null
    echo "  ok   exercises/cookbook/expected.cpp under -std=c++23"
elif [ "$REQUIRE_EXPECTED" = 1 ]; then
    echo "build_all.sh: $CXX cannot build exercises/cookbook/expected.cpp under" >&2
    echo "  $FLAGS23, and --require-expected was given. The compiler said:" >&2
    sed 's/^/  /' "$OUT/expected_build.log" >&2
    exit 1
else
    echo "  SKIPPED - $CXX cannot build the C++23 listing here (CI runs this for real)"
fi

echo "ALL GREEN"
