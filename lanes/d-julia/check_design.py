"""Static review of this lane's prose, citations and fixture allocation. No external modules."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
design = root / "docs/api-j.md"
lines = design.read_text().splitlines()
assert len(lines) <= 450, len(lines)
checked = 0
for relative in ("docs/api-j.md", "lanes/d-julia/probe.jl", "lanes/d-julia/help.jl",
                 "lanes/d-julia/abi_probe.jl", "lanes/d-julia/progress.md"):
    for i, line in enumerate((root / relative).read_text().splitlines(), 1):
        assert len(line) <= 116, (relative, i, len(line))
        checked += 1

# Check explicit path citations. Bare header citations are reviewed against the same header directory.
matches = re.findall(r"((?:docs|include|tests|lanes|refs)/[\w./-]+):(\d[\d,-]*)", design.read_text())
for relative, numbers in matches:
    source = root / relative
    assert source.is_file(), relative
    count = len(source.read_text().splitlines())
    for number in re.findall(r"\d+", numbers):
        assert 1 <= int(number) <= count, (relative, number, count)
bare = re.findall(r"(?<![\w/])([\w]+\.h):(\d[\d,-]*)", design.read_text())
for name, numbers in bare:
    source = root / "include/adelefeld" / name
    count = len(source.read_text().splitlines())
    for number in re.findall(r"\d+", numbers):
        assert 1 <= int(number) <= count, (name, number, count)
fixtures = {p.name for p in (root / "tests/julia").glob("*.jl")}
assigned = set(re.findall(r"\b[\w]+\.jl\b", design.read_text())) & fixtures
missing = fixtures - assigned
assert not missing, sorted(missing)
sections = re.findall(r"^## \d+\.", design.read_text(), re.M)
assert len(sections) == 9
assert design.read_text().count("[source pending:") == 3
assert re.findall(r"^## .*", design.read_text(), re.M)[-1].startswith("## 9. Thin slices")
print(f"design_lines={len(lines)} width_lines_checked={checked} overlong=0")
print(f"explicit_citations={len(matches)} bare_header_citations={len(bare)} out_of_range=0")
print(f"fixture_files={len(fixtures)} named_in_design={len(assigned)} unnamed=0")
print("sections=9 source_pending=3 final_section=slices")
