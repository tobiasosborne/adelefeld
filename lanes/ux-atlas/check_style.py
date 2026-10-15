#!/usr/bin/env python3
"""Check the brief's line limits. Run: timeout 120 python3 lanes/ux-atlas/check_style.py"""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
atlas = root / "ux/atlas"
lane = root / "lanes/ux-atlas"
files = [p for pattern in ("*.py", "*.js", "*.mjs", "*.html", "*.md", "*.sh") for p in atlas.glob(pattern)]
files += [lane / "check_scenes.py", lane / "check_viewer.mjs", Path(__file__).resolve()]
long_lines = [(str(p.relative_to(root)), i, len(s))
              for p in files for i, s in enumerate(p.read_text().splitlines(), 1) if len(s) > 116]
readme_lines = len((atlas / "README.md").read_text().splitlines())
html_lines = len((atlas / "atlas.html").read_text().splitlines())
print("files", len(files), "lines over 116", len(long_lines), long_lines)
print("README lines", readme_lines, "HTML lines", html_lines)
assert not long_lines
assert readme_lines < 80
assert html_lines < 1200
