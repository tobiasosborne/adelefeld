#!/usr/bin/env python3
"""D-DOC7 link check of README.md: every relative [text](path), src="path" and href="path" target must exist.
Run from the repository root: python3 lanes/docs-readme/check_links.py. Anchors (#...) are stripped; http(s)
links are counted and not fetched. Targets written by the other docs lanes are listed as PENDING, not as broken."""
import os, re, sys
PENDING = {"docs/GUIDE.md": "docs-guide", "julia/Adelefeld/README.md": "docs-julia", "docs/README.md": "docs-julia"}
text = open("README.md", encoding="utf-8").read()
targets = re.findall(r"\]\(([^)\s]+)\)", text) + re.findall(r'(?:src|href)="([^"]+)"', text)
checked = broken = pending = external = anchors = 0
for t in targets:
    if t.startswith(("http://", "https://")):
        external += 1
        continue
    if t.startswith("#"):
        anchors += 1
        continue
    path = t.split("#")[0]
    checked += 1
    if os.path.exists(path):
        continue
    if path in PENDING:
        pending += 1
        print("PENDING %s (lane %s)" % (path, PENDING[path]))
    else:
        broken += 1
        print("BROKEN %s" % t)
print("checked %d relative targets: %d exist, %d pending other lanes, %d broken; %d in-page anchors, %d external"
      % (checked, checked - pending - broken, pending, broken, anchors, external))
sys.exit(1 if broken else 0)
