#!/usr/bin/env bash
# Build and run every reference solution under strict flags + sanitizers.
# This is the repo's core invariant; CI runs the same script.
set -euo pipefail
cd "$(dirname "$0")/.."
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

echo "== running =="
$OUT/tracer > /dev/null
$OUT/buffer > /dev/null
$OUT/fakesdk > /dev/null
$OUT/device > /dev/null
$OUT/words exercises/words_sample.txt > /dev/null
$OUT/shapes > /dev/null
$OUT/invalid > /dev/null
$OUT/lambdas > /dev/null
echo "ALL GREEN"
