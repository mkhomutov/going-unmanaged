#!/usr/bin/env bash
# The verbatim-sync check: code the book quotes must equal the code the repo
# ships, mechanically. The discipline was always "edit both sides in the same
# commit" (CLAUDE.md, CONTRIBUTING.md); this script exists because that
# discipline once failed silently — solutions/tracer.cpp drifted from Chapter
# 14's listing and nothing noticed until a review diffed them by hand.
#
# Three checks, all substring containment on exact bytes:
#   1. full        - the committed file appears verbatim inside its chapter
#                    (vendor headers, ticket-lab fixed files, solution folds)
#   2. banner      - same, after stripping the file's leading //-comment
#                    provenance banner (testlab/abilab convention: the banner
#                    names the sync rule and is not part of the quoted listing)
#   3. cookbook/tasks - every ```cpp fence in Appendix F appears in some
#                    exercises/cookbook/ TU, and every ```cpp fence in a
#                    ticket TASK card appears in its chapter (the broken
#                    listings are book-and-card, identical by rule);
#                    bridgelab's card holds the same rule, and its chapter
#                    is additionally checked the other way - every cpp
#                    fence in Chapter 38 must live in exercises/bridgelab/
#                    (committed code or the card's broken listings), since
#                    the chapter quotes the lab by excerpt rather than by
#                    whole file; and Appendix G must hold NO cpp fence at
#                    all - its recorded shape is lookup material with no
#                    C++ listings (ROADMAP item 16's delivered note)
#
# Deliberately NOT checked: exercises/buildlab/CMakeLists.txt (assembled from
# snippets, comments added - its own banner says so), solutions/Buffer.h and
# solutions/buffer.cpp (Chapter 15 quotes the Buffer as ONE merged listing and
# narrates the header/TU split; the demo's -Wself-move pragma block is
# likewise acknowledged prose-side, not quoted), abilab engine.cpp (excerpted,
# not quoted in full), and solutions/device_threaded_solution.cpp (pointed at,
# never quoted). Needs python3, same as CI.
set -euo pipefail
cd "$(dirname "$0")/.."

python3 - <<'PYEOF'
import glob, re, sys

failures = []

# The one fence shape every containment check below extracts. One definition,
# because a tweak applied to two of three copies is a check gone silently lax.
def cpp_fences(path):
    return re.findall(r'```cpp\n(.*?)```', open(path).read(), re.S)

def strip_banner(text):
    lines = text.split('\n')
    i = 0
    while i < len(lines) and (lines[i].startswith('//') or lines[i].strip() == ''):
        i += 1
    return '\n'.join(lines[i:])

def check(chapter, path, banner=False):
    hay = open(chapter).read()
    needle = open(path).read()
    if banner:
        needle = strip_banner(needle)
    if needle.rstrip('\n') not in hay:
        failures.append(f"{path} is not verbatim inside {chapter}"
                        + (" (after banner strip)" if banner else ""))

FULL = [
    ('book/14-exercise-the-lifetime-tracer.md',   'solutions/tracer.cpp'),
    ('book/17-exercise-the-fakesdk.md',           'exercises/fakesdk/FakeSDK.h'),
    ('book/17-exercise-the-fakesdk.md',           'solutions/fakesdk_solution.cpp'),
    ('book/18-exercise-the-device-sdk.md',        'exercises/fakedevice/FakeDevice.h'),
    ('book/18-exercise-the-device-sdk.md',        'solutions/device_solution.cpp'),
    ('book/19-exercise-the-word-counter.md',      'solutions/words.cpp'),
    ('book/20-exercise-slicing-and-polymorphism.md', 'solutions/shapes.cpp'),
    ('book/21-exercise-iterator-invalidation.md', 'solutions/invalid.cpp'),
    ('book/22-exercise-lambda-lifetimes.md',      'solutions/lambdas.cpp'),
    ('book/32-it-crashes-on-exit.md',             'exercises/exitlab/audit.cpp'),
    ('book/33-here-is-the-report.md',             'exercises/reportlab/registry.cpp'),
    ('book/33-here-is-the-report.md',             'exercises/reportlab/main.cpp'),
    ('book/34-parse-this-capture.md',             'exercises/capturelab/wire.h'),
    ('book/34-parse-this-capture.md',             'exercises/capturelab/wire.cpp'),
    ('book/35-still-live-at-unload.md',           'exercises/comlab/FakeSDK2.h'),
    ('book/35-still-live-at-unload.md',           'exercises/comlab/ref.h'),
    ('book/35-still-live-at-unload.md',           'exercises/comlab/main.cpp'),
    ('book/36-the-host-stutters.md',              'exercises/perflab/meter.h'),
    ('book/36-the-host-stutters.md',              'exercises/perflab/meter.cpp'),
    ('book/36-the-host-stutters.md',              'exercises/perflab/main.cpp'),
    ('book/37-no-repro-dump-attached.md',         'exercises/dumplab/session.h'),
    ('book/37-no-repro-dump-attached.md',         'exercises/dumplab/session.cpp'),
    ('book/37-no-repro-dump-attached.md',         'exercises/dumplab/main.cpp'),
]
BANNER = [
    ('book/28-testing.md',                  'exercises/testlab/tiny_test.h'),
    ('book/28-testing.md',                  'exercises/testlab/buffer_test.cpp'),
    ('book/30-authoring-an-abi-boundary.md', 'exercises/abilab/Widget.h'),
    ('book/30-authoring-an-abi-boundary.md', 'exercises/abilab/Widget.cpp'),
    ('book/30-authoring-an-abi-boundary.md', 'exercises/abilab/IScorer.h'),
    ('book/30-authoring-an-abi-boundary.md', 'exercises/abilab/engine.h'),
]

for ch, f in FULL:
    check(ch, f)
for ch, f in BANNER:
    check(ch, f, banner=True)

# Appendix F: every cpp fence is a recipe listing and must live in a cookbook TU.
cookbook = ''.join(open(p).read() for p in sorted(glob.glob('exercises/cookbook/*.cpp')))
f_blocks = cpp_fences('book/F-rosetta-cookbook.md')
for i, block in enumerate(f_blocks, 1):
    if block.rstrip('\n') not in cookbook:
        first = block.strip().split('\n')[0]
        failures.append(f"Appendix F cpp fence #{i} ({first!r}) is in no exercises/cookbook/ TU")

# Ticket TASK cards: the broken listings are quoted in both places, identically.
TICKETS = [('exitlab', '32-it-crashes-on-exit'),
           ('reportlab', '33-here-is-the-report'),
           ('capturelab', '34-parse-this-capture'),
           ('comlab', '35-still-live-at-unload'),
           ('perflab', '36-the-host-stutters'),
           ('dumplab', '37-no-repro-dump-attached'),
           # A lab card rather than a ticket card, but the same rule: the
           # broken listings are quoted in both places, identically.
           ('bridgelab', '38-the-bridge-out')]
for lab, ch in TICKETS:
    chapter = open(f'book/{ch}.md').read()
    for i, block in enumerate(cpp_fences(f'exercises/{lab}/TASK.md'), 1):
        if block.rstrip('\n') not in chapter:
            failures.append(f"exercises/{lab}/TASK.md cpp fence #{i} is not in book/{ch}.md")

# Chapter 38's fences run the other direction too: every cpp fence in the
# chapter must live in exercises/bridgelab/ - as committed code (the lab is
# quoted by excerpt, so full-file containment does not apply) or as one of
# the TASK card's broken listings, which the loop above pinned to the
# chapter. Nothing in the chapter is quoted from nowhere.
# Text sources only: a learner's stray a.out (or a scratch subdirectory)
# in the lab must not crash the check with a decode error instead of a verdict.
bridge = ''.join(open(p).read() for p in sorted(glob.glob('exercises/bridgelab/*.h')
                                                + glob.glob('exercises/bridgelab/*.cpp')
                                                + glob.glob('exercises/bridgelab/*.md')))
ch38_fences = cpp_fences('book/38-the-bridge-out.md')
for i, block in enumerate(ch38_fences, 1):
    if block.rstrip('\n') not in bridge:
        first = block.strip().split('\n')[0]
        failures.append(f"book/38-the-bridge-out.md cpp fence #{i} ({first!r}) is in no exercises/bridgelab/ file")

# Appendix G holds the opposite contract: no cpp fence at all (ROADMAP item
# 16's shape decision - a page with nothing to compile owes build_all.sh
# nothing). The day one lands it becomes the book's only unverified listing.
g_fences = cpp_fences('book/G-the-bridge-catalogue.md')
if g_fences:
    failures.append(f"book/G-the-bridge-catalogue.md holds {len(g_fences)} cpp fence(s); "
                    "its contract is no C++ listings (ROADMAP item 16's delivered note)")

if failures:
    print("check_verbatim.sh: DRIFT", file=sys.stderr)
    for f in failures:
        print(f"  {f}", file=sys.stderr)
    sys.exit(1)
print(f"verbatim OK ({len(FULL)} full, {len(BANNER)} banner-stripped, "
      f"{len(f_blocks)} cookbook fences, {len(TICKETS)} cards, "
      f"{len(ch38_fences)} ch38 fences, G cpp-free)")
PYEOF
