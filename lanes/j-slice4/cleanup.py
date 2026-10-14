"""Remove only the generated paths the brief authorizes, after all Julia runs have exited."""
from pathlib import Path
import shutil

root = Path.cwd()
lane = root / "lanes/j-slice4"
targets = [root / "build", lane / "depot", lane / "tmp", root / "julia/Adelefeld/Manifest.toml",
           lane / "adf"]
removed = 0
for p in targets:
    if p.is_dir():
        shutil.rmtree(p)
        removed += 1
    elif p.exists():
        p.unlink()
        removed += 1
scratch = list(lane.glob("scratch-*"))
for p in scratch:
    shutil.rmtree(p)
remaining = sum(p.exists() for p in targets) + len(list(lane.glob("scratch-*")))
print(f"TARGETS={len(targets)} REMOVED={removed} SCRATCH_REMOVED={len(scratch)} REMAINING={remaining}")
assert remaining == 0
