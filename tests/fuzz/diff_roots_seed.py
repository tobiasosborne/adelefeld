#!/usr/bin/env python3
"""tests/fuzz/diff_roots_seed.py: differential run of adf_root_padic_from_seed (slice 1 of S.2, lane s2-slice1)
against seed_root of proto/solvers_checks.py (line 1721).

    sh tests/test_exports.sh                     # builds build/libadelefeld.so
    python3 tests/fuzz/diff_roots_seed.py --seconds 180 --seed 1

Random polynomials of degree 0 to 6: coefficients in small and in large ranges (up to 200 bits), products with
planted integer roots (repeated roots, pairs of roots close at p so that s > 0, a leading coefficient p or a
content), now and then a factor without roots. Primes of four kinds: small (2 to 13), medium (17 to 997),
word primes (2^31 - 1, 2^61 - 1, 2^64 - 59, and random primes of 20 to 64 bits from FLINT's n_nextprime with
proved = 1), and p = 2 alone (solvers 3.11). Seeds: a planted root plus a multiple of p^j (j = 0 to 6, of
both signs and up to 300 bits), or a random integer. Precisions 1 to 60, sometimes 200, and now and then 0 or
negative (DOMAIN). The same input goes to the C function (ctypes on build/libadelefeld.so) and to the reference.
The run asserts, for every call:
  - the status is the reference status (OK 0, NOT_DETERMINED 1, DOMAIN 7);
  - for OK: the certificate (a', K, s) of adf_rootlist_get_cert equals the reference certificate (it is
    determined: a' is the root modulo p^K, solvers.md P3.12(2)); g (adf_rootlist_get_poly) equals the
    normalised polynomial of the reference; reduced is 1 iff deg gcd(f, f') > 0 for nonconstant f; the scope
    is SEED, n = 1, nu = 0, complete = 0; adf_rootlist_verify_entries and adf_rootlist_is_canonical accept;
    root_cert_ok of the reference accepts the C certificate.
It prints the counts of each kind of prime and of each status. Exit status 1 on the first disagreement."""
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
OFF_REDUCED, OFF_NU, OFF_COMPLETE = 12, 80, 16


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
        f.n_nextprime.argtypes = [ctypes.c_ulong, ctypes.c_int]
        f.n_nextprime.restype = ctypes.c_ulong
        lib.adf_rootlist_init.argtypes = [vp]
        lib.adf_place_prime.argtypes = [ctypes.POINTER(Place), ctypes.c_ulong]
        lib.adf_place_prime.restype = ctypes.c_int
        lib.adf_root_padic_from_seed.argtypes = [vp, vp, Place, vp, ctypes.c_long]
        lib.adf_root_padic_from_seed.restype = ctypes.c_int
        lib.adf_rootlist_get_cert.argtypes = [vp, ctypes.POINTER(ctypes.c_long), ctypes.POINTER(ctypes.c_long), vp,
                                              ctypes.c_long]
        lib.adf_rootlist_get_cert.restype = ctypes.c_int
        lib.adf_rootlist_get_poly.argtypes = [vp, vp]
        for n in ("adf_rootlist_verify_entries",):
            getattr(lib, n).argtypes = [vp, vp]
            getattr(lib, n).restype = ctypes.c_int
        for n in ("adf_rootlist_is_canonical", "adf_rootlist_scope", "adf_rootlist_is_complete"):
            getattr(lib, n).argtypes = [vp]
            getattr(lib, n).restype = ctypes.c_int
        for n in ("adf_rootlist_length", "adf_rootlist_unresolved_length"):
            getattr(lib, n).argtypes = [vp]
            getattr(lib, n).restype = ctypes.c_long
        lib.adf_sizeof_rootlist.restype = ctypes.c_size_t
        assert lib.adf_sizeof_rootlist() == SIZEOF_ROOTLIST
        self.L = ctypes.create_string_buffer(SIZEOF_ROOTLIST)
        lib.adf_rootlist_init(self.L)
        self.f = ctypes.create_string_buffer(24)
        self.g = ctypes.create_string_buffer(24)
        f.fmpz_poly_init(self.f)
        f.fmpz_poly_init(self.g)
        self.z = ctypes.c_long(0)
        self.a = ctypes.c_long(0)
        self.ap = ctypes.c_long(0)
        for x in (self.z, self.a, self.ap):
            f.fmpz_init(ctypes.byref(x))

    def set_fmpz(self, addr, n):
        assert self.flint.fmpz_set_str(addr, str(n).encode(), 10) == 0

    def get_fmpz(self, addr):
        p = self.flint.fmpz_get_str(None, 10, addr)
        s = ctypes.string_at(p).decode()
        self.flint.flint_free(p)
        return int(s)

    def nextprime(self, n):
        return self.flint.n_nextprime(n, 1)

    def seed(self, f, p, a, prec):
        fl, lib = self.flint, self.lib
        fl.fmpz_poly_zero(self.f)
        for i, c in enumerate(f):
            self.set_fmpz(ctypes.byref(self.z), c)
            fl.fmpz_poly_set_coeff_fmpz(self.f, i, ctypes.byref(self.z))
        v = Place(0)
        assert lib.adf_place_prime(ctypes.byref(v), p) == 0
        self.set_fmpz(ctypes.byref(self.a), a)
        st = lib.adf_root_padic_from_seed(self.L, self.f, v, ctypes.byref(self.a), prec)
        res = {"status": st}
        if st == 0:
            K, s = ctypes.c_long(0), ctypes.c_long(0)
            res["got_cert"] = lib.adf_rootlist_get_cert(ctypes.byref(self.ap), ctypes.byref(K), ctypes.byref(s),
                                                        self.L, 0)
            res["cert"] = (self.get_fmpz(ctypes.byref(self.ap)), K.value, s.value)
            lib.adf_rootlist_get_poly(self.g, self.L)
            g = []
            for i in range(fl.fmpz_poly_length(self.g)):
                fl.fmpz_poly_get_coeff_fmpz(ctypes.byref(self.z), self.g, i)
                g.append(self.get_fmpz(ctypes.byref(self.z)))
            res["g"] = g
            res["reduced"] = ctypes.c_int.from_buffer(self.L, OFF_REDUCED).value
            res["shape"] = (lib.adf_rootlist_scope(self.L), lib.adf_rootlist_length(self.L),
                            lib.adf_rootlist_unresolved_length(self.L), lib.adf_rootlist_is_complete(self.L))
            res["verify"] = lib.adf_rootlist_verify_entries(self.L, self.f)
            res["canonical"] = lib.adf_rootlist_is_canonical(self.L)
        return res


WORD_PRIMES = [2 ** 31 - 1, 2 ** 61 - 1, 18446744073709551557]


def draw_prime(rng, br):
    kind = rng.choice(("small", "medium", "word", "two"))
    if kind == "small":
        return kind, rng.choice((2, 3, 5, 7, 11, 13))
    if kind == "medium":
        return kind, br.nextprime(rng.randint(16, 996))
    if kind == "word":
        if rng.random() < 0.4:
            return kind, rng.choice(WORD_PRIMES)
        bits = rng.randint(20, 63)
        return kind, br.nextprime(rng.getrandbits(bits) | (1 << (bits - 1)))
    return kind, 2


def draw_poly(rng, p):
    u = rng.random()
    if u < 0.3:                                               # random coefficients
        big = rng.random() < 0.2
        f = [rng.randrange(-2 ** 200, 2 ** 200) if big else rng.randint(-9, 9) for _ in range(rng.randint(1, 7))]
        return S.ptrim(f) or [1], None
    roots = [rng.randrange(-10 ** 6, 10 ** 6) if rng.random() < 0.5 else rng.randrange(-p ** 3 - 5, p ** 3 + 5)
             for _ in range(rng.randint(1, 4))]
    if len(roots) >= 2 and rng.random() < 0.4:
        roots[1] = roots[0] + p ** rng.randint(1, 4) * rng.choice((1, -1, 3))   # close at p: s > 0
    if len(roots) >= 2 and rng.random() < 0.2:
        roots[1] = roots[0]                                   # a repeated root
    f = S.pfrom_roots(roots, lead=rng.choice((1, 1, 2, 3, p)))
    if rng.random() < 0.25:
        f = S.pmul(f, rng.choice(([1, 0, 1], [1, 1, 1], [2, 0, 1], [-3, 0, 1])))
    if rng.random() < 0.15:
        f = [c * rng.choice((p, 6, -1)) for c in f]           # content, sign
    return f, roots


def draw_seed(rng, p, roots):
    if roots and rng.random() < 0.8:
        j = rng.randint(0, 6)
        t = rng.randrange(-2 ** 300, 2 ** 300) if rng.random() < 0.2 else rng.randint(-50, 50)
        return rng.choice(roots) + t * p ** j
    return rng.randrange(-p ** 5, p ** 5)


def draw_prec(rng):
    u = rng.random()
    if u < 0.03:
        return rng.choice((0, -1, -1000))
    if u < 0.1:
        return 200
    return rng.randint(1, 60)


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
    by_kind, by_status = {}, {}
    n = n_spos = 0
    while time.time() < t_end:
        kind, p = draw_prime(rng, br)
        f, roots = draw_poly(rng, p)
        a = draw_seed(rng, p, roots)
        prec = draw_prec(rng)
        n += 1
        st, L = S.seed_root(f, p, a, prec)
        got = br.seed(f, p, a, prec)
        where = f"case {n}: f={f} p={p} a={a} prec={prec}"[:800]
        fails = []
        if STATUS.get(got["status"]) != st:
            fails.append(f"status {got['status']}, reference {st}")
        elif st == S.OK:
            ref_cert = L["certs"][0]
            ft = S.ptrim(f)
            reduced = 1 if len(ft) > 1 and len(L["g"]) < len(ft) else 0
            if got["got_cert"] != 1 or got["cert"] != ref_cert:
                fails.append(f"certificate {got['cert']}, reference {ref_cert}")
            if got["g"] != L["g"]:
                fails.append(f"g = {got['g']}, reference {L['g']}")
            if got["reduced"] != reduced:
                fails.append(f"reduced {got['reduced']}, expected {reduced}")
            if got["shape"] != (1, 1, 0, 0):
                fails.append(f"scope, n, nu, complete = {got['shape']}")
            if got["verify"] != 1 or got["canonical"] != 1:
                fails.append(f"verify_entries {got['verify']}, is_canonical {got['canonical']}")
            if not S.root_cert_ok(L["g"], p, *got["cert"]):
                fails.append("root_cert_ok of the reference refuses the C certificate")
            if ref_cert[2] > 0:
                n_spos += 1
        by_kind[kind] = by_kind.get(kind, 0) + 1
        by_status[st] = by_status.get(st, 0) + 1
        if fails:
            print("DISAGREEMENT", where)
            for x in fails:
                print("   ", x)
            return 1
    print(f"diff_roots_seed: {n} calls in {args.seconds:.0f} s, seed {args.seed}; primes {by_kind}; "
          f"statuses {by_status}; OK with s > 0: {n_spos}; 0 disagreements")
    return 0


if __name__ == "__main__":
    sys.exit(main())
