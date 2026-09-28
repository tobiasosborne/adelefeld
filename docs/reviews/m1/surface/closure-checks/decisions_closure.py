#!/usr/bin/env python3
"""Build and run the two proposed-decision probes from the owned scratch tree."""
import resource
import subprocess


def no_core():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


src = "docs/reviews/m1/surface/closure-checks/decisions_closure.c"
for name, flags, archive in (
    ("release", [], "build/libadelefeld.a"),
    ("invariants", ["-DADF_CHECK_INVARIANTS"], "build/inv/libadelefeld.a"),
):
    cmd = ["cc", "-std=c11", "-Iinclude", *flags, src, archive,
           "-lflint", "-lgmp", "-lm", "-pthread", "-o", f"build/decisions-{name}"]
    subprocess.run(cmd, check=True)

cases = [("release", "noncanonical", 0),
         ("invariants", "noncanonical", -6),
         ("invariants", "handwrite", -6)]
bad = 0
for binary, mode, want in cases:
    p = subprocess.run([f"build/decisions-{binary}", mode], capture_output=True,
                       text=True, preexec_fn=no_core, check=False)
    good = p.returncode == want
    print(f"{binary} {mode}: exit {p.returncode}, expected {want}, {'PASS' if good else 'FAIL'}")
    bad += not good
print(f"decisions_closure: {len(cases)} cases, {bad} failures")
raise SystemExit(bool(bad))
