#!/usr/bin/env bash
# Hold the site's search to scripts/search_queries.tsv.
#
# The site is meant to be consulted daily rather than read once (SITE-PLAN.md),
# which makes retrieval load-bearing - and it was the one part of the build
# nothing checked. `mkdocs build --strict` proves every link and anchor
# resolves and says nothing about whether a reader can find the page at all,
# so a search that quietly stops answering a query stays broken until somebody
# gives up looking. This script closes that: it asks the published index the
# questions a reader asks and fails when the answer is not near the top.
#
# Two halves, because they need different things:
#
#   1. The fixture's targets exist. Every expected location in the queries
#      file must name a page or a section the index actually contains - a
#      heading renamed in book/ silently rots an entry otherwise. Pure Python,
#      always runs.
#   2. The ranking, by running the site's own search worker - the same file
#      the browser loads, over the same index - and asking it each query.
#      Needs Node, and nothing else.
#
# Run scripts/build_site.sh first; this reads what that produced.
#
#   scripts/check_search.sh                -> skips the ranking without Node
#   scripts/check_search.sh --require-node -> refuse to skip it (CI)
set -euo pipefail
cd "$(dirname "$0")/.."

REQUIRE=0
[ "${1:-}" = "--require-node" ] && REQUIRE=1

INDEX=build/site/search/search_index.json
QUERIES=scripts/search_queries.tsv
if [ ! -f "$INDEX" ]; then
    echo "check_search.sh: $INDEX not found - run scripts/build_site.sh first" >&2
    exit 1
fi

# --- 1. every expectation names something that exists --------------------
python3 - "$INDEX" "$QUERIES" <<'PY'
import json, sys

index, queries = sys.argv[1], sys.argv[2]
locations = [doc["location"] for doc in json.load(open(index, encoding="utf-8"))["docs"]]
bad = 0
total = 0
for number, line in enumerate(open(queries, encoding="utf-8"), 1):
    if not line.strip() or line.startswith("#"):
        continue
    parts = [p.strip() for p in line.split("\t")]
    if len(parts) < 2 or not parts[0] or not parts[1]:
        print(f"  line {number}: needs a query and an expected location, tab-separated")
        bad += 1
        continue
    total += 1
    if not any(loc.startswith(parts[1]) for loc in locations):
        print(f"  line {number}: no page or section starts with {parts[1]!r}"
              f" (query {parts[0]!r}) - renamed heading?")
        bad += 1
if bad:
    print(f"check_search.sh: FAILED - {bad} expectation(s) name nothing in the index")
    sys.exit(1)
print(f"search targets OK ({total} queries, every expectation resolves)")
PY

# --- 2. the ranking ------------------------------------------------------
if ! command -v node >/dev/null 2>&1; then
    if [ "$REQUIRE" = 1 ]; then
        echo "check_search.sh: node not found, and --require-node was given" >&2
        exit 1
    fi
    # A local run without Node should not look like a pass.
    echo "check_search.sh: SKIPPED the ranking - node not installed (CI runs it for real)"
    exit 0
fi

node scripts/search_rank.js build/site "$QUERIES" 3
