"""Bounded hand-picked faults in lane-local source copies; never changes src or tests."""
import pathlib
import subprocess

lane = pathlib.Path("lanes/i-review2")
directory = lane / "mutants"
directory.mkdir(exist_ok=True)
faults = [
    ("class_norm", "idclass", "test_idclass", "st = adf_idele_norm(t, x, prec);",
     "adf_idele_abs_inf(t, x); st = ADF_OK;"),
    ("class_product_unit", "idclass", "test_idclass", "adf_ucoset_mul(u, &x->u, &y->u);",
     "adf_ucoset_set(u, &x->u);"),
    ("valuation_sum", "idele", "test_idele_maps", "*v = a - b;", "*v = a + b;"),
    ("norm_reciprocal", "idele", "test_idele_maps",
     "adf_idele_ends_scale(lo, hi, lx, hx, fmpq_denref(x->r), fmpq_numref(x->r), p);",
     "adf_idele_ends_scale(lo, hi, lx, hx, fmpq_numref(x->r), fmpq_denref(x->r), p);"),
    ("mul_rat_sign", "idele", "test_idele_maps", "sx * sq, p);", "sx, p);"),
    ("tight_two_depth", "idpow", "test_idpow", "2 + (ulong) fac.exp[i]", "1 + (ulong) fac.exp[i]"),
    ("power_sign", "idpow", "test_idpow", "k % 2 != 0 ? s : 1, p);", "s, p);"),
    ("content_limit_edge", "idpow", "test_idpow",
     "return abs_k(k) > (ulong) ADF_IDELE_POW_BITS_MAX / b;",
     "return abs_k(k) >= (ulong) ADF_IDELE_POW_BITS_MAX / b;"),
    ("hull_two", "idmap", "test_idmap", "fmpz_mul_2exp(H, H, 1);", "fmpz_mul_2exp(H, H, 0);"),
    ("div_wrong_unit", "idmap", "test_idmap", "adf_ucoset_inv(w, &y->u);", "adf_ucoset_set(w, &y->u);"),
    ("conversion_certification", "idmap", "test_idmap",
     "st = FLINT_MAX(st, ADF_UNIT_NOT_CERTIFIED);     /* SPEC 4.5",
     "st = FLINT_MAX(st, ADF_OK);                     /* SPEC 4.5"),
]
support = list((lane / "build/support").glob("*.o"))
killed = 0
for name, source, test, before, after in faults:
    text = pathlib.Path(f"src/{source}.c").read_text()
    assert text.count(before) == 1, (name, text.count(before))
    sourcepath = directory / (name + ".c")
    sourcepath.write_text(text.replace(before, after))
    exe = directory / name
    obj = directory / (name + ".o")
    commands = [
        ["timeout", "30", "cc", "-Iinclude", "-Isrc", "-std=c11", "-O1", "-g", "-c", str(sourcepath),
         "-o", str(obj)],
        ["timeout", "30", "cc", "-Iinclude", "-Itests", "-std=c11", "-O1", "-g", f"tests/{test}.c",
         str(obj), *map(str, support), str(lane / "build/libadelefeld.a"), "-lflint", "-lgmp", "-lm",
         "-o", str(exe)],
    ]
    for cmd in commands:
        result = subprocess.run(cmd, capture_output=True, text=True)
        assert result.returncode == 0, (name, result.stderr)
    result = subprocess.run(["timeout", "35", str(exe)], capture_output=True, text=True)
    (directory / (name + ".log")).write_text(result.stdout + result.stderr)
    assert result.returncode not in (0, 124), (name, result.returncode)
    killed += 1
    summary = next((line for line in reversed(result.stdout.splitlines()) if "failed checks" in line), "aborted")
    print(f"{name}: exit={result.returncode} {summary}", flush=True)
print(f"mutations={len(faults)} killed={killed} survived={len(faults)-killed} compile_failures=0 timeouts=0")
