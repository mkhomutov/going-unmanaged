#!/usr/bin/env bash
# Render every mermaid diagram in book/ and fail if one of them does not.
#
# This is the half check_markup.sh cannot do. That script checks the shape of
# a fence; this one checks what is inside it, by handing each chapter to
# mermaid-cli and letting a real mermaid draw every block to SVG. A diagram
# with a syntax error otherwise merges green and renders as an error box on
# GitHub, where the reader finds it.
#
# Rendering, not just parsing: mermaid.parse() proves the grammar is legal,
# and a diagram can parse and still throw on the way to a picture.
#
# What it still cannot tell you is whether the picture is any GOOD - crossed
# edges, a decision tree stacked into a column, a diagram too wide to read.
# Look at every new diagram in a browser; see CLAUDE.md.
#
#   scripts/check_mermaid.sh              -> book/*.md
#   scripts/check_mermaid.sh --required   -> also fail if mmdc is missing (CI)
#   scripts/check_mermaid.sh FILE...      -> just those files
set -euo pipefail
cd "$(dirname "$0")/.."

REQUIRED=0
FILES=()
while [ $# -gt 0 ]; do
    case "$1" in
        --required) REQUIRED=1; shift ;;
        -*) echo "check_mermaid.sh: unknown argument: $1" >&2; exit 2 ;;
        *) FILES+=("$1"); shift ;;
    esac
done
[ ${#FILES[@]} -gt 0 ] || FILES=(book/*.md)

# Prefer a local install, then a global one. Deliberately no npx fallback:
# npx would silently download a different version mid-run.
MMDC=""
for candidate in ./node_modules/.bin/mmdc mmdc; do
    if command -v "$candidate" >/dev/null 2>&1; then MMDC="$candidate"; break; fi
done
if [ -z "$MMDC" ]; then
    if [ "$REQUIRED" = 1 ]; then
        echo "check_mermaid.sh: mmdc not found, and --required was given" >&2
        echo "  npm install -g @mermaid-js/mermaid-cli" >&2
        exit 1
    fi
    # A local run without Node should not look like a pass.
    echo "check_mermaid.sh: SKIPPED - mmdc not installed (CI runs this for real)"
    echo "  npm install -g @mermaid-js/mermaid-cli   # to run it here"
    exit 0
fi

# Only files that actually contain a diagram; mermaid-cli launches a browser
# per invocation, so handing it the other 30 chapters costs minutes for nothing.
WITH_DIAGRAMS=()
for f in "${FILES[@]}"; do
    grep -q '^```mermaid$' "$f" && WITH_DIAGRAMS+=("$f")
done
if [ ${#WITH_DIAGRAMS[@]} -eq 0 ]; then
    echo "mermaid OK (no diagrams found)"
    exit 0
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
# --no-sandbox: Chrome's sandbox is unavailable in most CI containers, and
# the input here is our own repository.
printf '{"args":["--no-sandbox"]}\n' > "$TMP/puppeteer.json"

rc=0
count=0
for f in "${WITH_DIAGRAMS[@]}"; do
    n=$(grep -c '^```mermaid$' "$f")
    # Markdown mode renders every block in the file in one browser launch.
    # The rewritten markdown is throwaway; the exit code is the whole point.
    if out=$("$MMDC" -p "$TMP/puppeteer.json" -i "$f" -o "$TMP/out.md" 2>&1); then
        printf '  ok   %-45s %s diagram(s)\n' "$f" "$n"
        count=$((count + n))
    else
        printf '  FAIL %-45s %s diagram(s)\n' "$f" "$n"
        echo "$out" | sed 's/^/       /'
        rc=1
    fi
done

if [ "$rc" != 0 ]; then
    echo "check_mermaid.sh: FAILED - a diagram does not render" >&2
    exit 1
fi
echo "mermaid OK ($count diagrams in ${#WITH_DIAGRAMS[@]} files)"
