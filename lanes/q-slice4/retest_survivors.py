#!/usr/bin/env python3
"""Retest five original survivors after adding missing tests; no new mutant is selected."""
from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[2]
LANE = ROOT / "lanes/q-slice4"
BUILD = LANE / "san-inv"
OUT = LANE / "survivors"
OUT.mkdir(exist_ok=True)
flags = ["clang", "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
         "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-DADF_CHECK_INVARIANTS",
         "-Iinclude", "-Itests", "-Isrc"]
env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
cases = [
    ("S1_decimal_limit", "text", "if (tx_real_over(s, &r, lim)) over = 1;",
     "if (tx_real_over(s, &r, lim)) over = 0;", "text"),
    ("S2_free_keys", "qclass", "flint_free(keys); adf_rat_clear(a); adf_rat_clear(N);",
     "adf_rat_clear(a); adf_rat_clear(N);", "text"),
    ("S3_last_binding", "dump", "/* Bind every occurrence before allocation or output writes, "
     "including the last one. */\n    for (i = 0; i < n; i++) {",
     "/* Skip last traversal occurrence. */\n    for (i = 1; i < n; i++) {", "dump"),
    ("S4_cmp_le", "qclass", "(p->ordinal > q->ordinal) - (p->ordinal < q->ordinal)",
     "(p->ordinal > q->ordinal) - (p->ordinal <= q->ordinal)", "component"),
    ("S5_cmp_ge", "qclass", "(p->ordinal > q->ordinal) - (p->ordinal < q->ordinal)",
     "(p->ordinal >= q->ordinal) - (p->ordinal < q->ordinal)", "component"),
]
results = []
for name, source, old, new, test in cases:
    original = (ROOT / "src" / (source + ".c")).read_text()
    assert original.count(old) == 1
    cfile, obj, exe = OUT / (name + ".c"), OUT / (name + ".o"), OUT / name
    cfile.write_text(original.replace(old, new))
    if test == "component":
        commands = [flags + [f'-DQCLASS_SOURCE="{cfile}"', str(LANE / "comparator_probe.c"),
                            str(BUILD / "libadelefeld.a"), "-lflint", "-lgmp", "-lm", "-pthread", "-o", str(exe)]]
    else:
        commands = [flags + ["-c", str(cfile), "-o", str(obj)],
                    flags + [f"tests/test_qclass_{test}.c", str(obj), str(BUILD / "support/jsonl.o"),
                             str(BUILD / "support/golden.o"), str(BUILD / "libadelefeld.a"),
                             "-lflint", "-lgmp", "-lm", "-pthread", "-o", str(exe)]]
    for cmd in commands:
        run = subprocess.run(["timeout", "60"] + cmd, cwd=ROOT, env=env,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        assert run.returncode == 0, (name, run.stdout)
    run = subprocess.run(["timeout", "60", str(exe)], cwd=ROOT, env=env,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    assert run.returncode == 1, (name, run.returncode, run.stdout)
    detail = run.stdout.strip().splitlines()[-1]
    results.append(f"{name}: exit 1; {detail}")
    print(results[-1], flush=True)
(LANE / "survivor-results.txt").write_text("\n".join(results) + "\n")
