#!/usr/bin/env python3
"""Run the two mutants of the reviewer (docs/reviews/s-design/checks/mutation_checks.py) against
proto/solvers_checks.py and report for each: KILLED (the mutated file makes the check fail, exit != 0) or
SURVIVED (exit 0). The reviewer's script stops at the first nonzero exit (it was written to show survivors); this
runner takes its two mutant texts unchanged and runs both.

Usage: python3 lanes/s-design-repair/run_reviewer_mutants.py [path of solvers_checks.py]
"""
import ast
import os
import subprocess
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[2]
target = Path(sys.argv[1]) if len(sys.argv) > 1 else root / "proto/solvers_checks.py"
review = (root / "docs/reviews/s-design/checks/mutation_checks.py").read_text()
tree = ast.parse(review)
mutants = None
for node in tree.body:
    if isinstance(node, ast.Assign) and getattr(node.targets[0], "id", "") == "mutants":
        mutants = ast.literal_eval(node.value)
assert mutants and len(mutants) == 2
source = target.read_text().rsplit('if __name__ == "__main__":', 1)[0]
env = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
survivors = 0
for name, mutant in mutants.items():
    r = subprocess.run([sys.executable, "-c", source + mutant], text=True, capture_output=True, timeout=120, env=env)
    verdict = "SURVIVED" if r.returncode == 0 else "KILLED"
    survivors += r.returncode == 0
    lines = [ln for ln in (r.stdout + r.stderr).splitlines() if ln.startswith(("PASS", "FAIL", "AssertionError"))]
    print(f"{verdict} {name}: exit={r.returncode}; {' | '.join(x[:160] for x in lines[:2])}")
print(f"surviving mutants of the reviewer: {survivors} of {len(mutants)}")
sys.exit(1 if survivors else 0)
