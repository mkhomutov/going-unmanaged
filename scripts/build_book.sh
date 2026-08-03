#!/usr/bin/env bash
# Rebuild the complete single-file book from the per-chapter files in book/.
#
# The per-chapter files are canonical; the single file is a build artifact
# (a release download), not something checked in. This script also owns the
# prev/next navigation footers, so they can never drift from the file order.
#
#   scripts/build_book.sh                 -> build/going-unmanaged.md
#   scripts/build_book.sh -o out.md       -> a path of your choosing ("-" = stdout)
#   scripts/build_book.sh --write-nav     -> regenerate the nav footers in book/
#   scripts/build_book.sh --check-nav     -> fail if any nav footer is stale (CI)
set -euo pipefail
cd "$(dirname "$0")/.."

BOOK=book
OUT=build/going-unmanaged.md
MODE=build

while [ $# -gt 0 ]; do
    case "$1" in
        -o|--output) [ $# -ge 2 ] || { echo "build_book.sh: $1 needs a path" >&2; exit 2; }
                     OUT="$2"; shift 2 ;;
        --write-nav) MODE=write-nav; shift ;;
        --check-nav) MODE=check-nav; shift ;;
        # The usage block above, down to the last line of it (keep in step).
        -h|--help)   sed -n '2,11p' "$0"; exit 0 ;;
        *) echo "build_book.sh: unknown argument: $1" >&2; exit 2 ;;
    esac
done

# Reading order: chapters 01-NN, then appendices A-D. Digits sort before
# letters in the C locale, so the glob order IS the reading order.
export LC_ALL=C
FILES=("$BOOK"/[0-9][0-9]-*.md "$BOOK"/[A-Z]-*.md)
[ -e "${FILES[0]}" ] || { echo "build_book.sh: no chapter files in $BOOK/" >&2; exit 1; }

# The chapter/appendix title, taken from the file's own H2 heading.
title_of() { sed -n 's/^## //p' "$1" | head -1; }

# The nav footer for FILES[$1], wrapped in the markers build_book strips.
nav_block() {
    local i=$1 prev next
    prev=""; next=""
    [ "$i" -gt 0 ] && prev=$((i - 1))
    [ "$i" -lt $(( ${#FILES[@]} - 1 )) ] && next=$((i + 1))
    echo "<!-- nav:begin -->"
    [ -n "$prev" ] && printf '[← %s](%s) · ' "$(title_of "${FILES[$prev]}")" "$(basename "${FILES[$prev]}")"
    printf '[Contents](README.md)'
    [ -n "$next" ] && printf ' · [%s →](%s)' "$(title_of "${FILES[$next]}")" "$(basename "${FILES[$next]}")"
    printf '\n'
    echo "<!-- nav:end -->"
}

# Everything in a chapter file except its nav footer (and the blank line that
# separates the footer from the text). This is what the single file gets.
strip_nav() {
    awk -v f="$1" '
        { line[NR] = $0 }
        END {
            navs = 0
            for (i = 1; i <= NR; i++)
                if (line[i] == "<!-- nav:begin -->") navs++
            last = NR
            while (last > 0 && line[last] == "") last--
            n = NR
            if (navs > 0) {
                # A marker that is not the trailing footer is ambiguous: cutting
                # there would lose text, and appending would leave two footers.
                # Say so instead of quietly doing either.
                if (line[last] != "<!-- nav:end -->") {
                    printf "build_book.sh: %s: <!-- nav:begin --> found, but the file does not end with the footer\n", f > "/dev/stderr"
                    exit 3
                }
                # Scan back from the end, so a chapter that quotes the markers
                # in its own prose keeps that text.
                for (i = last; i >= 1; i--)
                    if (line[i] == "<!-- nav:begin -->") { n = (line[i-1] == "") ? i - 2 : i - 1; break }
            }
            for (i = 1; i <= n; i++) print line[i]
        }' "$1"
}

# Relative file links go back to plain in-page anchors: the single file has
# every chapter in it, so ](26-build-systems-and-cmake.md#anchor) -> ](#anchor).
# Pinned to the NN-/A-D- chapter filenames on purpose: a link to a file OUTSIDE
# book/ (../CONTRIBUTING.md#..., an external .md URL) must keep its path.
inline_links() { sed -E 's%\]\(([0-9]{2}|[A-Z])-[^)#]*\.md#%](#%g'; }

case "$MODE" in
write-nav|check-nav)
    stale=0
    for i in "${!FILES[@]}"; do
        f="${FILES[$i]}"
        tmp=$(mktemp)
        { strip_nav "$f"; echo; nav_block "$i"; } > "$tmp"
        if cmp -s "$tmp" "$f"; then
            rm -f "$tmp"
        elif [ "$MODE" = write-nav ]; then
            # Copy the contents rather than mv the temp file over the top:
            # mktemp makes it 0600, and mv would carry that onto the chapter.
            cat "$tmp" > "$f"
            rm -f "$tmp"
            echo "nav updated: $f"
        else
            rm -f "$tmp"
            echo "stale nav footer: $f" >&2
            stale=1
        fi
    done
    if [ "$stale" -ne 0 ]; then
        echo "run scripts/build_book.sh --write-nav and commit the result" >&2
        exit 1
    fi
    echo "nav footers OK (${#FILES[@]} files)"
    ;;
build)
    emit() {
        cat "$BOOK/README.md"
        for f in "${FILES[@]}"; do strip_nav "$f"; done
    }
    if [ "$OUT" = "-" ]; then
        emit | inline_links
    else
        mkdir -p "$(dirname "$OUT")"
        emit | inline_links > "$OUT"
        echo "wrote $OUT ($(wc -l < "$OUT" | tr -d ' ') lines, ${#FILES[@]} chapter files)"
    fi
    ;;
esac
