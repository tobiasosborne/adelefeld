"""Planted faults. Run only in this lane; retain every build and test log."""
from pathlib import Path
import shutil
import subprocess
import json
import sys

root = Path(__file__).resolve().parents[2]
lane = root / "lanes/f-review9"
original = (root / "src/gfunc.c").read_text()

def change(old, new, count=1):
    assert original.count(old) >= count, old
    return original.replace(old, new, count)

faults = {
    "F01_real_sign_only": change("(n % 2 == 0 && sign < 0)", "(n % 2 == 0 && sign < -1)"),
    "F02_where_on_OK": change("if (st != ADF_OK && st == st_real && where != NULL)",
                              "if (st == st_real && where != NULL)"),
    "F03_idele_unit_one": change("adf_ucoset_minus_one(&t->u);", "adf_ucoset_one(&t->u);"),
    "F04_real_prec_minus_one": change("adf_real_root(J, I, n, prec);", "adf_real_root(J, I, n, prec - 1);"),
    "F05_prime_search_at_three": change("if (fmpz_fdiv_ui(A, 4) != 0)\n        return 2;", "/* skip 2 */"),
    "F06_degree_one_rounding": change("adf_adele_set(y, x);\n        return ADF_OK;",
                                     "adf_adele_set(y, x);\n        arb_set_round(y->inf, y->inf, prec < 2 ? 2 : prec);\n        return ADF_OK;"),
    "F07_LIMIT_after_degree_zero": change("if (prec > ADF_REAL_PREC_MAX)   /* N-D8: from prec alone, first */",
                                          "if (n == 0) return ADF_DOMAIN;\n    if (prec > ADF_REAL_PREC_MAX)"),
    "F08_tie_where_untouched": change("st == st_real && where != NULL", "st_real > st_fin && where != NULL"),
    "F09_negative_idele_content": change("fmpq_abs(t->r, r);", "fmpq_set(t->r, r);"),
    "F10_adele_zero_selector_rejected": change(
        "if (!(arb_is_zero(x->inf) && exact && fmpz_is_zero(x->fin.A)))", "if (1)"),
    "F11_inexact_idele_degree_one_normalised": change("adf_idele_set(y, x);\n        return ADF_OK;",
        "adf_idele_set(y, x);\n        adf_ucoset_normalise(&y->u, &y->u);\n        return ADF_OK;"),
    "F12_series_low_prec_not_clamped": change("st_real = at(t, NULL, s, inf, prec);",
        "st_real = at(t, NULL, s, inf, prec);\n    if (prec < 2) st_real = ADF_DOMAIN;"),
    "F13_inexact_idele_high_degree": change("if (adf_ucoset_is_exact(&x->u))",
        "if (adf_ucoset_is_exact(&x->u) || n >= 4)"),
}
results = json.loads((lane / "fault-results.json").read_text()) if len(sys.argv) > 1 else []
for name, source in faults.items():
    if len(sys.argv) > 1 and name not in sys.argv[1:]:
        continue
    scratch = lane / "faults" / name
    (scratch / "src").mkdir(parents=True, exist_ok=True)
    build = scratch / "build"
    build.mkdir(exist_ok=True)
    shutil.copy2(root / "Makefile", scratch / "Makefile")
    for part in ("include", "tests"):
        target = scratch / part
        if not target.exists():
            target.symlink_to(root / part, target_is_directory=True)
    shutil.copy2(lane / "build/libadelefeld.a", build / "libadelefeld.a")
    (scratch / "src/gfunc.c").write_text(source)
    cmd = ["timeout", "180", "make", "-j2", f"BUILD={build}", str(build / "test_gfunc")]
    with (scratch / "build.log").open("w") as f:
        b = subprocess.run(cmd, cwd=scratch, stdout=f, stderr=subprocess.STDOUT)
    rc = b.returncode
    test_rc = None
    output = ""
    if rc == 0:
        with (scratch / "test.log").open("w") as f:
            t = subprocess.run(["timeout", "180", str(build / "test_gfunc")], cwd=scratch,
                               stdout=f, stderr=subprocess.STDOUT)
        test_rc = t.returncode
        output = (scratch / "test.log").read_text()
    row = dict(fault=name, build=rc, test=test_rc,
               summary=[s for s in output.splitlines() if "failed checks" in s][-1:])
    results.append(row)
    (lane / "fault-results.json").write_text(json.dumps(results, indent=2) + "\n")
    print(json.dumps(row), flush=True)
