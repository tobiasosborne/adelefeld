"""Check authored source regions without reformatting earlier steps or harness transcripts."""
from pathlib import Path
import sys

root = Path.cwd()
lane = root / "lanes/j-slice4"
regions = []
for name in ("text.jl", "dump.jl", "recon.jl"):
    p = root / "julia/Adelefeld/src" / name
    regions.append((p, 0, p.read_text()))
for relative, marker in (("julia/Adelefeld/test/runtests.jl", "# ---- Step J-a.4:"),
                         ("julia/Adelefeld/test/fixtures/fault.c", "/* J-a.4 enum ABI faults."),
                         ("docs/api-ja.md", "# Step J-a.4:"),
                         ("julia/Adelefeld/src/status.jl", "# Text kind:")):
    p = root / relative
    text = p.read_text()
    start = text.index(marker)
    regions.append((p, text[:start].count("\n"), text[start:]))
for p in sorted(lane.iterdir()):
    if p.suffix in (".py", ".sh", ".jl", ".md", ".cmd") and p.name != "brief.md":
        regions.append((p, 0, p.read_text()))
bad = []
for p, offset, text in regions:
    if not text.endswith("\n"):
        bad.append(f"{p.relative_to(root)}: missing final newline")
    for n, line in enumerate(text.splitlines(), offset + 1):
        if len(line) > 116:
            bad.append(f"{p.relative_to(root)}:{n}: {len(line)} characters")
print(f"REGIONS={len(regions)} VIOLATIONS={len(bad)}")
print("\n".join(bad), end="\n" if bad else "")
sys.exit(bool(bad))
