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

# Chapter 26's CMakeLists, configured, built and run both ways. The chapter's
# snippets are code, and this is what holds them to the same standard as
# everything above: the reference file in exercises/buildlab/ is assembled
# from them, so a snippet that stopped working fails here.
#
# cmake is not part of the toolchain the rest of this script needs, so a
# laptop without it stays green and says so rather than pretending. CI passes
# --require-cmake, which refuses to skip - same bargain as check_mermaid.sh.
echo "== buildlab cmake =="
if command -v cmake > /dev/null 2>&1; then
    CM=build/buildlab-cmake            # under build/, which is gitignored
    rm -rf "$CM" "$CM-asan"
    cmake -S exercises/buildlab -B "$CM" > /dev/null
    cmake --build "$CM" > /dev/null
    "$CM/greet" > /dev/null
    # The GREETER_SANITIZE switch is only worth having if it still works.
    cmake -S exercises/buildlab -B "$CM-asan" \
        -DCMAKE_BUILD_TYPE=Debug -DGREETER_SANITIZE=ON > /dev/null
    cmake --build "$CM-asan" > /dev/null
    "$CM-asan/greet" > /dev/null
    echo "  ok   exercises/buildlab/CMakeLists.txt (default, and GREETER_SANITIZE=ON)"
elif [ "$REQUIRE_CMAKE" = 1 ]; then
    echo "build_all.sh: cmake not found, and --require-cmake was given" >&2
    exit 1
else
    echo "  SKIPPED - cmake not installed (CI runs this for real)"
fi

echo "ALL GREEN"
