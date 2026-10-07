"""Hunt 7: plant faults in scratch copies of src files, build a library copy per fault, link the lanes' tests,
run them. Usage (from the worktree root): python3 lanes/m4-review1/gen/faults.py [fault ids]"""
import os, subprocess, sys, shutil, time

ROOT = os.getcwd()
LANE = "lanes/m4-review1"
B = LANE + "/build"
FD = LANE + "/faults"
CF = "-Iinclude -std=c11 -O2 -g -Wall -Wextra -Wpedantic".split()
TESTS = ["test_ffun", "test_ffun_algebra", "test_rfun", "test_rfun_fourier", "test_tensor", "test_poisson",
         "test_ffun_dump", "test_rfun_dump"]

F = {
    "F00 baseline": ("ffun.c", []),
    "F01 finite kernel sign": ("ffun.c", [("fmpq_set_ui(theta,h ? L-h : 0,L)", "fmpq_set_ui(theta,h,L)")]),
    "F02 real kernel sign": ("rfun.c", [("acb_mul_onei(r->B, r->B);", "acb_div_onei(r->B, r->B);"),
                                        ("acb_div_onei(c1, c1);", "acb_mul_onei(c1, c1);"),
                                        ("acb_mul_onei(iinv, inv);", "acb_div_onei(iinv, inv);")]),
    "F03 weight 1/D for 1/M": ("ffun.c", [("acb_div_ui(t->f+k,t->f+k,x->M,p);", "acb_div_ui(t->f+k,t->f+k,x->D,p);")]),
    "F04 reflection dropped": ("ffun.c", [("acb_set(t->f+k,x->f+(k ? L-k : 0));", "acb_set(t->f+k,x->f+k);")]),
    "F05 lcm for gcd in E1": ("tensor.c", [("fmpz_gcd(g, g, t);", "fmpz_lcm(g, g, t);")]),
    "F06 right derived from left": ("poisson.c", [("    if (st == ADF_OK)\n    {\n        arb_get_mag(m, EL);",
                                                   "    if (st == ADF_OK) acb_set(rsum, lsum);\n    if (st == ADF_OK)\n    {\n        arb_get_mag(m, EL);")]),
    "F07 tail without factor 2": ("poisson.c", [("arb_mul_2exp_si(B, B, 1);", "")]),
    "F08 width checked before tail": ("poisson.c", [
        ("static int\npn_charge", "static mag_struct pn_tl[1], pn_tr[1];\nstatic int\npn_charge"),
        ("acb_add_error_mag(lsum, m);", "mag_set(pn_tl, m);"),
        ("acb_add_error_mag(rsum, m);", "mag_set(pn_tr, m);"),
        ("        acb_swap(left, l);", "        acb_add_error_mag(l, pn_tl); acb_add_error_mag(r, pn_tr);\n        acb_swap(left, l);")]),
    "F09 Re(1/A) not re-checked": ("rfun.c", [("if (!rf_fourier_term(t + i, x->term + i, p) || !rf_result_ok(t + i))",
                                               "if (!rf_fourier_term(t + i, x->term + i, p))")]),
    "F10 loader normalising": ("dump.c", [
        ("                if (L && j == L - 1 && dp_zero(a[0].m) && dp_zero(a[0].rm) &&\n"
         "                    dp_zero(a[1].m) && dp_zero(a[1].rm)) return ADF_DOMAIN;\n", ""),
        ("        _acb_poly_set_length(t->P, (slong) L);\n", "        _acb_poly_set_length(t->P, (slong) L); _acb_poly_normalise(t->P);\n")]),
    "F11 translation wrong way": ("ffun.c", [("source->f+(k+L-shift)%L", "source->f+(k+shift)%L")]),
    "F12 content inverted in dilate_idele": ("ffun.c", [
        ("    st=ffun_dilate_shape(&D,&M,x,a->r); if (st==ADF_LIMIT) return st;",
         "    fmpq_t rinv; fmpq_init(rinv); fmpq_inv(rinv,a->r);\n    st=ffun_dilate_shape(&D,&M,x,rinv); if (st==ADF_LIMIT) return st;"),
        ("ffun_dilate_cells(t,x,a->r,unit);", "ffun_dilate_cells(t,x,rinv,unit);")]),
    "F13 hull sampled at one point": ("tensor.c", [("        if (fmpz_divisible(t, g))\n            tn_box_join(&b, f->f + j, p);",
                                                   "        if (fmpz_divisible(t, g))\n        { tn_box_join(&b, f->f + j, p); break; }")]),
}


def sh(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)


def main():
    sel = sys.argv[1:]
    os.makedirs(FD, exist_ok=True)
    # test objects once
    for t in TESTS:
        o = "%s/%s.o" % (FD, t)
        if not os.path.exists(o):
            r = sh(["cc"] + CF + ["-Itests", "-c", "tests/%s.c" % t, "-o", o])
            assert r.returncode == 0, r.stderr
    sup = sorted("%s/support/%s" % (B, f) for f in os.listdir(B + "/support") if f.endswith(".o"))
    for name, (src, edits) in F.items():
        if sel and name.split()[0] not in sel:
            continue
        d = "%s/%s" % (FD, name.split()[0])
        os.makedirs(d, exist_ok=True)
        s = open("src/" + src).read()
        for a, b in edits:
            assert s.count(a) == 1, (name, a)
            s = s.replace(a, b)
        open("%s/%s" % (d, src), "w").write(s)
        obj = "%s/%s" % (d, src.replace(".c", ".o"))
        r = sh(["cc"] + CF + ["-Isrc", "-c", "%s/%s" % (d, src), "-o", obj])
        if r.returncode:
            print(name, "COMPILE FAIL", r.stderr[:500]); continue
        shutil.copy(B + "/libadelefeld.a", d + "/lib.a")
        sh(["ar", "r", d + "/lib.a", obj])
        res = []
        for t in TESTS:
            exe = "%s/%s" % (d, t)
            r = sh(["cc", "%s/%s.o" % (FD, t)] + sup + [d + "/lib.a", "-lflint", "-lgmp", "-lm", "-o", exe])
            if r.returncode:
                res.append("%s:LINKFAIL" % t); continue
            t0 = time.time()
            try:
                r = sh(["timeout", "170", exe], timeout=175)
                rc = r.returncode
            except subprocess.TimeoutExpired:
                rc = "TO"
            res.append("%s:%s" % (t.replace("test_", ""), "pass" if rc == 0 else "FAIL(%s)" % rc))
            os.remove(exe)
        print(name, " ".join(res), flush=True)


main()
