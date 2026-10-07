#!/usr/bin/env python3
"""Eight specified faults, full scratch source copies, matching SAN/INV archive."""
from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[2]
LANE = ROOT / "lanes/q-slice4"
BUILD = LANE / "san-inv"
OUT = LANE / "faults"
OUT.mkdir(exist_ok=True)
q = (ROOT / "src/qclass.c").read_text()
d = (ROOT / "src/dump.c").read_text()


def edit(source, old, new):
    assert source.count(old) == 1, (old, source.count(old))
    return source.replace(old, new)


q1 = edit(q, "if (piece_limit < 1 || n > piece_limit) return ADF_LIMIT;",
          "if (piece_limit < 1) return ADF_LIMIT;")
q1 = edit(q1, "out.piece = flint_malloc((size_t) keep * sizeof(*out.piece));",
          "if (keep > piece_limit) status = ADF_LIMIT;\n"
          "        out.piece = flint_malloc((size_t) keep * sizeof(*out.piece));")
q1 = edit(q1, "adf_qclass_swap(y, &out); adf_qclass_clear(&out);",
          "if (status == ADF_OK) adf_qclass_swap(y, &out); adf_qclass_clear(&out);")
q2 = edit(q, "fmpz_set(keys[i].A, fmpq_numref(a->q)); fmpz_set(keys[i].H, fmpq_numref(N->q));",
          "fmpz_set(keys[i].A, pieces[i].fin.A); fmpz_set(keys[i].H, pieces[i].fin.H);")
q3 = edit(q, "return c ? c : (p->ordinal > q->ordinal) - (p->ordinal < q->ordinal);",
          "return c ? c : (p->ordinal < q->ordinal) - (p->ordinal > q->ordinal);")
d4 = edit(d, "if (cmp >= 0)\n                keys->unordered = 1;",
          "if (cmp >= 0)\n                keys->unordered = 0;")
d4 = edit(d4, "st = adf_qclass_is_canonical(&out) ? ADF_OK : ADF_DOMAIN;",
          "if (form == ADF_QCLASS_PIECES) {\n"
          "        adf_qclass_t normalized; adf_qclass_init(normalized);\n"
          "        (void) adf_qclass_set_pieces(normalized, out.piece, out.len, out.len);\n"
          "        adf_qclass_swap(&out, normalized); adf_qclass_clear(normalized);\n"
          "    }\n    st = adf_qclass_is_canonical(&out) ? ADF_OK : ADF_DOMAIN;")
needle = "st = dp_validate(&P, s, len, lim, DP_QCLASS);"
d5 = edit(d, needle, "{ arb_t probe; arb_init(probe); (void) arb_load_str(probe, s); arb_clear(probe); }\n"
          "    " + needle)
d6 = edit(d, "if (f.local) ctx = binds[nbinds == DP_ONE_CONTEXT ? 0 : occurrence++];",
          "if (f.local) { ctx = binds[0]; occurrence++; }")
d7 = edit(d, "dp_abs_over(a->e, ADF_DUMP_QCLASS_EXP_MAX) || dp_abs_over(a->re, ADF_DUMP_QCLASS_EXP_MAX)",
          "dp_abs_over(a->e, ADF_DUMP_QCLASS_EXP_MAX+1) || dp_abs_over(a->re, ADF_DUMP_QCLASS_EXP_MAX+1)")
d8 = edit(d, "/* Bind every occurrence before allocation or output writes, including the last one. */",
          "{ adf_qclass_t early; adf_qclass_init(early); adf_qclass_swap(x, early); adf_qclass_clear(early); }")
faults = [("F1_limit_after_dedup", "qclass", q1, "text"),
          ("F2_raw_keys", "qclass", q2, "text"), ("F3_second_duplicate", "qclass", q3, "text"),
          ("F4_normalizing_load", "dump", d4, "dump"), ("F5_early_flint", "dump", d5, "dump"),
          ("F6_wrong_context", "dump", d6, "dump"), ("F7_bound_plus_one", "dump", d7, "dump"),
          ("F8_early_write", "dump", d8, "dump")]
env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
results = []
for name, source, contents, test in faults:
    cfile, obj, exe = OUT / (name + ".c"), OUT / (name + ".o"), OUT / name
    cfile.write_text(contents)
    flags = ["clang", "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-DADF_CHECK_INVARIANTS",
             "-Iinclude", "-Itests", "-Isrc"]
    commands = [flags + ["-c", str(cfile), "-o", str(obj)],
                flags + [f"tests/test_qclass_{test}.c", str(obj), str(BUILD / "support/jsonl.o"),
                         str(BUILD / "support/golden.o"), str(BUILD / "support/test.o"),
                         str(BUILD / "libadelefeld.a"), "-lflint", "-lgmp", "-lm", "-pthread", "-o", str(exe)]]
    # The support list is discovered rather than assuming a third support object exists.
    commands[1] = [arg for arg in commands[1] if not arg.endswith("support/test.o")]
    log = []
    for cmd in commands:
        run = subprocess.run(["timeout", "60"] + cmd, cwd=ROOT, env=env,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        log.append(run.stdout)
        assert run.returncode == 0, (name, run.stdout)
    run = subprocess.run(["timeout", "60", str(exe)], cwd=ROOT, env=env,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    log.append(run.stdout)
    assert run.returncode not in (0, 124), (name, run.returncode)
    (OUT / (name + ".log")).write_text("".join(log)[:20000])
    detail = run.stdout.strip().splitlines()[-1] if run.stdout.strip() else "signal"
    results.append(f"{name}: exit {run.returncode}; {detail}")
    print(results[-1], flush=True)
(LANE / "fault-results.txt").write_text("\n".join(results) + "\n")
