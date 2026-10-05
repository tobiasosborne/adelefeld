"""Remove only the known build directories owned by q-slice2."""
from pathlib import Path
import shutil

lane = Path(__file__).resolve().parent
assert lane.name == 'q-slice2' and lane.parent.name == 'lanes'
for name in ('build', 'san', 'inv', 'clang', 'clang-san-inv'):
    path = lane/name
    assert path.parent == lane and not path.is_symlink()
    if path.exists():
        assert path.is_dir()
        shutil.rmtree(path)
        print('removed', path.relative_to(lane))
