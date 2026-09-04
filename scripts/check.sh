#!/usr/bin/env bash
# Build and run YOUR exercise attempt with the handbook's canonical flags.
#
#   scripts/check.sh <your.cpp> [more.cpp...] [fakesdk|fakedevice|comlab] [args passed to the run...]
#
#   scripts/check.sh attempt.cpp                                # plain exercise
#   scripts/check.sh attempt.cpp fakesdk                        # + vendor code
#   scripts/check.sh attempt.cpp comlab                         # + the 2.0 SDK (Ch 35)
#   scripts/check.sh registry.cpp main.cpp 100                  # several TUs + run args
#   STD=c++20 scripts/check.sh attempt.cpp words_sample.txt     # C++20 + run args
#   SAN=thread scripts/check.sh attempt.cpp fakedevice          # ThreadSanitizer
#   SAN=none scripts/check.sh a.cpp b.cpp                       # no sanitizer at all
#   SAN=none OPT=2 scripts/check.sh a.cpp b.cpp                 # ...and optimised
#
# Every leading argument ending in .cpp is a source file; they are compiled
# together IN THE ORDER GIVEN, which is also the link order - Chapter 32's
# two-order test depends on that. The first argument that is neither a .cpp
# file nor a vendor name starts the run args.
# Works from any directory: your files and run args resolve relative to where
# you run it (from an exercise directory: ../../scripts/check.sh your.cpp);
# the vendor code is found relative to this script.
#
# Env overrides: CXX (default g++), STD (default c++17),
# SAN (default address,undefined - the sanitizers, spelled as -fsanitize= takes
# them) and OPT (default 0, spelled as -O takes it).
#
# Threaded work (Chapter 29) needs SAN=thread as a SECOND run: TSan and ASan
# cannot be combined, and they answer different questions.
#
# SAN=none is the odd one, because the sanitizers are the point of this script
# everywhere else. It exists for the exercises whose whole subject is what the
# tools DO NOT catch - Chapter 27's ODR diamond is the case that forced it:
# step 5 says to link two definitions of one class and note that "nothing
# warned you at any point", which cannot be shown by a build that warns, and
# then to rebuild at -O2 and watch the symptom change. Step 6 turns the
# sanitizers back on and asks you to predict which link order ASan catches -
# so a script that could only build one way handed over that answer before the
# prediction was made. SAN=none plus OPT=2 is that pair of steps.
#
# An EMPTY SAN still gets the default rather than no sanitizer: silently
# dropping them because a variable did not expand is the wrong way round.
# Turning them off is a thing you say, not a thing that happens to you.
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "usage: check.sh <your.cpp> [more.cpp...] [fakesdk|fakedevice|comlab] [run args...]" >&2
    exit 2
fi

root="$(cd "$(dirname "$0")/.." && pwd)"
CXX=${CXX:-g++}
STD=${STD:-c++17}
SAN=${SAN:-address,undefined}
OPT=${OPT:-0}
# -O is always spelled out, including the -O0 that used to be implicit: it is
# what makes OPT=2 a visible switch in the "built clean" line below rather than
# a silent difference between two runs.
# -isystem exercises/third_party: the one vendored header (Appendix F's JSON
# recipes) is reachable from any attempt; -isystem keeps its warnings its own.
FLAGS="-std=$STD -Wall -Wextra -O$OPT -g -isystem $root/exercises/third_party"
if [[ $SAN != none ]]; then
    FLAGS="$FLAGS -fsanitize=$SAN"
fi

srcs=("$1")
shift
while [[ $# -gt 0 && $1 == *.cpp ]]; do
    srcs+=("$1")
    shift
done
sdk=""
case "${1:-}" in
    fakesdk|fakedevice|comlab) sdk=$1; shift ;;
esac

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
out="$tmp/attempt"
if [[ -n $sdk ]]; then
    $CXX $FLAGS "$root/exercises/$sdk"/Fake*.cpp "${srcs[@]}" -I "$root/exercises/$sdk" -o "$out"
else
    $CXX $FLAGS "${srcs[@]}" -o "$out"
fi
echo "== built clean: $FLAGS"

# halt_on_error makes UBSan findings fail the run (its default is report-and-continue).
# For SAN=thread it does something smaller: TSan fails the run by itself, so the
# flag only stops it at the first race instead of the last. Each variable is
# ignored when that sanitizer is not in the binary, so both can be set always.
UBSAN_OPTIONS=halt_on_error=1 TSAN_OPTIONS=halt_on_error=1 "$out" "$@"
if [[ $SAN == none ]]; then
    # Not "sanitizers quiet": there were none. Saying it anyway would hand the
    # reader the false reassurance that Chapter 27's step 5 exists to remove -
    # an exit 0 from an uninstrumented build is not evidence of anything.
    echo "== ran (exit 0) - NO sanitizer was built in, so this says nothing about UB"
else
    echo "== ran clean (exit 0, sanitizers quiet)"
fi
