"""i-review1: hand-picked mutations of src/idele.c and src/ucoset.c, run against the lane's own tests
(tests/test_idele.c, tests/test_ucoset.c) and against this lane's fuzzers. Survivor = a mutant no test kills."""
import subprocess, os, sys, glob, shutil
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SCR = sys.argv[1]
os.makedirs(SCR, exist_ok=True)
MUT = [
 ("idele.c", "fmpz_cmp_si(gap, p) > 0", "fmpz_cmp_si(gap, p) >= 0", "B1 > to >="),
 ("idele.c", "fmpz_cmp_si(gap, p) > 0", "fmpz_cmp_si(gap, p) > 1", "B1 gap > p to > 1"),
 ("idele.c", "arf_sub(l, a, r, p, ARF_RND_FLOOR)", "arf_sub(l, a, r, p, ARF_RND_NEAR)", "l rounding floor to near"),
 ("idele.c", "arf_add(h, a, r, p, ARF_RND_CEIL)", "arf_add(h, a, r, p, ARF_RND_NEAR)", "h rounding ceil to near"),
 ("idele.c", "arf_mul(lo, lx, ly, p, ARF_RND_FLOOR)", "arf_mul(lo, lx, ly, p, ARF_RND_NEAR)", "product lo near"),
 ("idele.c", "arf_mul(hi, hx, hy, p, ARF_RND_CEIL)", "arf_mul(hi, hx, hy, p, ARF_RND_NEAR)", "product hi near"),
 ("idele.c", "arf_ui_div(lo, 1, hx, p, ARF_RND_FLOOR)", "arf_ui_div(lo, 1, hx, p, ARF_RND_NEAR)", "inv lo near"),
 ("idele.c", "arf_ui_div(hi, 1, lx, p, ARF_RND_CEIL)", "arf_ui_div(hi, 1, lx, p, ARF_RND_NEAR)", "inv hi near"),
 ("idele.c", "arf_max(d1, d1, d2);", "arf_set(d1, d1);", "B3 max dropped"),
 ("idele.c", "arf_cmpabs_mag(m, rho) <= 0", "arf_cmpabs_mag(m, rho) < 0", "B3/B4 <= to <"),
 ("idele.c", "ARF_RND_DOWN);\n        arf_mul_2exp_si(d1, d1, -1);", "ARF_RND_DOWN);\n        arf_mul_2exp_si(d1, d1, -2);", "B4 rho' half to quarter"),
 ("idele.c", "return ADF_NOT_UNIT;", "return ADF_DOMAIN;", "set_rat status"),
 ("idele.c", "int p = 0;", "int p = 0;", "noop"),
 ("idele.c", "slong p = prec < 2 ? 2 : prec;\n    arf_t lo, hi;\n    fmpz_t n;", "slong p = prec < 1 ? 1 : prec;\n    arf_t lo, hi;\n    fmpz_t n;", "set_rat p floor 2 to 1"),
 ("idele.c", "slong p = prec < 2 ? 2 : prec;\n    arf_t lx, hx, ly, hy, lo, hi;", "slong p = prec < 3 ? 3 : prec;\n    arf_t lx, hx, ly, hy, lo, hi;", "mul p floor 2 to 3"),
 ("idele.c", "arb_swap(z->inf, t);\n        fmpq_swap(z->r, r);", "arb_swap(z->inf, t);\n        fmpq_swap(z->r, x->r);", "mul content"),
 ("idele.c", "fmpq_inv(r, x->r);", "fmpq_set(r, x->r);", "inv content"),
 ("idele.c", "adf_ucoset_inv(u, &x->u);", "adf_ucoset_set(u, &x->u);", "inv unit"),
 ("idele.c", "if (s > 0)\n        adf_ucoset_one", "if (s < 0)\n        adf_ucoset_one", "set_rat unit sign"),
 ("idele.c", "ok = fmpq_sgn(t) > 0;", "ok = fmpq_sgn(t) >= 0;", "set_parts r>0"),
 ("idele.c", "sx * sy", "1", "mul sign"),
 ("ucoset.c", "fmpz_fdiv_ui(N, 4) == 2", "fmpz_fdiv_ui(N, 4) == 0", "normal form 2 mod 4"),
 ("ucoset.c", "if (fmpz_is_zero(c))\n        fmpz_set(c, N);", "if (fmpz_is_zero(c))\n        fmpz_zero(c);", "reduce 0 to N"),
 ("ucoset.c", "fmpz_divisible(a->N, b->N))", "fmpz_divisible(b->N, a->N))", "contains divisibility direction"),
 ("ucoset.c", "in = fmpz_is_zero(a->N) && fmpz_equal(a->c, b->c);", "in = fmpz_equal(a->c, b->c);", "contains exact target"),
 ("ucoset.c", "meet = fmpz_equal(x->c, y->c);", "meet = 1;", "overlaps exact-exact"),
 ("ucoset.c", "if (fmpz_is_zero(N))\n        fmpz_set(g, N2);", "if (fmpz_is_zero(N))\n        fmpz_set(g, N);", "gcd(0,N')"),
 ("ucoset.c", "uc_normal_inplace(tc, tN);      /* reduces", "uc_reduce(tc, tN);      /* reduces", "mul normal form"),
 ("ucoset.c", "uc_normal_inplace(tc, tN);\n    fmpz_swap(y->c", "uc_reduce(tc, tN);\n    fmpz_swap(y->c", "inv normal form"),
 ("ucoset.c", "fmpz_gcd(tc, c, N);\n    ok = fmpz_is_one(tc);", "ok = 1;", "set_fmpz2 gcd check"),
 ("ucoset.c", "if (fmpz_sgn(N) < 0)\n        return ADF_DOMAIN;", "", "set_fmpz2 N<0"),
 ("ucoset.c", "fmpz_sgn(x->c) <= 0 || ", "", "is_canonical c>0"),
 ("ucoset.c", "fmpz_cmp(x->c, x->N) > 0", "0", "is_canonical c<=N"),
]
sup = sorted(glob.glob(os.path.join(ROOT, "tests/support/*.c")))
def build(mut_file, test):
    objs = []
    for s in sup:
        o = os.path.join(SCR, "s_" + os.path.basename(s) + ".o")
        subprocess.run(["cc", "-std=c11", "-O1", "-w", "-I" + ROOT + "/include", "-I" + ROOT + "/tests", "-c", s, "-o", o], check=True)
        objs.append(o)
    return objs
objs = build(None, None)
def compile_test(name, srcs, exe):
    cmd = ["cc", "-std=c11", "-O1", "-w", "-I" + ROOT + "/include", "-I" + ROOT + "/tests", os.path.join(ROOT, "tests", name)] + objs + srcs + [ROOT + "/build/libadelefeld.a", "-lflint", "-lgmp", "-lm", "-pthread", "-o", exe]
    return subprocess.run(cmd, capture_output=True, text=True)
res = []
for f, old, new, name in MUT:
    src = open(os.path.join(ROOT, "src", f)).read()
    if old not in src:
        print("NOT FOUND", name); continue
    m = src.replace(old, new, 1)
    mf = os.path.join(SCR, "m_" + f)
    open(mf, "w").write(m.replace('#include "invariants.h"', '#include "%s/src/invariants.h"' % ROOT))
    mo = os.path.join(SCR, "m_" + f + ".o")
    r = subprocess.run(["cc", "-std=c11", "-O1", "-w", "-I" + ROOT + "/include", "-I" + ROOT + "/src", "-c", mf, "-o", mo], capture_output=True, text=True)
    if r.returncode:
        print("COMPILE FAIL", name, r.stderr[:200]); continue
    killed = []
    for t in ("test_idele.c", "test_ucoset.c"):
        exe = os.path.join(SCR, "t_" + t[:-2])
        c = compile_test(t, [mo], exe)
        if c.returncode:
            print("LINK FAIL", name, c.stderr[:300]); killed.append("link"); continue
        try:
            rr = subprocess.run([exe], capture_output=True, text=True, timeout=150, cwd=ROOT)
            if rr.returncode != 0: killed.append(t)
        except subprocess.TimeoutExpired:
            killed.append(t + "(timeout)")
    print("%-40s %s" % (name, ("KILLED by " + ",".join(killed)) if killed else "SURVIVED the lane's tests"), flush=True)
