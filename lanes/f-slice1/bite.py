"""Shows that tests/test_lball.c bites: plants faults in a scratch copy of src/lball.c under build/bite/ and runs the
test against each. Run from the repository root after `make -j2 build/test_lball`:
    python3 -B lanes/f-slice1/bite.py
Each fault is (name, old text, new text); the old text must occur exactly once. The real source is not changed."""
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = (ROOT / "src" / "lball.c").read_text()
OUT = ROOT / "build" / "bite"
OUT.mkdir(parents=True, exist_ok=True)

MUL_TERMS = ("        if (!y->exact && !xz)\n        {\n            c = x->v + y->N;\n            if (c < K)\n"
             "                K = c;\n        }\n"
             "        if (!x->exact && !yz)\n        {\n            c = y->v + x->N;\n            if (c < K)\n"
             "                K = c;\n        }\n")
MUL_TERMS_BAD = MUL_TERMS.replace("c = x->v + y->N;", "c = y->N;").replace("c = y->v + x->N;", "c = x->N;")
NN_TERM = ("        if (!x->exact && !y->exact)\n        {\n            c = x->N + y->N;\n")
NN_TERM_BAD = ("        if (!x->exact && !y->exact && !xz && !yz)\n        {\n            c = x->N + y->N;\n")

FAULTS = [
    ("mul exponent = min(N, N') without the valuations", MUL_TERMS, MUL_TERMS_BAD),
    ("mul without the term N + N' for two balls around 0", NN_TERM, NN_TERM_BAD),
    ("add exponent = max(N, N') instead of min",
     "(x->N < y->N ? x->N : y->N);\n    if (!fmpq_is_zero(x->u) && x->v < K)",
     "(x->N > y->N ? x->N : y->N);\n    if (!fmpq_is_zero(x->u) && x->v < K)"),
    ("inverse exponent N - v instead of N - 2 v",
     "lb_make(res, x->p, q, -x->v, x->exact, x->exact ? 0 : x->N - 2 * x->v);",
     "lb_make(res, x->p, q, -x->v, x->exact, x->exact ? 0 : x->N - x->v);"),
    ("sub computed as add (no negation)",
     "st = adf_lball_neg(m, y);", "adf_lball_set(m, y); st = ADF_OK;"),
    ("add never drops the second operand of valuation >= K",
     "if (!fmpq_is_zero(y->u) && y->v < K)", "if (!fmpq_is_zero(y->u))"),
    ("contains ignores the precision test N >= N'",
     "    if (!x->exact && x->N < y->N)\n        return 0;\n", ""),
    ("is_canonical does not test that p does not divide u",
     " || fmpz_fdiv_ui(fmpq_numref(x->u), p) == 0)", ")"),
    ("valuation of a ball around 0 answered instead of NOT_DETERMINED",
     "        if (!x->exact)\n            return ADF_NOT_DETERMINED;\n        *v = 0;", "        *v = 0;"),
    ("inverse of a ball around 0 answers NOT_UNIT instead of UNIT_NOT_CERTIFIED",
     "return x->exact ? ADF_NOT_UNIT : ADF_UNIT_NOT_CERTIFIED;", "return ADF_NOT_UNIT;"),
    ("centre reduction: the integer shortcut also for a negative numerator",
     "fmpz_is_one(dr) && fmpz_sgn(nr) > 0 && (ulong) k >= need", "fmpz_is_one(dr) && (ulong) k >= need"),
    ("decompose gives relative precision N instead of N - v",
     "res->N = x->exact ? 0 : x->N - x->v;", "res->N = x->exact ? 0 : x->N;"),
    ("mul writes its output before the status is known",
     "    fmpq_clear(q);\n    return finish(z, res, st);\n}\n\n/* L4. */",
     "    fmpq_clear(q);\n    if (st != ADF_OK) adf_lball_set(z, x);\n    return finish(z, res, st);\n}\n\n/* L4. */"),
]

results = []
for i, (name, old, new) in enumerate(FAULTS):
    if SRC.count(old) != 1:
        print(f"fault {i}: anchor occurs {SRC.count(old)} times: {name}")
        sys.exit(2)
    src = OUT / f"lball_f{i}.c"
    src.write_text(SRC.replace(old, new))
    exe = OUT / f"test_f{i}"
    cmd = ["cc", "-Iinclude", "-Itests", "-std=c11", "-O1", "-g", "-Wall", "-Wextra", "-Wno-unused-function",
           "-Wno-unused-variable", str(src), "tests/test_lball.c", "build/support/golden.o", "build/support/jsonl.o",
           "build/libadelefeld.a", "-lflint", "-lgmp", "-lm", "-o", str(exe)]
    r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        print(f"fault {i}: does not compile: {r.stderr[:300]}")
        sys.exit(2)
    try:
        t = subprocess.run([str(exe)], cwd=ROOT, capture_output=True, text=True, timeout=120)
        lines = t.stdout.strip().splitlines()
        tail = lines[-1] if lines else "(no output, exit %d)" % t.returncode
        failed = [l[5:].split(" (")[0] for l in lines if l.startswith("FAIL ") and "failed checks" in l]
        results.append((i, name, tail, failed))
    except subprocess.TimeoutExpired:
        results.append((i, name, "timeout", ["timeout"]))
for i, name, tail, failed in results:
    print(f"fault {i}: {name}\n    -> {tail}\n    tests that failed: {', '.join(failed) if failed else 'NONE'}")
survivors = [i for i, _, _, f in results if not f]
print(f"{len(results)} faults, {len(results) - len(survivors)} caught, survivors: {survivors}")
