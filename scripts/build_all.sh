#!/usr/bin/env bash
# Build and run every reference solution under strict flags + sanitizers.
# This is the repo's core invariant; CI runs the same script.
#
#   scripts/build_all.sh                   -> everything; cmake step may SKIP
#   scripts/build_all.sh --require-cmake   -> also fail if cmake is missing (CI)
set -euo pipefail
cd "$(dirname "$0")/.."

REQUIRE_CMAKE=0
while [ $# -gt 0 ]; do
    case "$1" in
        --require-cmake) REQUIRE_CMAKE=1; shift ;;
        *) echo "build_all.sh: unknown argument: $1" >&2; exit 2 ;;
    esac
done

CXX=${CXX:-g++}
FLAGS="-std=c++17 -Wall -Wextra -fsanitize=address,undefined -g"
# words.cpp uses C++20 (std::erase_if)
FLAGS20="-std=c++20 -Wall -Wextra -fsanitize=address,undefined -g"
OUT=$(mktemp -d)

run() { echo "== $1"; shift; "$@"; }

run "tracer"   $CXX $FLAGS   solutions/tracer.cpp                    -o $OUT/tracer
run "buffer"   $CXX $FLAGS   solutions/buffer.cpp                    -o $OUT/buffer
run "fakesdk"  $CXX $FLAGS   exercises/fakesdk/FakeSDK.cpp solutions/fakesdk_solution.cpp -I exercises/fakesdk -o $OUT/fakesdk
run "device"   $CXX $FLAGS   exercises/fakedevice/FakeDevice.cpp solutions/device_solution.cpp -I exercises/fakedevice -o $OUT/device
run "words"    $CXX $FLAGS20 solutions/words.cpp                     -o $OUT/words
run "shapes"   $CXX $FLAGS   solutions/shapes.cpp                    -o $OUT/shapes
run "invalid"  $CXX $FLAGS20 solutions/invalid.cpp                   -o $OUT/invalid
run "lambdas"  $CXX $FLAGS   solutions/lambdas.cpp                   -o $OUT/lambdas
# buildlab is exercise scaffolding, not a solution — but its starting point must stay green
run "buildlab" $CXX $FLAGS   exercises/buildlab/Greeter.cpp exercises/buildlab/main.cpp -o $OUT/buildlab

echo "== running =="
$OUT/tracer > /dev/null
$OUT/buffer > /dev/null
$OUT/fakesdk > /dev/null
$OUT/device > /dev/null
$OUT/words exercises/words/words_sample.txt > /dev/null
$OUT/shapes > /dev/null
$OUT/invalid > /dev/null
$OUT/lambdas > /dev/null
$OUT/buildlab > /dev/null

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

echo "ALL GREEN"
