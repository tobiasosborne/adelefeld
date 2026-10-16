#!/usr/bin/env python3
"""lanes/docs-guide/check_docs.py: the link check of D-DOC7, the line widths, and the citations of the guide.

Run from the repository root:  python3 lanes/docs-guide/check_docs.py [--citations]
1. Links: every relative target of a Markdown link [text](path) in docs/GUIDE.md and docs/examples/README.md
   is tested with os.path.exists, relative to the file (the anchor after # is dropped; a pure #anchor is
   checked against the headings of the same file). Allowed missing: julia/Adelefeld/README.md, written by
   lane docs-julia (the brief asks for the link although the file is not in this worktree); it is counted
   and reported separately.
2. Widths: lines longer than 116 characters, outside and inside fenced blocks, counted separately.
3. With --citations: every 'tools/adf/README.md:N' or ':N-M' (also the short ':N' form that follows such a
   citation in the same parenthesis or sentence) with the first line of the cited range, for review by eye.
"""
import os
import re
import sys

FILES = ["docs/GUIDE.md", "docs/examples/README.md"]
ALLOWED_MISSING = {"julia/Adelefeld/README.md"}


def anchors(text):
    out = set()
    for line in text.split("\n"):
        m = re.match(r"^#+ (.*)$", line)
        if m:
            a = m.group(1).strip().lower()
            a = re.sub(r"[^\w\- ]", "", a).replace(" ", "-")
            out.add(a)
    return out


def main():
    checked = broken = allowed = 0
    long_prose = long_code = 0
    for f in FILES:
        text = open(f).read()
        heads = anchors(text)
        infence = False
        for n, line in enumerate(text.split("\n"), 1):
            if line.startswith("```"):
                infence = not infence
            if len(line) > 116:
                if infence:
                    long_code += 1
                else:
                    long_prose += 1
                    print("LONG %s:%d (%d)" % (f, n, len(line)))
            if infence:
                continue
            for m in re.finditer(r"\[[^\]]*\]\(([^)\s]+)\)", line):
                target = m.group(1)
                if re.match(r"^[a-z]+:", target):
                    continue
                checked += 1
                path, _, frag = target.partition("#")
                if path == "":
                    ok = frag in heads
                else:
                    full = os.path.normpath(os.path.join(os.path.dirname(f), path))
                    ok = os.path.exists(full)
                    if not ok and os.path.relpath(full) in ALLOWED_MISSING:
                        allowed += 1
                        print("ALLOWED-MISSING %s:%d %s" % (f, n, target))
                        continue
                if not ok:
                    broken += 1
                    print("BROKEN %s:%d %s" % (f, n, target))
    print("links: %d checked, %d broken, %d allowed missing (lane docs-julia)" % (checked, broken, allowed))
    print("lines over 116: %d of prose, %d inside fenced blocks (verbatim driver lines)" % (long_prose, long_code))

    if "--citations" in sys.argv:
        readme = open("tools/adf/README.md").read().split("\n")
        text = open("docs/GUIDE.md").read()
        seen = []
        # a full citation, then the bare ':N' forms that follow it up to the closing parenthesis
        for m in re.finditer(r"tools/adf/README\.md((?::\d+(?:-\d+)?)(?:`?,? ?`?:\d+(?:-\d+)?)*)", text):
            for c in re.findall(r":(\d+)(?:-(\d+))?", m.group(1)):
                a = int(c[0]); b = int(c[1]) if c[1] else a
                seen.append((a, b))
        # bare continuations like (`:81`) after a tools/adf/README.md citation in the same paragraph
        for para in text.split("\n\n"):
            if "tools/adf/README.md" in para:
                for c in re.findall(r"\(`:(\d+)(?:-(\d+))?`", para):
                    a = int(c[0]); b = int(c[1]) if c[1] else a
                    seen.append((a, b))
        uniq = sorted(set(seen))
        for a, b in uniq:
            print("README:%d-%d | %s" % (a, b, readme[a - 1][:100]))
        print("citations: %d distinct ranges" % len(uniq))
    return 1 if broken or long_prose else 0


if __name__ == "__main__":
    sys.exit(main())
