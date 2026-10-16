#!/usr/bin/env python3
"""Run every "$ " command shown in README.md, as written, in a fresh shell (env -i) from the repository root,
each under timeout, and compare its standard output with the lines the README shows after it.
Run from the repository root after the build: python3 lanes/docs-readme/check_readme_commands.py [--log FILE]
Julia gets an empty depot of its own (JULIA_DEPOT_PATH=<tmpdir>), so the run is that of a new reader and writes
nothing to ~/.julia/compiled. Exit 0 when every output agrees."""
import os, shutil, subprocess, sys, tempfile

lines = open("README.md", encoding="utf-8").read().split("\n")
cases, i = [], 0
while i < len(lines):
    raw = lines[i]
    s = raw[4:] if raw.startswith("    $ ") else raw
    if s.startswith("$ "):
        indent = len(raw) - len(s)
        cmd = s[2:]
        while cmd.endswith("\\"):
            i += 1
            cmd = cmd + "\n" + lines[i]
        out = []
        i += 1
        while i < len(lines):
            t = lines[i][indent:] if indent else lines[i]
            if t.startswith("$ ") or t.startswith("```") or lines[i].strip() == "":
                break
            out.append(t)
            i += 1
        cases.append((cmd, out))
        continue
    i += 1

depot = tempfile.mkdtemp(prefix="fresh-julia-depot-")
env = {"HOME": os.environ["HOME"], "PATH": os.environ["PATH"], "JULIA_DEPOT_PATH": depot,
       "JULIA_NUM_THREADS": "1", "LANG": "C.UTF-8"}
log = []
bad = 0
for cmd, want in cases:
    if cmd.startswith("git clone"):
        log.append(("$ " + cmd, "(not run: network; the build lines are run below in the worktree)", "SKIP"))
        continue
    r = subprocess.run(["timeout", "600", "sh", "-c", cmd], cwd=os.getcwd(), env=env, capture_output=True,
                       text=True)
    got = r.stdout.rstrip("\n").split("\n") if r.stdout.strip() else []
    if cmd.startswith("make"):
        verdict = "OK (exit %d)" % r.returncode if r.returncode == 0 else "FAIL (exit %d)" % r.returncode
        shown = "(build output not compared; exit status %d)" % r.returncode
    else:
        verdict = "OK" if got == want and r.returncode == 0 else "DIFF (exit %d)" % r.returncode
        shown = "\n".join(got)
    if not verdict.startswith("OK"):
        bad += 1
        print("want:\n" + "\n".join(want) + "\ngot:\n" + "\n".join(got) + "\nstderr:\n" + r.stderr[-800:],
              file=sys.stderr)
    log.append(("$ " + cmd, shown, verdict))
for c, o, v in log:
    print("%s\n%s\n[%s]\n" % (c, o, v))
shutil.rmtree(depot, ignore_errors=True)
print("%d commands: %d agree, %d skipped, %d differ" % (len(log), sum(v.startswith("OK") for _, _, v in log),
      sum(v == "SKIP" for _, _, v in log), bad))
sys.exit(1 if bad else 0)
