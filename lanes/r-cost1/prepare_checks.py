"""Read-only mirrors for driver and mutation tools with fixed build destinations."""
from pathlib import Path

root = Path.cwd()
lane = root / "lanes/r-cost1"

driver = lane / "build-driver-root"
driver.mkdir(exist_ok=True)
for name in ("Makefile", "include", "src", "tests"):
    path = driver / name
    if not path.exists():
        path.symlink_to(root / name)
tools = driver / "tools/adf"
tools.mkdir(parents=True, exist_ok=True)
for name in ("Makefile", "adf.c"):
    path = tools / name
    if not path.exists():
        path.symlink_to(root / "tools/adf" / name)

# The mutation tool copies lanes recursively. Its scratch must be outside its input
# root, and only the cache this lane needs is included in the mirror's lanes.
mutation = lane / "build-mutation-root"
mutation.mkdir(exist_ok=True)
for name in ("Makefile", "include", "src", "tests"):
    path = mutation / name
    if not path.exists():
        path.symlink_to(root / name)
cache = mutation / "lanes/r-cost1/build-mutcache"
cache.parent.mkdir(parents=True, exist_ok=True)
if not cache.exists():
    cache.symlink_to(lane / "build-mutcache")
