#!/usr/bin/env python3
"""Check every SVG under docs/assets: it parses (xml.dom.minidom), the root has width, height and viewBox, and the
file has no <script> element, no on* event attribute and no external reference (href to http). Run from the
repository root: python3 lanes/docs-readme/check_svgs.py"""
import glob, re, sys, xml.dom.minidom
bad = 0
files = sorted(glob.glob("docs/assets/**/*.svg", recursive=True))
for f in files:
    text = open(f, encoding="utf-8").read()
    root = xml.dom.minidom.parse(f).documentElement
    problems = [a for a in ("width", "height", "viewBox") if not root.getAttribute(a)]
    if root.getElementsByTagName("script") or re.search(r"<script", text, re.I):
        problems.append("script")
    if re.search(r"\son[a-z]+\s*=", text):
        problems.append("event attribute")
    if re.search(r"href\s*=\s*[\"']https?:", text):
        problems.append("external href")
    print("%-40s %s x %s  viewBox %-12s %s" % (f, root.getAttribute("width"), root.getAttribute("height"),
                                              root.getAttribute("viewBox"), ", ".join(problems) or "ok"))
    bad += bool(problems)
print("%d SVG files, %d with a problem" % (len(files), bad))
sys.exit(1 if bad else 0)
