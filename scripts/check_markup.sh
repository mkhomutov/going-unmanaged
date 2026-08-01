#!/usr/bin/env bash
# Enforce the GitHub-native markup conventions from CLAUDE.md.
#
# Both callout alerts and mermaid diagrams render correctly on GitHub only in
# a shape that nothing else checks: an alert nested inside a list or a
# <details> fold silently renders as a plain blockquote, and a mermaid fence
# that is indented or missing its preceding blank line silently renders as a
# code block. Neither breaks the build, neither breaks a link, and both look
# fine in a diff — so this is the only pass that catches them.
#
# What it does NOT check is mermaid GRAMMAR: a diagram with a syntax error
# still merges green here and renders as an error box on GitHub. That needs
# a real mermaid parse (mmdc), and a Node dependency this job does not have.
# Render new diagrams in a browser before committing; see CLAUDE.md.
#
#   scripts/check_markup.sh                 -> book/*.md and the built single file
#   scripts/check_markup.sh FILE...         -> just those files
set -euo pipefail
cd "$(dirname "$0")/.."

if [ $# -gt 0 ]; then
    FILES=("$@")
else
    FILES=(book/*.md)
    # The built single file is derived, so checking it too is what catches a
    # build_book.sh change that mangles a fence on its way through.
    [ -f build/going-unmanaged.md ] && FILES+=(build/going-unmanaged.md)
fi

awk '
FNR == 1 { fence = ""; details = 0 }

# Track fenced code blocks first: everything inside one is literal text, and
# a book about markdown may well quote these very markers.
/^[ \t]*```/ {
    line = $0
    sub(/^[ \t]*/, "", line)
    if (fence == "") {
        fence = line
        # An opening mermaid fence is the thing being checked, so fall through.
        if (line !~ /^```mermaid[ \t]*$/) next
    } else {
        fence = ""          # closing fence
        next
    }
}
fence != "" && $0 !~ /^[ \t]*```mermaid[ \t]*$/ { next }

/^[ \t]*<details/  { details++ }
/^[ \t]*<\/details/ { details-- }

function bad(msg) { printf "%s:%d: %s\n", FILENAME, FNR, msg; rc = 1 }

# ---- mermaid fences -------------------------------------------------------
/^[ \t]*```mermaid[ \t]*$/ {
    if ($0 ~ /^[ \t]+/)  bad("mermaid fence is indented; it must start at column 1")
    if (details > 0)     bad("mermaid fence inside a <details> fold; GitHub will not render it")
    if (prev != "")      bad("no blank line before the mermaid fence")
    seen_mermaid++
}

# ---- callout alerts -------------------------------------------------------
/^[ \t]*> \[!/ {
    if ($0 !~ /^[ \t]*> \[!(NOTE|TIP|IMPORTANT|WARNING|CAUTION)\][ \t]*$/)
        bad("malformed alert marker: " $0)
    if ($0 ~ /^[ \t]+/) bad("indented alert marker; GitHub renders it as a plain blockquote")
    if (details > 0)    bad("alert inside a <details> fold; GitHub renders it as a plain blockquote")
    if (prev != "")     bad("no blank line before the alert marker")
    expect_body = 1
    seen_alert++
    prev = $0
    next
}
expect_body {
    # Indentation was already reported on the marker itself; only complain
    # here if the next line is not a blockquote continuation at all.
    if ($0 !~ /^[ \t]*> /)          bad("alert marker with no blockquote body on the next line")
    else if ($0 ~ /^[ \t]*> \[!/)   bad("two alert markers in a row")
    expect_body = 0
}

{ prev = $0 }

END {
    if (expect_body) bad("alert marker is the last line of the file")
    if (rc) {
        print "check_markup.sh: FAILED - see CLAUDE.md, Content conventions" > "/dev/stderr"
        exit 1
    }
    printf "markup OK (%d alerts, %d mermaid fences, %d files)\n", seen_alert, seen_mermaid, ARGC - 1
}
' "${FILES[@]}"
