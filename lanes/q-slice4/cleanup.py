#!/usr/bin/env python3
"""Remove only this lane's build roots and its named mutation scratch directory."""
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[2]
LANE = ROOT / "lanes/q-slice4"
names = ("build", "san", "inv", "clang", "san-inv", "mutroot", "faults", "survivors")
removed = []
for path in [LANE / name for name in names] + [Path("/tmp/adf-q-slice4-mutate")]:
    if path.exists():
        assert path.parent == LANE or str(path) == "/tmp/adf-q-slice4-mutate"
        shutil.rmtree(path)
        removed.append(str(path))
with (LANE / "cleanup.log").open("a") as log:
    log.write("\n".join(removed) + f"\n{len(removed)} trees removed\n")
print(f"cleanup: {len(removed)} trees removed")

# The harness transcript is verbose. Keep its final 80 KB; complete check results have their own logs.
for path in LANE.glob("*.log"):
    if path.stat().st_size > 100000:
        assert path.name == "stdout.log", path
        tail = path.read_bytes()[-80000:]
        temporary = path.with_suffix(".trim.tmp")
        temporary.write_bytes(tail)
        temporary.replace(path)
        print(f"retained 80000-byte tail of {path.name}")
