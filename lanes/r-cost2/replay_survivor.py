#!/usr/bin/env python3
"""Replay the exact survivor with the existing tool API and the new guard regression."""
import importlib.util
import pathlib
import shutil
import sys

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location("adf_mutate", "tools/mutate/mutate.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
root = pathlib.Path("lanes/r-cost2/build-mutation-root").resolve()
shutil.copy2("tests/test_roots_real.c", root / "tests/test_roots_real.c")
shutil.copy2("lanes/r-cost2/focused-check.sh", root / "lanes/r-cost2/focused-check.sh")
source = (root / "src/roots.c").read_text()
mutants = module.mutants_of("src/roots.c", source, str(root))
chosen = [m for m in mutants if m.line == 1681 and m.kind == "logic" and m.new == "&&"]
assert len(chosen) == 1, len(chosen)
env = {"ASAN_OPTIONS": "detect_leaks=0:max_allocation_size_mb=512:allocator_may_return_null=1"}
mutant = module.check_mutant(
    chosen[0], str(root), str(pathlib.Path("lanes/r-cost2/build-replay").resolve()),
    ["Makefile", "include", "src", "tests", "lanes"],
    "timeout 170 sh lanes/r-cost2/focused-check.sh", 170, 0, env=env)
print(mutant)
print("status:", mutant.status)
print("detail:", mutant.detail)
assert mutant.status == "killed", mutant.status
