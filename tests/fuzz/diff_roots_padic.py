#!/usr/bin/env python3
"""tests/fuzz/diff_roots_padic.py: differential run of adf_roots_padic and adf_roots_padic_partial (slice 2 of S.2,
lane s2-slice2) against rootlist_padic of proto/solvers_checks.py (line 1707; Algorithm P, padic_roots, line 1649);
since slice 4 (lane s2-slice4) also at primes above ADF_ROOTS_P_EVAL_MAX = 128.

    sh tests/test_exports.sh                     # builds build/libadelefeld.so
    python3 tests/fuzz/diff_roots_padic.py --seconds 180 --seed 1

Random polynomials of degree 0 to 7: coefficients in small and in large ranges (up to 200 bits), products with
planted integer roots (repeated roots, pairs of roots congruent modulo p^t, t up to 5, a leading coefficient p or a
content), now and then a factor without roots. Primes: every prime up to 101, 2 and 3 more often, and (slice 4)
the primes from 131 to 1031, where the library finds the roots modulo p by the degree of gcd(h, X^p - X) and the
candidates of FLINT (roots.h) and the reference by evaluation at every residue. Precisions 1 to 12, sometimes 40,
now and then 0 or negative (DOMAIN); depths 0 to 12, now and then negative (DOMAIN). The same input goes to the two
C functions (ctypes on build/libadelefeld.so) and to the reference. The run asserts, for every call:
  - the status of adf_roots_padic is the reference status (OK 0, NOT_DETERMINED 1, DOMAIN 7), and that of
    adf_roots_padic_partial is OK (DOMAIN when the reference says DOMAIN);
  - the partial list equals the reference list: g (adf_rootlist_get_poly), reduced, complete, the certificates
    (adf_rootlist_get_cert) and the unresolved classes (adf_rootlist_get_unresolved), in order; scope PARTITION;
  - adf_rootlist_verify_entries and adf_rootlist_is_canonical accept it, and adf_rootlist_verify_complete at the
    same depth returns complete;
  - for OK the strict list equals the partial list.
Primes of 21 to 64 bits (slice 4; the reference evaluates at every residue and cannot run there): a fixed list
(2^20 + 7, 2^32 - 5, 2^48 - 59, 2^63 - 25, 2^64 - 59) and random primes of 21 to 64 bits (Miller-Rabin with the
first twelve primes as bases, deterministic below 3.3 10^24, and adf_place_prime proves them again). The inputs
are products with planted integer roots of up to 100 bits, distinct modulo p except one pair r1 = r0 + p^t u
(t = 1 to 3, 0 < |u| <= 1000 < p), now and then a repeated root, a leading coefficient 2, 3 or p, a content, and
a factor X^2 - c with c a non-residue modulo p (Euler's criterion). The oracle is an argument, not the reference:
the roots in Z_p of such a product are exactly the planted roots (Z_p is an integral domain; X^2 - c has no root
modulo p). A class polynomial modulo p is a unit times the product of Y - (digit) over the planted roots in the
class, so a class is opened only when two planted roots share its next digit, and the list is complete exactly
when depth >= T, T = v_p(r1 - r0) for the pair and 0 without one. The run asserts: the statuses (DOMAIN as the
arguments say; strict OK exactly when depth >= T, else NOT_DETERMINED; partial OK); g is normalise_g of the
reference; every certificate is (r mod p^K, K, s) for a planted root r, s = v_p(g'(r)), K = max(prec, s + 1), in
increasing order; every planted root lies in exactly one ball or class; complete lists hold all planted roots;
verify_entries and is_canonical accept the list, and verify_complete returns complete.
It prints the counts of each status, of lists with classes and with s > 0, and of the calls by the size of p.
Exit status 1 on the first disagreement. A run of 180 s is a smoke test, not a long fuzz run."""
import argparse
import ctypes
import os
import random
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "proto"))
import solvers_checks as S  # noqa: E402

STATUS = {0: "OK", 1: "NOT_DETERMINED", 7: "DOMAIN"}
SIZEOF_ROOTLIST = 120                     # include/adelefeld/roots.h, the layout
OFF_REDUCED = 12
PRIMES = [q for q in range(2, 102) if all(q % r for r in range(2, q))]
PRIMES_MID = [q for q in range(131, 1032) if all(q % r for r in range(2, int(q ** 0.5) + 1))]
PRIMES_BIG = [2 ** 20 + 7, 2 ** 32 - 5, 2 ** 48 - 59, 2 ** 63 - 25, 2 ** 64 - 59]


def is_prime_word(n):
    """Miller-Rabin with the first twelve primes as bases: deterministic for n < 3.3 10^24."""
    if n < 2:
        return False
    small = (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37)
    for q in small:
        if n % q == 0:
            return n == q
    d, r = n - 1, 0
    while d % 2 == 0:
        d, r = d // 2, r + 1
    for a in small:
        x = pow(a, d, n)
        if x in (1, n - 1):
            continue
        for _ in range(r - 1):
            x = x * x % n
            if x == n - 1:
                break
        else:
            return False
    return True


assert all(is_prime_word(q) for q in PRIMES_BIG) and all(is_prime_word(q) == (q in PRIMES) for q in range(2, 102))


class Place(ctypes.Structure):
    _fields_ = [("opaque", ctypes.c_ulong)]


class Bridge:
    def __init__(self, path):
        self.flint = ctypes.CDLL("libflint.so.18", mode=ctypes.RTLD_GLOBAL)
        self.lib = ctypes.CDLL(path)
        f, lib = self.flint, self.lib
        vp = ctypes.c_void_p
        lp = ctypes.POINTER(ctypes.c_long)
        f.fmpz_set_str.argtypes = [vp, ctypes.c_char_p, ctypes.c_int]
        f.fmpz_get_str.argtypes = [ctypes.c_char_p, ctypes.c_int, vp]
        f.fmpz_get_str.restype = vp
        f.flint_free.argtypes = [vp]
        f.fmpz_init.argtypes = [vp]
        f.fmpz_poly_init.argtypes = [vp]
        f.fmpz_poly_zero.argtypes = [vp]
        f.fmpz_poly_set_coeff_fmpz.argtypes = [vp, ctypes.c_long, vp]
        f.fmpz_poly_get_coeff_fmpz.argtypes = [vp, vp, ctypes.c_long]
        f.fmpz_poly_length.argtypes = [vp]
        f.fmpz_poly_length.restype = ctypes.c_long
        lib.adf_rootlist_init.argtypes = [vp]
        lib.adf_place_prime.argtypes = [ctypes.POINTER(Place), ctypes.c_ulong]
        lib.adf_place_prime.restype = ctypes.c_int
        for n in ("adf_roots_padic", "adf_roots_padic_partial"):
            getattr(lib, n).argtypes = [vp, vp, Place, ctypes.c_long, ctypes.c_long]
            getattr(lib, n).restype = ctypes.c_int
        lib.adf_rootlist_get_cert.argtypes = [vp, lp, lp, vp, ctypes.c_long]
        lib.adf_rootlist_get_cert.restype = ctypes.c_int
        lib.adf_rootlist_get_unresolved.argtypes = [vp, lp, vp, ctypes.c_long]
        lib.adf_rootlist_get_unresolved.restype = ctypes.c_int
        lib.adf_rootlist_get_poly.argtypes = [vp, vp]
        lib.adf_rootlist_verify_entries.argtypes = [vp, vp]
        lib.adf_rootlist_verify_entries.restype = ctypes.c_int
        lib.adf_rootlist_verify_complete.argtypes = [vp, vp, ctypes.c_long]
        lib.adf_rootlist_verify_complete.restype = ctypes.c_int
        for n in ("adf_rootlist_is_canonical", "adf_rootlist_scope", "adf_rootlist_is_complete"):
            getattr(lib, n).argtypes = [vp]
            getattr(lib, n).restype = ctypes.c_int
        for n in ("adf_rootlist_length", "adf_rootlist_unresolved_length"):
            getattr(lib, n).argtypes = [vp]
            getattr(lib, n).restype = ctypes.c_long
        lib.adf_sizeof_rootlist.restype = ctypes.c_size_t
        assert lib.adf_sizeof_rootlist() == SIZEOF_ROOTLIST
        self.LS = ctypes.create_string_buffer(SIZEOF_ROOTLIST)
        self.LP = ctypes.create_string_buffer(SIZEOF_ROOTLIST)
        lib.adf_rootlist_init(self.LS)
        lib.adf_rootlist_init(self.LP)
        self.f = ctypes.create_string_buffer(24)
        self.g = ctypes.create_string_buffer(24)
        f.fmpz_poly_init(self.f)
        f.fmpz_poly_init(self.g)
        self.z = ctypes.c_long(0)
        f.fmpz_init(ctypes.byref(self.z))

    def set_fmpz(self, addr, n):
        assert self.flint.fmpz_set_str(addr, str(n).encode(), 10) == 0

    def get_fmpz(self, addr):
        p = self.flint.fmpz_get_str(None, 10, addr)
        s = ctypes.string_at(p).decode()
        self.flint.flint_free(p)
        return int(s)

    def read(self, L, depth):
        fl, lib = self.flint, self.lib
        res = {}
        n = lib.adf_rootlist_length(L)
        nu = lib.adf_rootlist_unresolved_length(L)
        K, s = ctypes.c_long(0), ctypes.c_long(0)
        certs, unres = [], []
        for i in range(n):
            assert lib.adf_rootlist_get_cert(ctypes.byref(self.z), ctypes.byref(K), ctypes.byref(s), L, i) == 1
            certs.append((self.get_fmpz(ctypes.byref(self.z)), K.value, s.value))
        for i in range(nu):
            assert lib.adf_rootlist_get_unresolved(ctypes.byref(self.z), ctypes.byref(K), L, i) == 1
            unres.append((self.get_fmpz(ctypes.byref(self.z)), K.value))
        res["certs"], res["unres"] = certs, unres
        lib.adf_rootlist_get_poly(self.g, L)
        g = []
        for i in range(fl.fmpz_poly_length(self.g)):
            fl.fmpz_poly_get_coeff_fmpz(ctypes.byref(self.z), self.g, i)
            g.append(self.get_fmpz(ctypes.byref(self.z)))
        res["g"] = g
        res["reduced"] = ctypes.c_int.from_buffer(L, OFF_REDUCED).value
        res["scope"] = lib.adf_rootlist_scope(L)
        res["complete"] = lib.adf_rootlist_is_complete(L)
        res["verify"] = lib.adf_rootlist_verify_entries(L, self.f)
        res["canonical"] = lib.adf_rootlist_is_canonical(L)
        res["vc"] = lib.adf_rootlist_verify_complete(L, self.f, depth)
        return res

    def run(self, f, p, prec, depth):
        fl, lib = self.flint, self.lib
        fl.fmpz_poly_zero(self.f)
        for i, c in enumerate(f):
            self.set_fmpz(ctypes.byref(self.z), c)
            fl.fmpz_poly_set_coeff_fmpz(self.f, i, ctypes.byref(self.z))
        v = Place(0)
        assert lib.adf_place_prime(ctypes.byref(v), p) == 0
        st = lib.adf_roots_padic(self.LS, self.f, v, prec, depth)
        stp = lib.adf_roots_padic_partial(self.LP, self.f, v, prec, depth)
        out = {"status": st, "status_partial": stp}
        if stp == 0:
            out["partial"] = self.read(self.LP, depth)
        if st == 0:
            out["strict"] = self.read(self.LS, depth)
        return out


def draw_poly(rng, p):
    u = rng.random()
    if u < 0.25:                                              # random coefficients
        big = rng.random() < 0.15
        f = [rng.randrange(-2 ** 200, 2 ** 200) if big else rng.randint(-9, 9) for _ in range(rng.randint(1, 8))]
        return S.ptrim(f) or [rng.randint(1, 5)]
    roots = [rng.randrange(-10 ** 6, 10 ** 6) if rng.random() < 0.3 else rng.randrange(-p ** 3 - 5, p ** 3 + 5)
             for _ in range(rng.randint(1, 5))]
    if len(roots) >= 2 and rng.random() < 0.5:
        roots[1] = roots[0] + p ** rng.randint(1, 5 if p < 10 else 3) * rng.choice((1, -1, 2, 3))
    if len(roots) >= 3 and rng.random() < 0.3:
        roots[2] = roots[0]                                   # a repeated root
    f = S.pfrom_roots(roots, lead=rng.choice((1, 1, 2, 3, p)))
    if rng.random() < 0.3:
        f = S.pmul(f, rng.choice(([1, 0, 1], [1, 1, 1], [2, 0, 1], [-3, 0, 1], [p, 0, 1], [-p, 0, 1])))
    if rng.random() < 0.15:
        f = [c * rng.choice((p, 6, -1)) for c in f]           # content, sign
    if rng.random() < 0.02:
        f = [0] * rng.randint(0, 3)                           # the zero polynomial
    return f


def vp(x, p):
    v = 0
    while x % p == 0:
        x, v = x // p, v + 1
    return v


def draw_big_prime(rng):
    if rng.random() < 0.5:
        return rng.choice(PRIMES_BIG)
    while True:
        q = rng.randrange(2 ** (rng.randint(21, 64) - 1), 2 ** 64) | 1
        if q < 2 ** 64 and is_prime_word(q):
            return q


def non_residue(p, rng):
    while True:
        c = rng.randrange(1, p)
        if pow(c, (p - 1) // 2, p) == p - 1:
            return c


def draw_planted(rng, p):
    """f with the planted roots `roots` (distinct), and T (see the docstring)."""
    n = rng.randint(1, 6)
    roots = []
    while len(roots) < n:
        r = rng.randrange(-2 ** 100, 2 ** 100) if rng.random() < 0.5 else rng.randrange(-p, 2 * p)
        if all((r - q) % p for q in roots):
            roots.append(r)
    T = 0
    if n >= 2 and rng.random() < 0.3:
        T = rng.randint(1, 3)
        roots[1] = roots[0] + p ** T * rng.randint(1, 1000) * rng.choice((1, -1))
    mult = list(roots)
    if rng.random() < 0.3:
        mult.append(roots[0])                                 # a repeated root
    f = S.pfrom_roots(mult, lead=rng.choice((1, 1, 2, 3, p)))
    if rng.random() < 0.4:
        f = S.pmul(f, [-non_residue(p, rng), 0, 1])
    if rng.random() < 0.15:
        f = [c * rng.choice((p, 6, -1)) for c in f]           # content, sign
    return f, roots, T


def check_big(br, rng, n):
    """One call at a prime of 21 to 64 bits against the planted oracle; returns (fails, where, status, info)."""
    p = draw_big_prime(rng)
    f, roots, T = draw_planted(rng, p)
    prec = draw_prec(rng)
    depth = rng.randint(0, 4) if rng.random() > 0.03 else -1
    where = f"case {n}: f={f} p={p} prec={prec} depth={depth} roots={roots}"[:1500]
    got = br.run(f, p, prec, depth)
    fails = []
    if prec < 1 or depth < 0:
        if got["status"] != 7 or got["status_partial"] != 7:
            fails.append(f"statuses {got['status']}, {got['status_partial']}; want DOMAIN")
        return fails, where, S.DOMAIN, None
    complete = depth >= T
    want_st = 0 if complete else 1
    if got["status"] != want_st or got["status_partial"] != 0:
        fails.append(f"statuses {got['status']}, {got['status_partial']}; want {want_st}, 0 (T = {T})")
        return fails, where, None, None
    P = got["partial"]
    g = S.normalise_g(f)
    dg = S.pderiv(g)
    want = []
    for r in roots:
        s = vp(S.peval(dg, r), p)
        K = max(prec, s + 1)
        want.append((r % p ** K, K, s))
    want.sort()
    if P["g"] != g or P["scope"] != 0 or P["complete"] != (1 if complete else 0):
        fails.append(f"g = {P['g']}, scope {P['scope']}, complete {P['complete']}; oracle {g}, {complete}")
    for c in P["certs"]:
        if c not in want:
            fails.append(f"certificate {c} is not one of the oracle {want}")
    if P["certs"] != sorted(P["certs"]):
        fails.append("certificates not in increasing order")
    for r in roots:
        m = sum((r - a) % p ** K == 0 for a, K, _ in P["certs"]) + sum((r - a) % p ** e == 0 for a, e in P["unres"])
        if m != 1:
            fails.append(f"the planted root {r} lies in {m} balls and classes")
    if complete and P["certs"] != want:
        fails.append(f"certificates {P['certs']}, oracle {want}")
    if P["verify"] != 1 or P["canonical"] != 1 or P["vc"] != (1 if complete else 0):
        fails.append(f"verify_entries {P['verify']}, is_canonical {P['canonical']}, verify_complete {P['vc']}")
    if complete and got.get("strict") != P:
        fails.append("the strict list differs from the partial list")
    return fails, where, S.OK if complete else S.NOT_DETERMINED, (bool(P["unres"]), any(c[2] > 0 for c in want))


def draw_prec(rng):
    u = rng.random()
    if u < 0.03:
        return rng.choice((0, -1, -1000))
    if u < 0.1:
        return 40
    return rng.randint(1, 12)


def draw_depth(rng):
    if rng.random() < 0.03:
        return rng.choice((-1, -7))
    return rng.randint(0, 12)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seconds", type=float, default=30)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--lib", default="build/libadelefeld.so")
    args = ap.parse_args()
    if not os.path.exists(args.lib):
        print(f"{args.lib} not found: run sh tests/test_exports.sh first")
        return 2
    br = Bridge(args.lib)
    rng = random.Random(args.seed)
    t_end = time.time() + args.seconds
    by_status = {}
    n = n_classes = n_spos = n_small = n_mid = n_big = 0
    while time.time() < t_end:
        u = rng.random()
        if u < 0.3:                                           # slice 4: 21 to 64 bits, the planted oracle
            n += 1
            n_big += 1
            fails, where, st, info = check_big(br, rng, n)
            by_status[st] = by_status.get(st, 0) + 1
            if info:
                n_classes += info[0]
                n_spos += info[1]
            if fails:
                print("DISAGREEMENT", where)
                for x in fails:
                    print("   ", x)
                return 1
            continue
        if u < 0.5:
            p = rng.choice(PRIMES_MID)                        # slice 4: the degree route against the reference
            n_mid += 1
        else:
            p = rng.choice((2, 3)) if rng.random() < 0.35 else rng.choice(PRIMES)
        n_small += p <= 7
        f = draw_poly(rng, p)
        prec, depth = draw_prec(rng), draw_depth(rng)
        n += 1
        st, L = S.rootlist_padic(f, p, prec, depth)
        got = br.run(f, p, prec, depth)
        where = f"case {n}: f={f} p={p} prec={prec} depth={depth}"[:900]
        fails = []
        want_partial = 7 if st == S.DOMAIN else 0
        if STATUS.get(got["status"]) != st or got["status_partial"] != want_partial:
            fails.append(f"statuses {got['status']}, {got['status_partial']}; reference {st}")
        elif st != S.DOMAIN:
            P = got["partial"]
            ft = S.ptrim(f)
            reduced = 1 if len(ft) > 1 and len(L["g"]) < len(ft) else 0
            if P["certs"] != sorted(L["certs"]):
                fails.append(f"certificates {P['certs']}, reference {sorted(L['certs'])}")
            if P["unres"] != sorted(L["unres"]):
                fails.append(f"classes {P['unres']}, reference {sorted(L['unres'])}")
            if P["g"] != L["g"] or P["reduced"] != reduced:
                fails.append(f"g = {P['g']}, reduced {P['reduced']}; reference {L['g']}, {reduced}")
            if P["complete"] != L["complete"] or P["scope"] != 0:
                fails.append(f"complete {P['complete']}, scope {P['scope']}; reference {L['complete']}")
            if P["verify"] != 1 or P["canonical"] != 1 or P["vc"] != L["complete"]:
                fails.append(f"verify_entries {P['verify']}, is_canonical {P['canonical']}, "
                             f"verify_complete {P['vc']}")
            if st == S.OK and got.get("strict") != P:
                fails.append("the strict list differs from the partial list")
            n_classes += bool(L["unres"])
            n_spos += any(s > 0 for _, _, s in L["certs"])
        by_status[st] = by_status.get(st, 0) + 1
        if fails:
            print("DISAGREEMENT", where)
            for x in fails:
                print("   ", x)
            return 1
    print(f"diff_roots_padic: {n} calls in {args.seconds:.0f} s, seed {args.seed}; statuses {by_status}; "
          f"at p <= 7: {n_small}; at 131 <= p <= 1031: {n_mid}; at p of 21 to 64 bits: {n_big}; "
          f"lists with classes {n_classes}, with s > 0 {n_spos}; 0 disagreements")
    return 0


if __name__ == "__main__":
    sys.exit(main())
