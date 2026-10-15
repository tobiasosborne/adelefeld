"""Make the isolated, unchanged source copy used by this drive. Run under timeout."""
from pathlib import Path
import shutil

lane = Path(__file__).resolve().parent
root = lane.parents[1]
dest = lane/'work'
assert not dest.exists(), 'work already exists; keep the existing evidence or remove it deliberately'
excluded = {'.git', '.agents', '.codex', '.aws', '.beads', 'lanes', 'refs', 'build'}
for source in root.iterdir():
    if source.name in excluded:
        continue
    target = dest/source.name
    if source.is_dir():
        shutil.copytree(source, target)
    else:
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
(dest/'lanes/ux-notebook').mkdir(parents=True)
print('Created unchanged source copy:', dest)
