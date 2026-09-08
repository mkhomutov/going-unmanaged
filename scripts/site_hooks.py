"""MkDocs hooks for the site build (scripts/build_site.sh).

Two jobs, both small on purpose, so that nothing but this file stands between
the markdown under book/ and the renderer:

1. on_config builds the navigation from the chapter files themselves - the
   reading order is the file order (digits sort before letters), a part's H1
   opens a section, and each page's title is its own `## Chapter N` line - so
   adding a chapter file needs no edit here (the same rule build_book.sh
   follows for the single file).

2. on_page_markdown turns GitHub alerts (`> [!TIP]` then a blockquote body)
   into Material admonitions. Only the shape the book uses is handled; the
   marker line names the type and the body keeps its bold label, which is
   what check_markup.sh already enforces. It also marks every `<details>`
   and `<summary>` for md_in_html: GitHub resumes markdown after a blank
   line inside a raw HTML block, Python-Markdown does not, and without the
   attribute the fourteen solution and walkthrough folds render as literal
   asterisks and backticks (found by review, not by the build - a fold that
   shows raw text is still a fold to every checker).
"""
import pathlib
import re

# Material has no "important" type - an unknown type renders in the plain
# note style, which would give the book's two non-negotiable rules (CLAUDE.md:
# IMPORTANT only if breaking it is a bug) the mildest box on the page while
# every Trap gets a stronger one. "danger" is Material's strongest, so the
# visual order matches GitHub's: TIP < NOTE < WARNING < IMPORTANT. An
# extra_css rule would have to live under book/, which is the book's, not
# the site's.
ALERT_TYPES = {
    "NOTE": "note",
    "TIP": "tip",
    "IMPORTANT": "danger",
    "WARNING": "warning",
    "CAUTION": "danger",
}
ALERT_MARK = re.compile(r"^> \[!(NOTE|TIP|IMPORTANT|WARNING|CAUTION)\]\s*$")


def _headings(path):
    """First H1 (a part or the Appendices separator, if any) and first H2.

    Lines inside a code fence are skipped: a `# comment` in a shell or CMake
    listing is not a heading, and a Part intro may open a fence before the
    chapter's own `## ` line.
    """
    h1 = h2 = None
    in_fence = False
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("```") or line.startswith("~~~"):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        if h1 is None and line.startswith("# "):
            h1 = line[2:].strip()
        elif h2 is None and line.startswith("## "):
            h2 = line[2:].strip()
        if h2 is not None:
            break
    return h1, h2


def on_config(config):
    docs = pathlib.Path(config["docs_dir"])
    nav = [{"Contents": "README.md"}]
    section_title, section = None, None
    for path in sorted(p for p in docs.glob("*.md") if p.name != "README.md"):
        h1, h2 = _headings(path)
        if h1 is not None:
            section_title, section = h1, []
            nav.append({section_title: section})
        entry = {h2 or path.stem: path.name}
        (section if section is not None else nav).append(entry)
    config["nav"] = nav
    return config


def on_page_markdown(markdown, page, config, files):
    markdown = markdown.replace("<details>", '<details markdown="1">')
    markdown = markdown.replace("<summary>", '<summary markdown="1">')
    out, lines, i = [], markdown.split("\n"), 0
    while i < len(lines):
        match = ALERT_MARK.match(lines[i])
        if not match:
            out.append(lines[i])
            i += 1
            continue
        out.append(f'!!! {ALERT_TYPES[match.group(1)]} ""')
        i += 1
        while i < len(lines) and lines[i].startswith(">"):
            out.append("    " + lines[i][1:].lstrip(" "))
            i += 1
        out.append("")
    return "\n".join(out)
