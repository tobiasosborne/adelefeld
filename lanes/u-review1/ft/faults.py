import os, subprocess, sys, re
here = os.path.dirname(os.path.abspath(__file__))
root = os.path.abspath(os.path.join(here, "..", "..", ".."))
orig = open(os.path.join(here, "dump.orig.c")).read()
PLAIN = os.environ.get("FT_PLAIN") == "1"
LIB = os.path.join(root, "lanes/u-review1/build/libadelefeld.a" if PLAIN else "lanes/u-review1/build-inv/libadelefeld.a")
SUP = os.path.join(here, "support" + ("-plain" if os.environ.get("FT_PLAIN") == "1" else ""))
os.makedirs(SUP, exist_ok=True)
FL = ["-std=c11", "-O1", "-g"] + ([] if PLAIN else ["-DADF_CHECK_INVARIANTS"]) + [ "-I" + os.path.join(root, "include"), "-I" + os.path.join(root, "tests"), "-I" + os.path.join(root, "src")]

FAULTS = [
    ("F00 baseline, no change", "static int\ndp_hexnz(char c)", "static int\ndp_hexnz(char c)", 1),
    ("F01 class real ball positivity dropped (idclass accepts a ball that contains 0 or is negative)",
     "dp_arb_sign(&nd->arb[0]) != 1", "dp_arb_sign(&nd->arb[0]) == 0 && 0", 1),
    ("F02 v and N exchanged in the ball form of dp_v_lb",
     "        dp_fmpz(v, lb->b);\n        dp_fmpz(N, lb->c);", "        dp_fmpz(v, lb->c);\n        dp_fmpz(N, lb->b);", 1),
    ("F03 primality test skipped", "|| p < 2 || !n_is_prime(p))", "|| p < 2)", 1),
    ("F04 strict increase of the primes made non-strict", "if (q <= prev)", "if (q < prev)", 1),
    ("F05 max_items compared with >= instead of >", "n > (size_t) lim->max_items;", "n >= (size_t) lim->max_items;", 1),
    ("F06 archimedean count 0 accepted for the idele", "(nd->narch != 1 || !dp_v_arb(&nd->arb[0]) || dp_arb_sign(&nd->arb[0]) == 0",
     "(nd->narch > 1 || !dp_v_arb(&nd->arb[0]) || dp_arb_sign(&nd->arb[0]) == 0", 1),
    ("F07 max_items off by one (allows one more)", "n > (size_t) lim->max_items;", "n > (size_t) lim->max_items + 1;", 1),
    ("F08 exact form x accepts den(u) divisible by p",
     "ok = fmpz_fdiv_ui(u, p) != 0 && fmpz_fdiv_ui(N, p) != 0;", "ok = fmpz_fdiv_ui(u, p) != 0;", 1),
    ("F09 unit modulus canonicalised on load (N = 2 mod 4 halved)",
     "    if (adf_ucoset_set_fmpz2(x, c, N) != ADF_OK)\n        flint_abort();                 /* cannot happen: stage 6 checked the predicate of 5.6 */",
     "    if (adf_ucoset_set_fmpz2(x, c, N) != ADF_OK)\n        flint_abort();\n    adf_ucoset_normalise(x, x);", 1),
    ("F10 output written before the last check (lball loader touches x on DOMAIN)",
     "    adf_lball_t t;\n    size_t pos;\n    int st = dp_prepare(&P, DP_LBALL, s, len, binds, nbinds, lim);\n\n    if (st != ADF_OK)\n        return st;",
     "    adf_lball_t t;\n    size_t pos;\n    int st = dp_prepare(&P, DP_LBALL, s, len, binds, nbinds, lim);\n\n    if (st != ADF_OK)\n    {\n        if (st == ADF_DOMAIN) x->N += 1;\n        return st;\n    }", 1),
    ("F11 exact unit -1 (N = 0) refused", " || (c.n == 2 && c.p[0] == '-' && c.p[1] == '1')", "", 1),
    ("F12 radius mantissa bound 2^30 -> 2^31", "r < (UWORD(1) << MAG_BITS)", "r < (UWORD(1) << (MAG_BITS + 1))", 1),
    ("F13 radius mantissa parity check dropped", "return (r & 1) == 1 && r < (UWORD(1) << MAG_BITS);", "return r < (UWORD(1) << MAG_BITS);", 1),
    ("F14 real ball of the sball tag r not validated",
     "if (st->mode == DP_SEM && nd->sform != 0 && !dp_v_arb(&nd->arb[0]))", "if (0)", 1),
    ("F15 idele content positivity dropped", " || !dp_pos(nd->a)\n", "\n", 1),
    ("F16 ball u = 0 with v != 0 accepted (second zero rule)",
     "        if (fmpz_is_zero(u))\n            ok = fmpz_is_zero(v);\n        else\n        {\n            fmpz_sub(k, N, v);",
     "        if (fmpz_is_zero(u))\n            ok = 1;\n        else\n        {\n            fmpz_sub(k, N, v);", 1),
    ("F17 gcd(c, N) = 1 dropped in the unit coset", "        ok = fmpz_is_one(g);\n    }\n    fmpz_clear(a);\n    fmpz_clear(b);\n    fmpz_clear(g);\n    return ok;\n}\n\n/* The sign of the ball",
     "        ok = 1;\n    }\n    fmpz_clear(a);\n    fmpz_clear(b);\n    fmpz_clear(g);\n    return ok;\n}\n\n/* The sign of the ball", 1),
    ("F18 dp_arb_sign: tie of the leading bits (the repair of finding 2, not a fault; control)",
     "res = fmpz_get_ui(m) > rt ? 1 : 0;", "res = (bm > br ? fmpz_get_ui(m) >= rt : fmpz_get_ui(m) > rt) ? 1 : 0;", 1),
    ("F19 dp_arb_sign: length of the negative token (the repair of finding 1; control)",
     "    if (dp_neg(am))\n        am.p++;                        /* |m| */", "    if (dp_neg(am))\n    {\n        am.p++;\n        am.n--;\n    }", 1),
]

tests = os.environ.get("FT_TESTS", "test_dump_units,test_dump_local").split(",")


def sh(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, timeout=170, **kw)


# support objects once
sup_objs = []
for f in ["golden", "jsonl"]:
    o = os.path.join(SUP, f + ".o")
    if not os.path.exists(o):
        r = sh(["gcc"] + FL + ["-c", os.path.join(root, "tests/support", f + ".c"), "-o", o])
        if r.returncode:
            print(r.stderr[:500])
    sup_objs.append(o)

sel = sys.argv[1:]
for name, old, new, cnt in FAULTS:
    tag = name.split()[0]
    if sel and tag not in sel:
        continue
    n = orig.count(old)
    if n != 1:
        print(tag, "ANCHOR COUNT", n)
        continue
    src = orig.replace(old, new)
    p = os.path.join(here, tag + ".c")
    open(p, "w").write(src)
    o = os.path.join(here, tag + ".o")
    r = sh(["gcc"] + FL + ["-Wno-unused", "-c", p, "-o", o])
    if r.returncode:
        print(tag, "DOES NOT COMPILE", r.stderr[:300])
        continue
    res = []
    for t in tests:
        exe = os.path.join(here, tag + "_" + t)
        r = sh(["gcc"] + FL + [os.path.join(root, "tests", t + ".c"), o] + sup_objs + [LIB, "-lflint", "-lgmp", "-lm", "-pthread", "-o", exe])
        if r.returncode:
            res.append(t + ": link fail " + r.stderr[-200:])
            continue
        try:
            q = subprocess.run([exe], capture_output=True, text=True, timeout=160, cwd=root)
            last = [l for l in (q.stdout + q.stderr).split("\n") if l.strip()][-1:]
            res.append(f"{t}: rc={q.returncode} {last[0][:100] if last else ''}")
        except subprocess.TimeoutExpired:
            res.append(t + ": timeout")
        os.remove(exe)
    print(name)
    for x in res:
        print("    ", x)
    os.remove(o)
    os.remove(p)
