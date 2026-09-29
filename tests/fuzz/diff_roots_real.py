#!/usr/bin/env python3
"""tests/fuzz/diff_roots_real.py: differential run of adf_roots_real (slice 3 of S.2, lane s2-slice3) against
real_roots_ref of proto/solvers_checks.py (line 2555: Algorithm RR in the reference, with its own Sturm count and
isolation by bisection).

    sh tests/test_exports.sh                     # builds build/libadelefeld.so
    python3 tests/fuzz/diff_roots_real.py --seconds 180 --seed 1

Random polynomials of degree 0 to 12: random coefficients in small and in large ranges (up to 200 bits); products
with planted rational roots (small, dyadic, large, close pairs), repeated factors, a leading coefficient, now and
then a factor without real roots; now and then the zero polynomial. Precisions 2 to 100, sometimes below 2. The
same input goes to the C function (ctypes on build/libadelefeld.so) and to the reference, whose enclosures are
computed at 64 bits (much narrower than the gaps, so that the one-to-one test below is meaningful). The run
asserts, for every call:
  - the status is the reference status: DOMAIN for f = 0, OK otherwise (the C function may return NOT_DETERMINED
    by its contract; the run counts that as a disagreement, since the reference never needs it);
  - the list is at the real place, including the empty list for a constant;
  - the list: g (adf_rootlist_get_poly) and reduced equal those of the reference, n = count = the reference count;
  - the C balls (adf_rootlist_get_arb, exact end points by arb_get_interval_fmpz_2exp) overlap the reference
    enclosures one to one: C ball i meets enclosure j exactly when i = j;
  - every C ball has arb_rel_accuracy_bits >= max(prec, 2) or is exact;
  - adf_rootlist_is_canonical, adf_rootlist_verify_entries and adf_rootlist_verify_complete accept the list.
It prints the counts of each status, of roots, of exact balls and of reduced inputs. Exit status 1 on the first
disagreement. The reference replaces S.sturm_chain by a memoised wrapper of the same function (speed only).

Lane r-slice1 (the candidates of src/roots_real.c) added: (1) the same input also goes to real_roots of
proto/real_isolation.py, the reference of docs/design/real-roots.md, which runs the same algorithm in Python
integers; the C balls must equal its balls exactly (every end point), unless --no-same; (2) the families of the
review finding (roots 2^e and 2^e + 1), clusters 2^e + i/3 and close pairs 1/3 and 1/3 + 2^-e / 3, with e up to
2000 (--big-e), and planted dyadic roots."""
import argparse
import ctypes
import os
import random
import sys
import time
from fractions import Fraction as F

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "proto"))
import solvers_checks as S  # noqa: E402
import real_isolation as RI  # noqa: E402

_chain = S.sturm_chain
_memo = {}


def _sturm_chain_memo(g):
    key = tuple(F(c) for c in g)
    if key not in _memo:
        if len(_memo) > 64:
            _memo.clear()
        _memo[key] = _chain(g)
    return _memo[key]


S.sturm_chain = _sturm_chain_memo

STATUS = {0: "OK", 1: "NOT_DETERMINED", 7: "DOMAIN", 10: "LIMIT"}
SIZEOF_ROOTLIST = 120                     # include/adelefeld/roots.h, the layout
OFF_REDUCED = 12
OFF_COUNT = 112
SIZEOF_ARB = 48                           # FLINT 3.0.1: arf 32 bytes, mag 16 bytes
REF_BITS = 64


class Place(ctypes.Structure):
    _fields_ = [("opaque", ctypes.c_ulong)]


class Bridge:
    def __init__(self, path):
        self.flint = ctypes.CDLL("libflint.so.18", mode=ctypes.RTLD_GLOBAL)
        self.lib = ctypes.CDLL(path)
        f, lib = self.flint, self.lib
        vp = ctypes.c_void_p
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
        f.arb_init.argtypes = [vp]
        f.arb_get_interval_fmpz_2exp.argtypes = [vp, vp, vp, vp]
        f.arb_rel_accuracy_bits.argtypes = [vp]
        f.arb_rel_accuracy_bits.restype = ctypes.c_long
        f.arb_is_exact.argtypes = [vp]
        f.arb_is_exact.restype = ctypes.c_int
        lib.adf_rootlist_init.argtypes = [vp]
        lib.adf_roots_real.argtypes = [vp, vp, ctypes.c_long]
        lib.adf_roots_real.restype = ctypes.c_int
        lib.adf_rootlist_get_arb.argtypes = [vp, vp, ctypes.c_long]
        lib.adf_rootlist_get_arb.restype = ctypes.c_int
        lib.adf_rootlist_get_poly.argtypes = [vp, vp]
        lib.adf_rootlist_place.argtypes = [vp]
        lib.adf_rootlist_place.restype = Place
        lib.adf_place_is_archimedean.argtypes = [Place]
        lib.adf_place_is_archimedean.restype = ctypes.c_int
        lib.adf_rootlist_verify_entries.argtypes = [vp, vp]
        lib.adf_rootlist_verify_entries.restype = ctypes.c_int
        lib.adf_rootlist_verify_complete.argtypes = [vp, vp, ctypes.c_long]
        lib.adf_rootlist_verify_complete.restype = ctypes.c_int
        for n in ("adf_rootlist_is_canonical", "adf_rootlist_scope", "adf_rootlist_is_complete"):
            getattr(lib, n).argtypes = [vp]
            getattr(lib, n).restype = ctypes.c_int
        lib.adf_rootlist_length.argtypes = [vp]
        lib.adf_rootlist_length.restype = ctypes.c_long
        lib.adf_sizeof_rootlist.restype = ctypes.c_size_t
        assert lib.adf_sizeof_rootlist() == SIZEOF_ROOTLIST
        self.L = ctypes.create_string_buffer(SIZEOF_ROOTLIST)
        lib.adf_rootlist_init(self.L)
        self.f = ctypes.create_string_buffer(24)
        self.g = ctypes.create_string_buffer(24)
        f.fmpz_poly_init(self.f)
        f.fmpz_poly_init(self.g)
        self.x = ctypes.create_string_buffer(SIZEOF_ARB)
        f.arb_init(self.x)
        self.z = [ctypes.c_long(0) for _ in range(3)]
        for z in self.z:
            f.fmpz_init(ctypes.byref(z))

    def set_fmpz(self, addr, n):
        assert self.flint.fmpz_set_str(addr, str(n).encode(), 10) == 0

    def get_fmpz(self, addr):
        p = self.flint.fmpz_get_str(None, 10, addr)
        s = ctypes.string_at(p).decode()
        self.flint.flint_free(p)
        return int(s)

    def run(self, f, prec):
        fl, lib = self.flint, self.lib
        fl.fmpz_poly_zero(self.f)
        z = ctypes.byref(self.z[0])
        for i, c in enumerate(f):
            self.set_fmpz(z, c)
            fl.fmpz_poly_set_coeff_fmpz(self.f, i, z)
        st = lib.adf_roots_real(self.L, self.f, prec)
        out = {"status": st}
        if st != 0:
            return out
        L = self.L
        n = lib.adf_rootlist_length(L)
        balls, acc_ok, exact = [], True, 0
        za, zb, ze = (ctypes.byref(t) for t in self.z)
        for i in range(n):
            assert lib.adf_rootlist_get_arb(self.x, L, i) == 1
            fl.arb_get_interval_fmpz_2exp(za, zb, ze, self.x)
            a, b, e = (self.get_fmpz(t) for t in (za, zb, ze))
            s = F(2) ** e
            balls.append((a * s, b * s))
            ex = fl.arb_is_exact(self.x)
            exact += ex
            if not ex and fl.arb_rel_accuracy_bits(self.x) < max(prec, 2):
                acc_ok = False
        lib.adf_rootlist_get_poly(self.g, L)
        g = []
        for i in range(fl.fmpz_poly_length(self.g)):
            fl.fmpz_poly_get_coeff_fmpz(z, self.g, i)
            g.append(self.get_fmpz(z))
        out.update(place_is_real=lib.adf_place_is_archimedean(lib.adf_rootlist_place(L)), balls=balls, acc_ok=acc_ok, exact=exact, g=g, n=n,
                   reduced=ctypes.c_int.from_buffer(L, OFF_REDUCED).value,
                   count=ctypes.c_long.from_buffer(L, OFF_COUNT).value,
                   scope=lib.adf_rootlist_scope(L), complete=lib.adf_rootlist_is_complete(L),
                   canonical=lib.adf_rootlist_is_canonical(L), ve=lib.adf_rootlist_verify_entries(L, self.f),
                   vc=lib.adf_rootlist_verify_complete(L, self.f, 0))
        return out


def rand_root(rng):
    u = rng.random()
    if u < 0.3:
        return F(rng.randint(-30, 30))
    if u < 0.5:
        return F(rng.randint(-2 ** 12, 2 ** 12), 2 ** rng.randint(1, 40))        # dyadic
    if u < 0.7:
        return F(rng.randint(-99, 99), rng.randint(1, 99))
    if u < 0.85:
        return F(rng.randrange(-10 ** 30, 10 ** 30), rng.choice((1, 3, 7)))       # large
    return F(rng.randint(-5, 5), 10 ** rng.randint(5, 25))                       # near 0


BIG_E = 2000


def draw_poly(rng):
    u = rng.random()
    if u < 0.08:                                                                 # the families of lane r-slice1
        e = rng.randint(1, BIG_E)
        v = rng.random()
        if v < 0.3:
            return S.pmul([rng.choice((1, 1234567, -3))], S.pmul([-2 ** e, 1], [-2 ** e - 1, 1]))
        if v < 0.6:
            f = [1]
            for i in range(1, rng.randint(2, 4) + 1):
                f = S.pmul(f, [-3 * 2 ** e - i, 3])
            return f
        if v < 0.8:
            return S.pmul([-1, 3], [-2 ** e - 1, 3 * 2 ** e])
        f = [rng.choice((1, -5))]
        for _ in range(rng.randint(1, 4)):                                       # dyadic roots, some repeated
            m, t = rng.randint(-2 ** 20, 2 ** 20), rng.randint(-60, 60)
            r = F(m) * F(2) ** t
            f = S.pmul(f, [-r.numerator, r.denominator])
        return f
    if u < 0.1:
        return [0] * rng.randint(0, 3)                                           # the zero polynomial
    if u < 0.3:
        big = rng.random() < 0.3
        f = [rng.randrange(-2 ** 200, 2 ** 200) if big else rng.randint(-50, 50) for _ in range(rng.randint(1, 13))]
        return S.ptrim(f) or [rng.randint(1, 9)]
    roots = [rand_root(rng) for _ in range(rng.randint(1, 5))]
    if len(roots) >= 2 and rng.random() < 0.3:
        roots[1] = roots[0] + F(1, 2 ** rng.randint(10, 60)) * rng.choice((1, -1, 3))  # a close pair
    f = [rng.choice((1, -1, 2, 3, -7, 2 ** 100 + 1))]
    deg = 0
    for r in roots:
        for _ in range(rng.choice((1, 1, 1, 2, 3))):                             # repeated factors
            if deg >= 12:
                break
            f = S.pmul(f, [-r.numerator, r.denominator])
            deg += 1
    if deg <= 10 and rng.random() < 0.3:
        f = S.pmul(f, rng.choice(([1, 0, 1], [1, 1, 1], [2, -1, 3], [5, 0, 1])))
    return f


def draw_prec(rng):
    u = rng.random()
    if u < 0.05:
        return rng.choice((1, 0, -1, -1000))
    return rng.choice((2, 2, 5, 10, 20, 30, 53, 64, 100))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seconds", type=float, default=30)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--lib", default="build/libadelefeld.so")
    ap.add_argument("--no-same", action="store_true", help="do not compare with proto/real_isolation.py")
    ap.add_argument("--big-e", type=int, default=BIG_E)
    args = ap.parse_args()
    globals()["BIG_E"] = args.big_e
    if not os.path.exists(args.lib):
        print(f"{args.lib} not found: run sh tests/test_exports.sh first")
        return 2
    br = Bridge(args.lib)
    rng = random.Random(args.seed)
    t_end = time.time() + args.seconds
    by_status = {}
    n = n_roots = n_exact = n_reduced = n_none = n_same = n_same_fail = 0
    while time.time() < t_end:
        f = draw_poly(rng)
        prec = draw_prec(rng)
        n += 1
        ft = S.ptrim(f)
        got = br.run(f, prec)
        where = f"case {n}: f={f} prec={prec}"[:1200]
        fails = []
        if not ft:
            want = "DOMAIN"
            if STATUS.get(got["status"]) != want:
                fails.append(f"status {got['status']}, reference DOMAIN")
        else:
            want = "OK"
            st, cnt, enc = S.real_roots_ref(ft, REF_BITS)
            g = S.squarefree_part(ft)
            reduced = 1 if len(ft) > 1 and len(g) < len(ft) else 0
            if st != S.OK:
                fails.append(f"the reference returned {st}")
            elif got["status"] != 0:
                fails.append(f"status {STATUS.get(got['status'], got['status'])}, reference OK")
            else:
                if got["g"] != g or got["reduced"] != reduced:
                    fails.append(f"g {got['g']}, reduced {got['reduced']}; reference {g}, {reduced}")
                if got["n"] != cnt or got["count"] != cnt:
                    fails.append(f"n {got['n']}, count {got['count']}; reference count {cnt}")
                else:
                    for i, (clo, chi) in enumerate(got["balls"]):
                        for j, (elo, ehi) in enumerate(enc):
                            meet = clo <= ehi and elo <= chi
                            if meet != (i == j):
                                fails.append(f"C ball {i} [{clo}, {chi}] and enclosure {j} [{elo}, {ehi}]: "
                                             f"meet {meet}")
                if not args.no_same:
                    st2, n2, balls2, _ = RI.real_roots(ft, prec)
                    if st2 != RI.OK or balls2 != got["balls"]:
                        fails.append(f"C balls {got['balls']} differ from proto/real_isolation.py {st2} {balls2}")
                        n_same_fail += 1
                    else:
                        n_same += 1
                if got["place_is_real"] != 1:
                    fails.append("list is not at the real place")
                if not got["acc_ok"]:
                    fails.append("a ball below the accuracy max(prec, 2)")
                if got["scope"] != 0 or got["complete"] != 1:
                    fails.append(f"scope {got['scope']}, complete {got['complete']}")
                if got["canonical"] != 1 or got["ve"] != 1 or got["vc"] != 1:
                    fails.append(f"is_canonical {got['canonical']}, verify_entries {got['ve']}, "
                                 f"verify_complete {got['vc']}")
                n_roots += got["n"]
                n_exact += got["exact"]
                n_reduced += reduced
                n_none += got["n"] == 0
        by_status[want] = by_status.get(want, 0) + 1
        if fails:
            print("DISAGREEMENT", where)
            for x in fails[:10]:
                print("   ", str(x)[:600])
            return 1
    print(f"diff_roots_real: {n} calls in {args.seconds:.0f} s, seed {args.seed}; statuses {by_status}; roots "
          f"{n_roots}, exact balls {n_exact}, lists without a root {n_none}, reduced inputs {n_reduced}; "
          f"lists equal to proto/real_isolation.py {n_same}; 0 disagreements")
    return 0


if __name__ == "__main__":
    sys.exit(main())
