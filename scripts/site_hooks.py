"""MkDocs hooks for the site build (scripts/build_site.sh).

Two jobs, both small on purpose, so that nothing but this file stands between
the markdown under book/ and the renderer:

1. on_config builds the navigation from book/README.md's three entry-point
   groups - the `### Reference`, `### Concepts` and `### Labs` lists under
   Contents - in the order the README gives them, each page titled by its
   own `## ` line. A page in none of the three groups is left out of the nav,
   which MkDocs reports and --strict turns into a failure: adding a chapter
   file means adding it to a group, and the build says so. The chapter files
   carry no navigation of their own.

2. on_page_markdown renders every ```mermaid fence to SVG at build time when
   mermaid-cli (`mmdc`) is on PATH - once in mermaid's default theme and once
   in its dark theme, shown per Material colour scheme by a style block the
   hook adds to the page - so the published site loads no diagram script
   from any CDN at read time. Without `mmdc` (a laptop without Node) the
   fence is left for Material's browser-side rendering, which fetches
   mermaid from a CDN: the same picture, one network dependency more. CI's
   site job and the Pages workflow install mermaid-cli, so what is published
   is always the pre-rendered form. Renders are cached under build/ by the
   hash of the fence and the theme.

3. on_page_markdown also turns GitHub alerts (`> [!TIP]` then a blockquote body)
   into Material admonitions. Only the shape the book uses is handled; the
   marker line names the type and the body keeps its bold label, which is
   what check_markup.sh already enforces. It also marks every `<details>`
   and `<summary>` for md_in_html: GitHub resumes markdown after a blank
   line inside a raw HTML block, Python-Markdown does not, and without the
   attribute the fourteen solution and walkthrough folds render as literal
   asterisks and backticks (found by review, not by the build - a fold that
   shows raw text is still a fold to every checker).
"""
import hashlib
import pathlib
import re
import shutil
import subprocess
import tempfile

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


GROUPS = ("Reference", "Concepts", "Labs")
LINK = re.compile(r"\]\(([0-9A-Za-z][^)#]*\.md)#")


def on_config(config):
    docs = pathlib.Path(config["docs_dir"])
    readme = (docs / "README.md").read_text(encoding="utf-8")
    nav = [{"Contents": "README.md"}]
    current = None
    for line in readme.splitlines():
        if line.startswith("### "):
            title = line[4:].strip()
            if title in GROUPS:
                current = []
                nav.append({title: current})
            else:
                current = None
            continue
        if current is None:
            continue
        for name in LINK.findall(line):
            if any(name in entry.values() for entry in current):
                continue
            _, h2 = _headings(docs / name)
            current.append({h2 or pathlib.Path(name).stem: name})
    config["nav"] = nav
    return config


MERMAID_FENCE = re.compile(r"^```mermaid\n(.*?)^```\n", re.M | re.S)
DIAGRAM_STYLE = (
    '<style>'
    '.diagram svg{max-width:100%;height:auto}'
    '[data-md-color-scheme="default"] .diagram-dark{display:none}'
    '[data-md-color-scheme="slate"] .diagram-light{display:none}'
    '</style>\n'
)


def _mmdc():
    return shutil.which("mmdc")


def _render_mermaid(source, theme, cache_dir):
    key = hashlib.sha256((theme + "\n" + source).encode("utf-8")).hexdigest()[:24]
    cached = cache_dir / f"{key}.svg"
    if cached.exists():
        return cached.read_text(encoding="utf-8")
    with tempfile.TemporaryDirectory() as tmp:
        src = pathlib.Path(tmp) / "d.mmd"
        out = pathlib.Path(tmp) / "d.svg"
        src.write_text(source, encoding="utf-8")
        subprocess.run([_mmdc(), "-q", "-i", str(src), "-o", str(out), "-t", theme, "-b", "transparent"],
                       check=True, capture_output=True)
        svg = out.read_text(encoding="utf-8")
    cache_dir.mkdir(parents=True, exist_ok=True)
    cached.write_text(svg, encoding="utf-8")
    return svg


def _prerender_diagrams(markdown, config):
    if not _mmdc():
        return markdown
    cache_dir = pathlib.Path(config["site_dir"]).parent / "site-mermaid-cache"
    rendered = 0

    def replace(match):
        nonlocal rendered
        source = match.group(1)
        light = _render_mermaid(source, "default", cache_dir)
        dark = _render_mermaid(source, "dark", cache_dir)
        rendered += 1
        return ('<div class="diagram diagram-light">' + light + "</div>\n"
                '<div class="diagram diagram-dark">' + dark + "</div>\n")

    markdown = MERMAID_FENCE.sub(replace, markdown)
    if rendered:
        markdown = DIAGRAM_STYLE + markdown
    return markdown


def on_page_markdown(markdown, page, config, files):
    markdown = _prerender_diagrams(markdown, config)
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
