#!/usr/bin/env bash
# Build the book as a static site with MkDocs Material, strictly.
#
# The site is a second rendering of book/ - the same files GitHub renders,
# untouched - so this script is also a check: `mkdocs build --strict` fails on
# a link to a missing file or anchor (the anchors are GitHub's, byte for byte,
# through pymdownx.slugs), on an include (`--8<--`) whose file or line range
# does not exist, and on any page the nav does not know. Output goes to
# build/site/ (gitignored). SITE-PLAN.md is why this exists and what comes next.
#
# Usage: scripts/build_site.sh [--serve]
#   --serve   run the live-reloading dev server instead of a one-shot build
set -euo pipefail
cd "$(dirname "$0")/.."

VENV=build/site-venv
if [ ! -x "$VENV/bin/mkdocs" ]; then
    python3 -m venv "$VENV"
    "$VENV/bin/pip" install --quiet --upgrade pip
    "$VENV/bin/pip" install --quiet -r scripts/site-requirements.txt
fi

if [ "${1:-}" = "--serve" ]; then
    exec "$VENV/bin/mkdocs" serve
fi
"$VENV/bin/mkdocs" build --strict
echo "site: build/site/index.html"
