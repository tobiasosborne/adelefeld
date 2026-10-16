#!/usr/bin/env python3
"""lanes/docs-julia/check.py: the checks of the lane, run from the worktree root.

1. REPL blocks: every fenced ```julia block of julia/Adelefeld/README.md whose first line starts with
   `julia> ` is compared, in order, with the blocks of a tutorial output (argument 1, default
   lanes/docs-julia/tutorial.out). Prints N blocks, M differ (and a unified diff of each difference).
2. The fenced ```text block of Pkg.test lines: every line must occur in lanes/docs-julia/pkgtest.log.
3. Exports: the names of the `export` lines of julia/Adelefeld/src/Adelefeld.jl against the first column
   of the API table of the README.
4. Links (D-DOC7): every relative target of [text](target) in julia/Adelefeld/README.md and docs/README.md,
   resolved against the page's directory, without its #fragment, must exist.
5. Widths: lines over 116 characters in both pages, outside fenced blocks and tables (prose), and inside.
Exit status 1 if any check fails.
"""
import difflib, os, re, sys

PAGES = ["julia/Adelefeld/README.md", "docs/README.md"]
fail = False

def fenced(text):
    """(info, body) for every fenced block."""
    return re.findall(r"^```(\w*)\n(.*?)\n```$", text, flags=re.M | re.S)

def tutorial_blocks(path):
    blocks, cur = [], None
    for line in open(path).read().split("\n"):
        if line.startswith("#### block "):
            cur = []; blocks.append(cur)
        elif cur is not None:
            cur.append(line)
    return ["\n".join(b).rstrip("\n") for b in blocks]

readme = open(PAGES[0]).read()
tut = tutorial_blocks(sys.argv[1] if len(sys.argv) > 1 else "lanes/docs-julia/tutorial.out")
repl = [b for info, b in fenced(readme) if info == "julia" and b.startswith("julia> ")]
differ = 0
for i in range(max(len(repl), len(tut))):
    a = repl[i] if i < len(repl) else ""
    b = tut[i] if i < len(tut) else ""
    if a != b:
        differ += 1
        print("".join(difflib.unified_diff((a + "\n").splitlines(True), (b + "\n").splitlines(True),
                                           "README block %d" % (i + 1), "tutorial block %d" % (i + 1))))
print("1. REPL blocks: README %d, tutorial %d; %d differ" % (len(repl), len(tut), differ))
fail |= differ != 0 or len(repl) != len(tut)

log = open("lanes/docs-julia/pkgtest.log").read().split("\n")
texts = [b for info, b in fenced(readme) if info == "text"]
missing = [l for b in texts for l in b.split("\n") if l not in log]
print("2. Pkg.test block: %d lines, %d not in pkgtest.log" % (sum(len(b.split("\n")) for b in texts), len(missing)))
fail |= bool(missing) or not texts

src = open("julia/Adelefeld/src/Adelefeld.jl").read()
exported = [n.strip() for l in re.findall(r"^export (.*)$", src, flags=re.M) for n in l.split(",")]
table = re.findall(r"^\| `([^`]+)`", readme, flags=re.M)
api_start = readme.index("## The exported names")
api = re.findall(r"^\| `([^`]+)`", readme[api_start:readme.index("\n## ", api_start + 3)], flags=re.M)
notin = sorted(set(exported) - set(api)); extra = sorted(set(api) - set(exported))
print("3. Exports: %d names in the export lines (%d distinct), %d rows in the API table; missing %s; extra %s"
      % (len(exported), len(set(exported)), len(api), notin, extra))
fail |= bool(notin) or bool(extra) or len(api) != len(set(api))

# Targets that another docs lane of this round writes (lanes/DOCS-COMMON.md); reported apart, not as broken.
PENDING = {"docs/GUIDE.md": "lane docs-guide"}
checked = broken = pending = 0
for page in PAGES:
    if not os.path.exists(page):
        print("4. Links: %s missing" % page); fail = True; continue
    text = open(page).read()
    text_nocode = re.sub(r"^```.*?^```$", "", text, flags=re.M | re.S)
    for t in re.findall(r"\[[^\]]*\]\(([^)\s]+)\)", text_nocode):
        if re.match(r"^[a-z]+:", t) or t.startswith("#"):
            continue
        checked += 1
        path = os.path.normpath(os.path.join(os.path.dirname(page), t.split("#")[0]))
        if not os.path.exists(path):
            if path in PENDING:
                pending += 1; print("   pending: %s -> %s (written by %s)" % (page, t, PENDING[path]))
            else:
                broken += 1; print("   broken: %s -> %s" % (page, t))
print("4. Links: %d relative links checked in %d pages, %d broken, %d pending" % (checked, len(PAGES), broken, pending))
fail |= broken != 0

for page in PAGES:
    if not os.path.exists(page):
        continue
    infence, prose, other = False, [], []
    for n, l in enumerate(open(page).read().split("\n"), 1):
        if l.startswith("```"):
            infence = not infence; continue
        if len(l) > 116:
            (other if infence or l.startswith("|") else prose).append(n)
    print("5. Widths: %s: %d prose lines over 116, %d table or code lines over 116 %s"
          % (page, len(prose), len(other), other))
    fail |= bool(prose) or bool(other)

sys.exit(1 if fail else 0)
