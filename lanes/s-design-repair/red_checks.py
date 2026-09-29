#!/usr/bin/env python3
"""Red-green probes of lane s-design-repair: one probe for each finding of the review that is shown by an input.

Usage:  python3 lanes/s-design-repair/red_checks.py <path of a solvers_checks.py>

The probes load the given file (without its main), and run each requirement of the review against it.
On the file of commit 62f15fe (`before/solvers_checks_before.py`) every RED probe must print FAIL (that is the
red log); on the repaired `proto/solvers_checks.py` every RED probe must print PASS (the green log).
Where the file has no function for a requirement (the design of 62f15fe has no seed function, no verifier of a
result), the probe uses the procedure exactly as `docs/api-s.md` of 62f15fe states it, written out below as
`literal_*`; the repaired file has the function under the name given in `NEW` and the probe then uses it.

REFUTED probes are claims of the old text that the review shows to be false: they print REFUTED when the input
refutes the claim (the claim is then replaced in the documents); they do not depend on the file.
"""
import contextlib
import io
import sys
from fractions import Fraction as F
from math import gcd

path = sys.argv[1]
src = open(path).read().rsplit('if __name__ == "__main__":', 1)[0]
ns = {"__name__": "target"}
exec(compile(src, path, "exec"), ns)
FAILED = []
INF = float("inf")


def report(kind, name, ok, detail):
    label = {"RED": "PASS" if ok else "FAIL", "REFUTED": "REFUTED" if ok else "NOT-REFUTED"}[kind]
    print(f"{label} [{kind}] {name}: {detail}")
    if kind == "RED" and not ok:
        FAILED.append(name)


def quiet(fn, *a):
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        fn(*a)
    return buf.getvalue()


# ---------------------------------------------------------------------------------------------------------
# 1. real_roots_ref at X - 10^400 (review, "Reference algorithm failure", item 1; R6)
# ---------------------------------------------------------------------------------------------------------
def probe_real_overflow():
    try:
        st, n, balls = ns["real_roots_ref"]([-10 ** 400, 1], 6)
    except Exception as err:                                        # noqa: BLE001
        return False, f"raises {type(err).__name__}"
    r = F(10 ** 400)
    ok = st == "OK" and n == 1 and len(balls) == 1 and balls[0][0] <= r <= balls[0][1]
    return ok, f"status {st}, n {n}, one ball holding the root 10^400: {len(balls) == 1 and balls[0][0] <= r <= balls[0][1]}"


# ---------------------------------------------------------------------------------------------------------
# 2. a list of real balls presented as complete with a supplied count (item 5; R7)
# ---------------------------------------------------------------------------------------------------------
def probe_real_untrusted():
    f = ns["pfrom_roots"]([1, 2, 3])
    balls = [(F(0), F(4))]
    if "real_verify_complete" in ns:                               # NEW
        got = ns["real_verify_complete"](f, balls, 1)
        got2 = ns["real_verify_complete"](f, balls, 3)
        entries = ns["real_verify_entries"](f, balls)
        return (got is False and got2 is False and entries is True), \
            f"complete(n=1) {got}, complete(n=3) {got2} (both must be False); entries {entries} (True: one root inside)"
    got = ns["real_cert_ok"](f, 1, balls)                           # literal: the checker of the old design
    return got is False, f"old checker real_cert_ok((X-1)(X-2)(X-3), n=1, [[0,4]]) returns {got}; must refuse"


# ---------------------------------------------------------------------------------------------------------
# 3. the seed function and the normalisation to g (item 2; R7): f = 27 X, p = 3, a = 0
# ---------------------------------------------------------------------------------------------------------
def literal_seed(f, p, a, kreq):
    """docs/api-s.md of 62f15fe, adf_root_padic_from_seed: the condition and s are stated with f; the list stores the
    polynomial g of the certificates (squarefree part, primitive)."""
    ptrim, pderiv, peval, val = ns["ptrim"], ns["pderiv"], ns["peval"], ns["val"]
    f = ptrim(f)
    if not f:
        return "DOMAIN", None
    fa, da = peval(f, a), peval(pderiv(f), a)
    if da == 0:
        return "NOT_DETERMINED", None
    s = val(da, p)
    vf = INF if fa == 0 else val(fa, p)
    if not vf > 2 * s:
        return "NOT_DETERMINED", None
    K = max(kreq, s + 1)
    k = int(min(vf - s, K)) if vf != INF else K
    # a certificate of precision k for f: valuation of f(a) is at least k + s
    a1 = a % p ** k
    while k < K:
        a1, k = ns["newton_step"](f, p, a1, k, s)
    return "OK", {"g": ns["squarefree_part"](f), "cert": (a1 % p ** K, K, s)}


def probe_seed():
    f, p, a = [0, 27], 3, 0
    if "seed_root" in ns:                                           # NEW
        st, L = ns["seed_root"](f, p, a, 2)
        g, (a1, K, s) = L["g"], L["certs"][0]
        ok = (st == "OK" and ns["root_cert_ok"](g, p, a1, K, s) and L["scope"] == "SEED" and L["n"] == 1
              and L["nu"] == 0 and L["complete"] == 0)
        return ok, f"status {st}, g {g}, certificate {(a1, K, s)} refers to g: {ns['root_cert_ok'](g, p, a1, K, s)}"
    st, L = literal_seed(f, p, a, 2)
    g, (a1, K, s) = L["g"], L["cert"]
    ok = ns["root_cert_ok"](g, p, a1, K, s)
    return ok, f"status {st}, stored g {g}, certificate {(a1, K, s)}: root_cert_ok for g is {ok} (s = 3, v(g'(0)) = 0)"


# ---------------------------------------------------------------------------------------------------------
# 4. NOT_UNIQUE certified by lowering B (item 7; R9): m = 2, c = 1, A = B = 1
# ---------------------------------------------------------------------------------------------------------
def literal_lowering(m, c, A, B):
    """api-s.md 62f15fe, note 4: a NOT_UNIQUE is checked by the two solutions of _reconstruct_first and of a second
    call with the bound B lowered below the first denominator."""
    st, sols, _ = ns["recon_partial"](m, c, A, B, 10 ** 6)
    if st != "NOT_UNIQUE":
        return None
    d1 = sols[0][1]
    st2, sols2, _ = ns["recon_partial"](m, c, A, d1 - 1, 10 ** 6)     # B lowered below the first denominator
    return st2 == "OK" and sols2[0] != sols[0]


def probe_not_unique_certificate():
    m, c, A, B = 2, 1, 1, 1
    if "recon_verify_result" in ns:                                 # NEW
        st, sols, cert = ns["recon_partial"](m, c, A, B, 10 ** 6)
        good = ns["recon_verify_result"](m, c, A, B, 10 ** 6, st, sols, cert)
        bad = ns["recon_verify_result"](m, c, A, B, 10 ** 6, st, sols[:1] * 2, cert)
        return (st == "NOT_UNIQUE" and good and not bad), f"verifier accepts the pair {sols}: {good}; refuses one " \
            f"solution repeated: {not bad}"
    got = literal_lowering(m, c, A, B)
    return got is True, f"the shortcut of note 4 certifies NOT_UNIQUE for (m, c, A, B) = {(m, c, A, B)}: {got} " \
        f"(both solutions 1/1 and -1/1 have denominator 1; B = 0 loses both)"


# ---------------------------------------------------------------------------------------------------------
# 5. final real accuracy (item 6; R8): relative accuracy in the sense of arb.rst:499-509
# ---------------------------------------------------------------------------------------------------------
def rel_acc(lo, hi):
    if lo == hi:
        return INF
    mid, rad = (lo + hi) / 2, (hi - lo) / 2
    if mid == 0:
        return -INF

    def top(x):                                                     # position of the top bit of x > 0
        x = abs(F(x))
        e = x.numerator.bit_length() - x.denominator.bit_length()
        return e - 1 if F(2) ** e > x else e

    return -(top(rad) - top(mid) + 1)


def probe_real_accuracy():
    worst, cases = INF, 0
    for f, bits in (([-1, 0, 1000], 20), ([-2, 0, 1], 30), ([-1, 0, 10 ** 6], 12), ([-1, 0, 4 ** 20], 8)):
        st, n, balls = ns["real_roots_ref"](f, bits)
        for lo, hi in balls:
            cases += 1
            worst = min(worst, rel_acc(F(lo), F(hi)) - max(bits, 2))
    return worst >= 0, f"{cases} balls; smallest (arb_rel_accuracy_bits - required) = {worst}"


# ---------------------------------------------------------------------------------------------------------
# 6. what a printed count counts (Other limitations): check_s3_edge, check_s3_complete, check_s2_descent
# ---------------------------------------------------------------------------------------------------------
def probe_edge_count():
    calls = []
    orig = ns["recon_partial"]

    def wrapped(*a):
        calls.append(a)
        return orig(*a)

    ns["recon_partial"] = wrapped
    try:
        out = quiet(ns["check_s3_edge"])
    finally:
        ns["recon_partial"] = orig
    import re
    printed_big = int(re.search(r"fixed cases, (\d+) large random", out).group(1))
    big_calls = len([a for a in calls if a[4] == 0])                # the large random part calls with limit 0
    ok = printed_big == big_calls
    return ok, f"printed {printed_big} large random fractions; recon_partial was called on {big_calls} of them"


def probe_complete_range():
    seen = {}
    orig = ns["recon_partial"]

    def wrapped(m, c, A, B, limit):
        seen.setdefault(m, set()).add(A)
        return orig(m, c, A, B, limit)

    ns["recon_partial"] = wrapped
    try:
        quiet(ns["check_s3_complete"])
    finally:
        ns["recon_partial"] = orig
    missing = {m: sorted(set(range(0, 2 * m + 1)) - a) for m, a in seen.items() if set(range(0, 2 * m + 1)) - a}
    bad_m = len(missing)
    return bad_m == 0, f"{len(seen)} moduli; {bad_m} of them lack some A in 0..2m (m = 36 lacks {len(missing.get(36, []))})"


def probe_descent_skip():
    hits = []
    orig = ns["approx_roots"]

    def wrapped(*a, **k):
        try:
            return orig(*a, **k)
        except OverflowError:
            hits.append(a[1:3])
            raise

    ns["approx_roots"] = wrapped
    try:
        out = quiet(ns["check_s2_descent"])
    finally:
        ns["approx_roots"] = orig
    said = "skipped" in out
    return (not hits) or said, f"{len(hits)} oracle comparisons skipped by OverflowError; output says so: {said}"


# ---------------------------------------------------------------------------------------------------------
# 7. D2.1: the prescribed associate of the zero residue (R1)
# ---------------------------------------------------------------------------------------------------------
def probe_assoc():
    def literal(a, N):
        return gcd(a, N)                                            # D2.1 of 62f15fe: "the associate of a is gcd(a, N)"

    fn = ns.get("assoc_rep", literal)
    bad = []
    for N in range(1, 40):
        for a in range(N):
            orbit = {(u * a) % N for u in range(1, N + 1) if gcd(u, N) == 1}
            if fn(a, N) != min(orbit):
                bad.append((a, N))
    return not bad, f"{len(bad)} residues a in [0, N), N < 40, where the representative differs " \
        f"from the least element of the orbit of a under the units of Z/N; first {bad[:3]}"


# ---------------------------------------------------------------------------------------------------------
# REFUTED: statements of the old text that the review refutes by an input
# ---------------------------------------------------------------------------------------------------------
def refuted_limit_zero():
    """S-D3: "with limit = 0 it is exactly '2 A B >= m' and no search"."""
    cx = []
    for m in range(1, 12):
        for c in range(m):
            for A in range(0, 2 * m + 1):
                for B in range(1, 8):
                    st = ns["recon_partial"](m, c, A, B, 0)[0]
                    if (st == "NOT_DETERMINED") != (2 * A * B >= m):
                        cx.append(((m, c, A, B), st))
    return bool(cx), f"{len(cx)} counterexamples, first {cx[:2]} (m = 2, c = 1, A = 2, B = 1 gives NOT_UNIQUE)"


def refuted_p39():
    if ns["FLINT"] is None:
        return False, "no FLINT"
    res = {}
    for name, g in (("X^2", [0, 0, 1]), ("X^3", [0, 0, 0, 1]), ("X^4", [0, 0, 0, 0, 1]), ("(X-1)^2", [1, -2, 1])):
        pol = ns["flint_poly"](g)
        ns["FLINT"].fmpz_poly_num_real_roots.restype = ns["ctypes"].c_long
        res[name] = ns["FLINT"].fmpz_poly_num_real_roots(ns["ctypes"].byref(pol))
    # claim of 62f15fe: quadratic non-squarefree returns 0; cubic and quartic non-squarefree throw
    return res["X^2"] != 0 and res["X^3"] == 3 and res["X^4"] == 4, f"FLINT 3.0.1 returns {res}"


def refuted_3_11():
    """3.11(5): "the smallest precision of a ball that isolates the root is s + 1", f = X^3 + 2X, p = 2."""
    f, p = [0, 2, 0, 1], 2
    roots = ns["approx_roots"](f, p, 12)
    st, certs, unres = ns["padic_roots"](f, p, 1, 6)
    (a, K, s), = certs
    # Z_2 (precision 0) contains exactly one root of f: the solutions modulo 2^12 all agree with 0 modulo 2^11
    one = len([x for x in roots if x % 2 ** 11 == 0]) >= 1 and all(x % 2 ** 11 == 0 for x in roots)
    return one and s == 1, f"s = {s}; solutions modulo 2^12: {sorted(roots)[:4]}...; the ball Z_2 (precision 0 < s + 1) " \
        f"holds the only root 0"


RED = [
    ("real_roots_ref at X - 10^400 (R6)", probe_real_overflow),
    ("untrusted real count [0, 4] for (X-1)(X-2)(X-3) (R7)", probe_real_untrusted),
    ("seed f = 27 X at 3, certificate refers to g (R7)", probe_seed),
    ("NOT_UNIQUE for m = 2, c = 1, A = B = 1 is certified (R9)", probe_not_unique_certificate),
    ("real accuracy is arb_rel_accuracy_bits >= max(prec, 2) (R8)", probe_real_accuracy),
    ("check_s3_edge prints the count of what was verified", probe_edge_count),
    ("check_s3_complete exhausts A in 0..2m", probe_complete_range),
    ("check_s2_descent does not skip silently", probe_descent_skip),
    ("D2.1: associate of zero is 0 (R1)", probe_assoc),
]
REFUTED = [
    ("S-D3: limit = 0 is exactly 2AB >= m", refuted_limit_zero),
    ("P3.9(1): X^2 returns 0, X^3 and X^4 throw", refuted_p39),
    ("3.11(5): s + 1 is the smallest isolating precision", refuted_3_11),
]

for name, fn in RED:
    try:
        ok, detail = fn()
    except Exception as err:                                        # noqa: BLE001
        ok, detail = False, f"raises {type(err).__name__}: {err}"
    report("RED", name, ok, detail)
for name, fn in REFUTED:
    try:
        ok, detail = fn()
    except Exception as err:                                        # noqa: BLE001
        ok, detail = False, f"raises {type(err).__name__}: {err}"
    report("REFUTED", name, ok, detail)
print(f"red probes failing: {len(FAILED)} of {len(RED)}")
sys.exit(1 if FAILED else 0)
