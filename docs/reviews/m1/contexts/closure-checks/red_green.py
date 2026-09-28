#!/usr/bin/env python3
"""Revert one repair condition at a time in a scratch object, then run a regression.

Run from the repository root after the focused test binaries have been built.
"""
from contextlib import contextmanager
from pathlib import Path
import re
import subprocess
import tempfile

root = Path.cwd()
objects = sorted((root / "build").glob("*.o"))


@contextmanager
def scratch_build(name, edits, test_source):
    source = (root / "src" / name).read_text()
    for old, new in edits:
        assert source.count(old) >= 1, (name, old, source.count(old))
        source = source.replace(old, new)
    with tempfile.TemporaryDirectory() as td:
        temp = Path(td)
        src = temp / name
        src.write_text(source)
        obj = temp / name.replace(".c", ".o")
        binary = temp / "probe"
        subprocess.run(["cc", "-std=c11", "-O2", "-g", "-Iinclude", "-c", str(src),
                        "-o", str(obj)], check=True)
        other = [str(p) for p in objects if p.name != obj.name]
        subprocess.run(["cc", "-std=c11", "-O2", "-g", "-Iinclude", "-Itests",
                        str(test_source), str(obj), *other,
                        "build/support/golden.o", "build/support/jsonl.o",
                        "-lflint", "-lgmp", "-lm", "-lpthread", "-o", str(binary)],
                       check=True)
        yield binary


loss = """    if (lost != NULL)
    {
        fmpz_mul(t, x->u, Kp);
        *lost = fmpz_divisible(t, K) ? 0 : 1;
    }
"""
with scratch_build("scaled.c", [(loss, ""),
                                ("    y->exact = 0;\n    fmpz_clear(K);",
                                 "    y->exact = 0;\n" + loss + "    fmpz_clear(K);")],
                   root / "docs/reviews/m1/contexts/checks/set_context_alias.c") as binary:
    run = subprocess.run([binary], text=True, capture_output=True)
    line = next(x for x in run.stdout.splitlines() if x.startswith("cases "))
    wrong = int(re.search(r"wrong lost \(y = x\) (\d+)", line).group(1))
    print("R1 scratch old loss order:", line)
    assert wrong > 0

guard = """    if (k > ADF_MODCTX_MAX_BLOCKS)
        return ADF_UNSUPPORTED;
"""
with scratch_build("modctx.c", [(guard, "")], root / "tests/test_modctx_limits.c") as binary:
    run = subprocess.run([binary], text=True, capture_output=True, timeout=20)
    print("R2 scratch no k guards: exit", run.returncode,
          "last output", run.stdout.splitlines()[-1:] or ["none"])
    assert run.returncode != 0

restriction = """    adf_ctx_desc_init(&d);
    st = adf_dump_ctx_occurrence(&d, s, len, occurrence, lim);"""
old_restriction = """    if (len < 14 || memcmp(s, "adf1 Q modctx ", 14) != 0)
        return ADF_PARSE;
    adf_ctx_desc_init(&d);
    st = adf_dump_ctx_occurrence(&d, s, len, occurrence, lim);"""
with scratch_build("modctx.c", [(restriction, old_restriction)],
                   root / "docs/reviews/m1/contexts/checks/dump_run.c") as binary:
    run = subprocess.run(["python3", str(root / "docs/reviews/m1/contexts/closure-checks/context_dump_cases.py"),
                          str(binary)], text=True, capture_output=True, timeout=20)
    print("R3 scratch modctx-only: exit", run.returncode,
          "summary", [x for x in run.stdout.splitlines() if x.startswith("cases ")])
    assert run.returncode != 0

header = "    if (pos == v0 || (pos < len && s[pos] != ' '))"
with scratch_build("dump.c", [(header, "    if (pos == v0)")],
                   root / "docs/reviews/m1/contexts/checks/dump_run.c") as binary:
    run = subprocess.run(["python3", str(root / "docs/reviews/m1/contexts/closure-checks/context_dump_cases.py"),
                          str(binary)], text=True, capture_output=True, timeout=20)
    print("R4 scratch early version decision: exit", run.returncode,
          "summary", [x for x in run.stdout.splitlines() if x.startswith("cases ")])
    assert run.returncode != 0

with scratch_build("scaled.c", [("            fmpq_mul(t.s, aq, w->s);\n", "")],
                   root / "docs/reviews/m1/contexts/checks/scaled_ops.c") as binary:
    run = subprocess.run([binary, "1", "100"], text=True, capture_output=True, timeout=20)
    with tempfile.NamedTemporaryFile(mode="w", suffix=".txt") as output:
        output.write(run.stdout)
        output.flush()
        oracle = subprocess.run(["python3",
                                 str(root / "docs/reviews/m1/contexts/checks/scaled_oracle.py"),
                                 output.name], text=True, capture_output=True, timeout=20)
    print("R5 current line 466 drop_call: oracle exit", oracle.returncode,
          "summary", oracle.stdout.splitlines()[-2:])
    assert oracle.returncode != 0
