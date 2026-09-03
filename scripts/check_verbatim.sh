#!/usr/bin/env bash
# The verbatim-sync check: code the book quotes must equal the code the repo
# ships, mechanically. The discipline was always "edit both sides in the same
# commit" (CLAUDE.md, CONTRIBUTING.md); this script exists because that
# discipline once failed silently — solutions/tracer.cpp drifted from Chapter
# 14's listing and nothing noticed until a review diffed them by hand.
#
# Four kinds of check, all substring containment on exact bytes:
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
#                    whole file; Appendix H takes that same both-directions
#                    rule against exercises/choosing/, which exists to
#                    assert the costs that appendix quotes - forward, every
#                    cpp fence on the page is in that directory; backward,
#                    each unit its banners name is on the page WHOLE, since
#                    that lab has no TASK card to carry the reverse the way
#                    bridgelab's does; and Appendix G must hold NO cpp fence at
#                    all - its recorded shape is lookup material with no
#                    C++ listings (ROADMAP item 16's delivered note)
#                    that same whole-unit table carries a page column, so
#                    it also serves Chapter 6 (three units of
#                    exercises/choosing/passing.cpp) and Chapter 8 (three
#                    of the cookbook's, from errors.cpp and expected.cpp)
#   4. generated   - a listing the book quotes that no lab commits, because
#                    the code half is a script that writes it to a temp
#                    directory: Chapter 27's ODR headers, generated and
#                    asserted by scripts/check_platform_claims.sh. Runs
#                    chapter -> script, per listing, since that script also
#                    generates code the chapter never shows
#
# Deliberately NOT checked: exercises/buildlab/CMakeLists.txt (assembled from
# snippets, comments added - its own banner says so), solutions/Buffer.h and
# solutions/buffer.cpp (Chapter 15 quotes the Buffer as ONE merged listing and
# narrates the header/TU split; the demo's -Wself-move pragma block is
# likewise acknowledged prose-side, not quoted), abilab engine.cpp (excerpted,
# not quoted in full), solutions/device_threaded_solution.cpp (pointed at,
# never quoted), and book/I-const.md (its five fences are teaching sketches,
# not quotations of exercises/constlab/ - the lab's own claim, that five
# violations are refused, is asserted by build_all.sh instead). Needs python3,
# same as CI.
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
    # Chapter 40 quotes its lab's build files whole, comments included - a
    # CMake file's comments are the listing, not a banner - and deplab's
    # install/export file, which Chapter 27 pointed at and never showed.
    ('book/40-cmake-for-the-plug-in.md',          'exercises/pluginlab/plugin/cmake/FindHostSDK.cmake'),
    ('book/40-cmake-for-the-plug-in.md',          'exercises/pluginlab/plugin/CMakeLists.txt'),
    ('book/40-cmake-for-the-plug-in.md',          'exercises/pluginlab/plugin/CMakePresets.json'),
    ('book/40-cmake-for-the-plug-in.md',          'exercises/pluginlab/plugin/monitor_export.h'),
    ('book/40-cmake-for-the-plug-in.md',          'exercises/pluginlab/plugin/monitor.cpp'),
    ('book/40-cmake-for-the-plug-in.md',          'exercises/deplab/mathlib/CMakeLists.txt'),
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

# Chapter 39 quotes exercises/interoplab/ by excerpt too, so it runs the same
# forward direction as Chapter 38: every cpp fence in the chapter must be
# byte-identical to something the lab commits. There is no reverse pass to
# run - this lab's TASK card holds no cpp fence (nothing here is broken on
# purpose), and Appendix H's whole-unit reverse cannot express a fence that
# is one bare declaration lifted out of a header. Sources only, not the
# card: a fence must be pinned to code build_all.sh actually compiles.
interop = ''.join(open(p).read() for p in sorted(glob.glob('exercises/interoplab/*.h')
                                                 + glob.glob('exercises/interoplab/*.cpp')))
ch39_fences = cpp_fences('book/39-the-round-trip-home.md')
for i, block in enumerate(ch39_fences, 1):
    if block.rstrip('\n') not in interop:
        first = block.strip().split('\n')[0]
        failures.append(f"book/39-the-round-trip-home.md cpp fence #{i} ({first!r}) is in no exercises/interoplab/ file")

# Appendix H quotes exercises/choosing/ by excerpt, so it takes the same
# both-directions rule as Chapter 38. Forward: every cpp fence on the page
# must be byte-identical to something the build actually compiles.
choosing = ''.join(open(p).read() for p in sorted(glob.glob('exercises/choosing/*.h')
                                                  + glob.glob('exercises/choosing/*.cpp')))
h_fences = cpp_fences('book/H-choosing.md')
for i, block in enumerate(h_fences, 1):
    if block.rstrip('\n') not in choosing:
        first = block.strip().split('\n')[0]
        failures.append(f"book/H-choosing.md cpp fence #{i} ({first!r}) is in no exercises/choosing/ file")

# Reverse: each unit the lab's banners promise is quoted must actually be on
# its page, WHOLE. Chapter 38's reverse direction is carried by its TASK
# card; exercises/choosing/ has no card (it is not an exercise), so the
# named units are the contract instead - which is what stops a lab function
# from growing a branch the page never shows. The table carries a page
# column because more than one page quotes a lab by whole unit: Appendix H
# and Chapter 6 for exercises/choosing/, Chapter 8 for the cookbook.
# Whole-unit containment is the stronger check in both directions at once -
# the fence cannot be truncated, and the lab unit cannot grow - which is
# why no chapter gets a forward-only substring pass of its own.
def whole_unit(path, opening):
    """The text from the line starting with `opening` to where its braces close."""
    lines = open(path).read().split('\n')
    for i, line in enumerate(lines):
        if line.startswith(opening):
            depth, out, seen = 0, [], False
            for body in lines[i:]:
                out.append(body)
                depth += body.count('{') - body.count('}')
                seen = seen or '{' in body
                if seen and depth <= 0:
                    return '\n'.join(out)
            break
    return None

H_PAGE = 'book/H-choosing.md'
CH6 = 'book/06-the-rule-of-five-and-move-semantics.md'
CH8 = 'book/08-error-handling.md'
UNITS = [
    (H_PAGE, 'exercises/choosing/counted.h',    'struct Counts {'),
    (H_PAGE, 'exercises/choosing/counted.h',    'inline Counts& Tally() {'),
    (H_PAGE, 'exercises/choosing/passing.cpp',  'class Widget {'),
    (H_PAGE, 'exercises/choosing/passing.cpp',  'Counted MakeTemporary()'),
    (H_PAGE, 'exercises/choosing/passing.cpp',  'Counted MakeNamed()'),
    (H_PAGE, 'exercises/choosing/passing.cpp',  'void TheSinkAllocatesWhereTheBorrowDoesNot()'),
    (H_PAGE, 'exercises/choosing/passing.cpp',  'void ReturningCostsNoCopy()'),
    (H_PAGE, 'exercises/choosing/storing.cpp',  'void GrowthRelocatesAndMovesEveryElement()'),
    (H_PAGE, 'exercises/choosing/storing.cpp',  'void BoxedElementsStandStillWhenTheVectorGrows()'),
    (H_PAGE, 'exercises/choosing/storing.cpp',  'void AClosedSetStoresByValueWithoutABase()'),
    (CH6,    'exercises/choosing/passing.cpp',  'Counted MakeNamedMoved()'),
    (CH6,    'exercises/choosing/passing.cpp',  'void MovingFromAConstObjectCopies()'),
    (CH6,    'exercises/choosing/passing.cpp',  'void ReturnStdMoveCostsTheMoveElisionRemoved()'),
    # Chapter 8's translation layer quotes Recipe 22's Result and load_config,
    # so Chapter 8, Appendix F and errors.cpp are one listing in three places.
    (CH8,    'exercises/cookbook/errors.cpp',   'template <class T, class E>'),
    (CH8,    'exercises/cookbook/errors.cpp',   'Result<Config, ConfigError> load_config('),
    (CH8,    'exercises/cookbook/expected.cpp', 'std::expected<int, ConfigError> channels_doubled('),
]
pages = {}
for page, path, opening in UNITS:
    text = pages.setdefault(page, open(page).read())
    unit = whole_unit(path, opening)
    if unit is None:
        failures.append(f"{path}: no unit starting {opening!r} (check_verbatim's own list is stale)")
    elif unit not in text:
        failures.append(f"{path}: {opening!r} is not quoted whole in {page}")

# Chapter 27's ODR diamond has no lab file to point at. It is an ill-formed
# program whose FAILURE is the lesson, so it is generated into a temp directory
# by scripts/check_platform_claims.sh rather than committed to a harness whose
# contract is that programs succeed - which makes that script the code half of
# a book<->code pair, and the only one that is not a file under exercises/ or
# solutions/. The direction is chapter -> script, per listing rather than per
# file: the script generates more than the chapter shows (the two .cpp that
# include these headers are its own), so whole-file containment the other way
# could never hold. One direction still catches drift from either side, because
# both halves are pinned to the same fixed pair of blocks.
GENERATED = [
    ('book/27-dependency-management.md', 'scripts/check_platform_claims.sh',
     ('// v1.h', '// v2.h')),
    # Chapter 26's macro-ODR header, the same arrangement: an ill-formed
    # program's one listing, generated and asserted by the same script.
    ('book/26-build-systems-and-cmake.md', 'scripts/check_platform_claims.sh',
     ('// session.h',)),
]
gen_pairs = 0
for chapter, script, openings in GENERATED:
    generator = open(script).read()
    by_first = {b.strip().split('\n')[0]: b for b in cpp_fences(chapter)}
    for opening in openings:
        gen_pairs += 1
        block = by_first.get(opening)
        if block is None:
            failures.append(f"{chapter}: no cpp fence starting {opening!r} "
                            "(check_verbatim's own list is stale)")
        elif block.rstrip('\n') not in generator:
            failures.append(f"{chapter}: the {opening!r} listing is not verbatim "
                            f"in {script}, which generates and asserts it")

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
      f"{len(ch38_fences)} ch38 fences, {len(ch39_fences)} ch39 fences, "
      f"{len(h_fences)} appH fences + "
      f"{len(UNITS)} whole units on {len(pages)} pages, {gen_pairs} generated, G cpp-free)")
PYEOF
