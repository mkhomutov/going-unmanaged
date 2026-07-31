#!/usr/bin/env bash
# Build and run YOUR exercise attempt with the handbook's canonical flags.
#
#   scripts/check.sh <your.cpp> [fakesdk|fakedevice] [args passed to the run...]
#
#   scripts/check.sh attempt.cpp                                # plain exercise
#   scripts/check.sh attempt.cpp fakesdk                        # + vendor code
#   STD=c++20 scripts/check.sh attempt.cpp words_sample.txt     # C++20 + run args
#
# Env overrides: CXX (default g++), STD (default c++17).
set -euo pipefail
cd "$(dirname "$0")/.."

if [[ $# -lt 1 ]]; then
    echo "usage: scripts/check.sh <your.cpp> [fakesdk|fakedevice] [run args...]" >&2
    exit 2
fi

CXX=${CXX:-g++}
STD=${STD:-c++17}
FLAGS="-std=$STD -Wall -Wextra -fsanitize=address,undefined -g"

src=$1
shift
sdk=""
case "${1:-}" in
    fakesdk|fakedevice) sdk=$1; shift ;;
esac

out="$(mktemp -d)/attempt"
if [[ -n $sdk ]]; then
    $CXX $FLAGS "exercises/$sdk"/Fake*.cpp "$src" -I "exercises/$sdk" -o "$out"
else
    $CXX $FLAGS "$src" -o "$out"
fi
echo "== built clean: $FLAGS"

"$out" "$@"
echo "== ran clean (exit 0, sanitizers quiet)"
