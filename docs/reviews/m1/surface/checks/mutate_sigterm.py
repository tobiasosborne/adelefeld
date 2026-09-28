#!/usr/bin/env python3
"""mutate_sigterm.py: stop tools/mutate/mutate.py with SIGTERM (what `timeout`, a CI runner or
`kill` sends) while a mutant's test program loops, and look for what is left behind.

    python3 docs/reviews/m1/surface/checks/mutate_sigterm.py      (from the repository root)

The tool promises: 'The source tree is never written to' and removes its scratch run directory
in a try/finally (mutate.py:778-782); run_make kills the process group of a mutant on timeout
(mutate.py:675-696, issue adf-98j).  The mini tree has two mutants whose test loops for ever
(src/mini.c:24, `n - 1` to `n + 1` and `1` to `0`)."""
import os, shutil, signal, subprocess, sys, time

scratch = os.path.abspath("build/review_mutate_term")
shutil.rmtree(scratch, ignore_errors=True)
p = subprocess.Popen([sys.executable, "tools/mutate/mutate.py", "--root",
                      "docs/reviews/m1/surface/checks/mini", "--scratch", scratch,
                      "--files", "src/mini.c", "--timeout", "600", "--jobs", "2",
                      "--equivalent", "/dev/null"],
                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
time.sleep(6)
p.send_signal(signal.SIGTERM)
p.wait()
time.sleep(1)
print("tool exit code after SIGTERM:", p.returncode)
left = []
for d, _, fs in os.walk(scratch):
    left.extend(os.path.join(d, f) for f in fs)
print("files left under the scratch directory:", len(left))
ps = subprocess.run(["pgrep", "-af", "build/test_mini"], capture_output=True, text=True).stdout
alive = [l for l in ps.splitlines() if "pgrep" not in l]
print("mutant test programs still running:", len(alive))
for l in alive:
    print("   ", l)
# clean up what the tool left
for l in alive:
    try:
        os.kill(int(l.split()[0]), signal.SIGKILL)
    except OSError:
        pass
shutil.rmtree(scratch, ignore_errors=True)
