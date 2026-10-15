# lanes/ux-notebook-r1/mutate.py: a bounded mutation run over the code the repair changed (CLAUDE.md rule 2).
# Usage: python3 lanes/ux-notebook-r1/mutate.py FIRST LAST  (mutants FIRST..LAST, 1-based), from the repo root.
# Each mutant is applied, the Julia tests run (sh ux/notebook/test.sh under timeout 240), the file restored.
import subprocess, sys, os
M = [
 ("cards.jl", "    iseven(n) && return false\n    for d", "    iseven(n) && return true\n    for d"),
 ("cards.jl", "        if pr === false", "        if pr === nothing"),
 ("notebook.jl", "f.p > DIGITS_UP_TO && (refine =", "f.p > 101 && (refine ="),
 ("driver.jl", "dump_text(y) == dump_text(x)) ? t : nothing", "true) ? t : nothing"),
 ("driver.jl", "return value_text(finitepart(x)), \" \" * FINITE_PART_NOTE", "return value_text(x), \"\""),
 ("notebook.jl", "r[\"status\"] == st || push!", "true || push!"),
 ("notebook.jl", "d == _get(r, \"dump\") || push!(diffs, _neq(\"dump\"", "true || push!(diffs, _neq(\"dump\""),
 ("notebook.jl", "x isa Integer ? want(\"$(k) =\"", "false ? want(\"$(k) =\""),
 ("notebook.jl", "            unchecked += 1\n", "            same += 1\n"),
 ("driver.jl", "    t = lossless_operand(x)\n    if t !== nothing\n", "    t = _operand(x)\n    if t !== nothing\n"),
 ("notebook.jl", "drv = why !== nothing ? nothing : lines", "drv = lines"),
 ("notebook.jl", "                PROVENANCE[m][v] = lt\n", ""),
]
first, last = int(sys.argv[1]), int(sys.argv[2])
env = dict(os.environ, ADELICA_LANE="lanes/ux-notebook-r1")
for i in range(first, last + 1):
    f, old, new = M[i - 1]
    p = "ux/notebook/src/" + f
    src = open(p).read()
    assert src.count(old) == 1, (i, "pattern not unique")
    open(p, "w").write(src.replace(old, new))
    try:
        r = subprocess.run(["timeout", "240", "sh", "ux/notebook/test.sh"], env=env, capture_output=True, text=True)
    finally:
        open(p, "w").write(src)
    tail = [l for l in r.stdout.splitlines() + r.stderr.splitlines() if "Test Summary" in l or l.startswith("Adelica ")]
    summ = [l for l in (r.stdout + r.stderr).splitlines() if l.startswith("Adelica ")]
    print(f"mutant {i} {f}: {'killed' if r.returncode != 0 else 'SURVIVED'} (exit {r.returncode}) "
          f"{summ[0].split('|')[1].strip() if summ else ''}", flush=True)
