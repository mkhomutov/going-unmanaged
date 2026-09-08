#!/usr/bin/env bash
# The verbatim-sync check: code the book shows must be the code the repo
# ships, mechanically. The discipline was once "edit both sides in the same
# commit", and this script existed because that discipline failed silently
# (solutions/tracer.cpp drifted from Chapter 14's listing and nothing noticed
# until a review diffed them by hand). Since SITE-PLAN step 4 the pages do not
# carry the code at all: a listing is `--8<-- "path:section"` inside a fence,
# the section fenced in the source by `--8<-- [start:section]` and
# `[end:section]` comment lines, and the site build renders the file. Drift
# is impossible by construction; what this script checks is everything around
# that construction:
#
#   1. includes, forward  - every include names a file that exists and a
#                           section that file marks exactly once (the strict
#                           site build catches a missing file or section too;
#                           this keeps the verdict in one script and catches
#                           a marker duplicated by a paste, which the snippet
#                           engine would resolve to the first)
#   2. includes, reverse  - every section a source marks is included by some
#                           page: a marked unit is a promise that a page shows
#                           it (the old whole-unit rule, generalized)
#   3. no copies          - no cpp, cmake or rust fence of four lines or more on a
#                           page is byte-identical to a region of a source
#                           file: a listing pasted back into a page instead
#                           of included is the drift this script was born
#                           for, refused before it can happen
#   4. cards              - the ticket and bridgelab TASK cards' broken
#                           listings appear in their chapters verbatim: those
#                           are book-and-card code with no compiled source
#                           (they exist to fail), so they stay copied and are
#                           held by containment
#   5. pinned lines       - two one-line quotations, an if-statement in
#                           Chapter 39 and a set() in Chapter 40, too short
#                           for a marker, held by containment
#   6. page shapes        - Appendix F holds no cpp or rust fence with code
#                           (every recipe, in both languages, is an include),
#                           Appendix G holds no cpp fence and Appendix J no
#                           cpp fence and at least one cmake fence, every one
#                           of them an include: a page with nothing to compile
#                           owes build_all.sh nothing, and a page with a
#                           listing owes it exactly that listing
#
# Deliberately NOT checked: exercises/buildlab/CMakeLists.txt (assembled from
# snippets, comments added - its own banner says so), solutions/Buffer.h and
# solutions/buffer.cpp (Chapter 15 quotes the Buffer as ONE merged listing and
# narrates the header/TU split), abilab engine.cpp (excerpted in prose, not
# quoted), solutions/device_threaded_solution.cpp (pointed at, never quoted),
# and book/I-const.md (its five fences are teaching sketches, not quotations
# of exercises/constlab/ - the lab's own claim is asserted by build_all.sh).
# Needs python3, same as CI.
set -euo pipefail
cd "$(dirname "$0")/.."

python3 - <<'PYEOF'
import glob, os, re, sys

failures = []

INCLUDE = re.compile(r'^[ \t]*--8<-- "([^":]+)(?::([^"]+))?"\s*$', re.M)   # indented inside a tab, or not
MARK = re.compile(r'--8<-- \[(start|end):([A-Za-z0-9_.-]+)\]')
FENCE = re.compile(r'```(cpp|cmake|rust)\n(.*?)```', re.S)

def fences(path, lang='cpp'):
    return [b for l, b in FENCE.findall(open(path).read()) if l == lang]

SOURCE_GLOBS = ('exercises/**/*.h', 'exercises/**/*.cpp', 'exercises/**/*.rs', 'exercises/**/*.cmake',
                'exercises/**/CMakeLists.txt', 'exercises/**/CMakePresets.json',
                'solutions/*.h', 'solutions/*.cpp',
                'scripts/check_platform_claims.sh', 'scripts/build_all.sh')
sources = {}
for pattern in SOURCE_GLOBS:
    for path in sorted(glob.glob(pattern, recursive=True)):
        if 'third_party' in path or '/build/' in path:
            continue
        sources[path] = open(path, errors='replace').read()

# 1. includes, forward
includes = []
pages = sorted(glob.glob('book/*.md'))
for page in pages:
    for path, section in INCLUDE.findall(open(page).read()):
        includes.append((page, path, section))
        if not os.path.exists(path):
            failures.append(f"{page} includes {path}, which does not exist")
            continue
        if section:
            marks = MARK.findall(open(path, errors='replace').read())
            starts = sum(1 for k, n in marks if k == 'start' and n == section)
            ends = sum(1 for k, n in marks if k == 'end' and n == section)
            if (starts, ends) != (1, 1):
                failures.append(f"{page} includes {path}:{section}, marked {starts} start(s) and {ends} end(s) there (want exactly one each)")

# 2. includes, reverse
referenced = {(path, section) for _, path, section in includes if section}
whole_files = {path for _, path, section in includes if not section}
marked_files = 0
for path, text in sources.items():
    marks = MARK.findall(text)
    if not marks:
        continue
    marked_files += 1
    for kind, name in marks:
        if kind == 'start' and (path, name) not in referenced:
            failures.append(f"{path} marks section {name!r}, which no page under book/ includes")

# 3. no copies
copied = 0
for page in pages:
    for lang, block in FENCE.findall(open(page).read()):
        body = block.rstrip('\n')
        if INCLUDE.match(body.strip()) or sum(1 for l in body.split('\n') if l.strip()) < 4:
            continue
        for path, text in sources.items():
            if body in text:
                copied += 1
                first = body.strip().split('\n')[0]
                failures.append(f"{page}: a {lang} fence ({first!r}) is a copy of {path}; include it by a marked section instead")
                break

# 4. cards
TICKETS = [('exitlab', '32-it-crashes-on-exit'),
           ('reportlab', '33-here-is-the-report'),
           ('capturelab', '34-parse-this-capture'),
           ('comlab', '35-still-live-at-unload'),
           ('perflab', '36-the-host-stutters'),
           ('dumplab', '37-no-repro-dump-attached'),
           ('bridgelab', '38-the-bridge-out')]
card_fences = 0
for lab, ch in TICKETS:
    chapter = open(f'book/{ch}.md').read()
    for i, block in enumerate(fences(f'exercises/{lab}/TASK.md'), 1):
        card_fences += 1
        if block.rstrip('\n') not in chapter:
            failures.append(f"exercises/{lab}/TASK.md cpp fence #{i} is not in book/{ch}.md")

# 5. pinned lines
PINNED = [
    ('book/39-the-round-trip-home.md', 'exercises/interoplab/plugin.cpp',
     'if (options->size != sizeof(PluginOptions)) return PLUGIN_VERSION_MISMATCH;'),
    ('book/40-cmake-for-the-plug-in.md', 'exercises/pluginlab/plugin/CMakeLists.txt',
     'set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")'),
]
for page, path, line in PINNED:
    if line not in open(page).read():
        failures.append(f"{page}: the pinned line {line[:40]!r}... is no longer on the page (check_verbatim's own list is stale)")
    elif line not in sources.get(path, ''):
        failures.append(f"{path}: the line {page} quotes, {line[:40]!r}..., is no longer in the file")

# 6. page shapes
f_code = [b for l in ('cpp', 'rust') for b in fences('book/F-rosetta-cookbook.md', l) if not INCLUDE.match(b.strip())]
if f_code:
    failures.append(f"book/F-rosetta-cookbook.md holds {len(f_code)} cpp/rust fence(s) with code; every recipe, in both languages, is included from exercises/cookbook/")
if fences('book/G-the-bridge-catalogue.md'):
    failures.append("book/G-the-bridge-catalogue.md holds a cpp fence; its contract is no C++ listings (ROADMAP item 16's delivered note)")
if fences('book/J-cmake-catalogue.md'):
    failures.append("book/J-cmake-catalogue.md holds a cpp fence; its contract is no C++ listings")
j_cmake = fences('book/J-cmake-catalogue.md', 'cmake')
if not j_cmake:
    failures.append("book/J-cmake-catalogue.md holds no cmake fence; its two checked projects are missing")
for block in j_cmake:
    if not INCLUDE.match(block.strip()):
        failures.append("book/J-cmake-catalogue.md holds a cmake fence with code; both of its listings are included from checked projects")

if failures:
    print("check_verbatim.sh: DRIFT", file=sys.stderr)
    for f in failures:
        print(f"  {f}", file=sys.stderr)
    sys.exit(1)
print(f"verbatim OK ({len(includes)} includes - {len(whole_files)} whole files, "
      f"{len(referenced)} sections in {marked_files} marked files - no copied fence, "
      f"{card_fences} card fences in {len(TICKETS)} chapters, {len(PINNED)} pinned lines, "
      f"G cpp-free, J cmake-only)")
PYEOF
