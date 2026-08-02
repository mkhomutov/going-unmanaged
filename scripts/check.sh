#!/usr/bin/env bash
# Build and run YOUR exercise attempt with the handbook's canonical flags.
#
#   scripts/check.sh <your.cpp> [fakesdk|fakedevice] [args passed to the run...]
#
#   scripts/check.sh attempt.cpp                                # plain exercise
#   scripts/check.sh attempt.cpp fakesdk                        # + vendor code
#   STD=c++20 scripts/check.sh attempt.cpp words_sample.txt     # C++20 + run args
#   SAN=thread scripts/check.sh attempt.cpp fakedevice          # ThreadSanitizer
#
# Works from any directory: your file and run args resolve relative to where
# you run it (from an exercise directory: ../../scripts/check.sh your.cpp);
# the vendor code is found relative to this script.
# Env overrides: CXX (default g++), STD (default c++17),
# SAN (default address,undefined - the sanitizers, spelled as -fsanitize= takes
# them). Threaded work (Chapter 29) needs SAN=thread as a SECOND run: TSan and
# ASan cannot be combined, and they answer different questions.
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "usage: check.sh <your.cpp> [fakesdk|fakedevice] [run args...]" >&2
    exit 2
fi

root="$(cd "$(dirname "$0")/.." && pwd)"
CXX=${CXX:-g++}
STD=${STD:-c++17}
SAN=${SAN:-address,undefined}
FLAGS="-std=$STD -Wall -Wextra -fsanitize=$SAN -g"

src=$1
shift
sdk=""
case "${1:-}" in
    fakesdk|fakedevice) sdk=$1; shift ;;
esac

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
out="$tmp/attempt"
if [[ -n $sdk ]]; then
    $CXX $FLAGS "$root/exercises/$sdk"/Fake*.cpp "$src" -I "$root/exercises/$sdk" -o "$out"
else
    $CXX $FLAGS "$src" -o "$out"
fi
echo "== built clean: $FLAGS"

# halt_on_error makes UBSan findings fail the run (its default is report-and-continue).
# For SAN=thread it does something smaller: TSan fails the run by itself, so the
# flag only stops it at the first race instead of the last. Each variable is
# ignored when that sanitizer is not in the binary, so both can be set always.
UBSAN_OPTIONS=halt_on_error=1 TSAN_OPTIONS=halt_on_error=1 "$out" "$@"
echo "== ran clean (exit 0, sanitizers quiet)"
