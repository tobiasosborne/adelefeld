#!/usr/bin/env python3
"""tests/fuzz/diff_resid.py: differential run of adf_resid_reconstruct (slices 1 and 2 of milestone S) against
recon_partial of proto/solvers_checks.py.

    sh tests/test_exports.sh                     # builds build/libadelefeld.so
    python3 tests/fuzz/diff_resid.py --seconds 180 --seed 1

Random (m, c, A, B, limit): three sizes of operands (small; near the 64 bit boundary, where 2 A B crosses 2^64 and
2^63; thousands of bits), half of them built from a planted fraction n/d with |n| <= A, d <= B, gcd(d, m) = 1, and
with m drawn near 2 A B (2 A B - 2 .. 2 A B + 2, so the boundary of the range is hit), also A < 0, B < 1 and
A >= m. The limit is drawn from -3, -1, 0, 1, 2, 3, 5, 10, 50, 300, 1000 so that one case runs at most about 1000
rounds. The same input goes to the C function (ctypes on build/libadelefeld.so) and to the reference. The run
asserts
  - the status equals the reference status (OK, NO_SOLUTION, NOT_UNIQUE, NOT_DETERMINED);
  - q equals the reference solution on OK, and is untouched (sentinel) on every other status;
  - the certificate is the reference certificate (kind 1) when 0 <= A < m and B >= 1, else none (kind 0), and it
    satisfies (C1) to (C4) (cert_pair_ok of the reference); with cert = NULL (every 7th case) the certificate
    object is not changed;
  - a planted fraction is a solution of the problem, so the status is never NO_SOLUTION, and on OK q is the
    planted fraction.
It prints the number of cases of each status, of them with a cut search (floor(B/|T|) > limit) and with
NOT_UNIQUE found inside a cut search. Exit status 1 on the first disagreement. A run of 180 s is a smoke test of the
contract, not a proof."""
import argparse
import ctypes
import os
import random
import sys
import time
from math import gcd

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "proto"))
import solvers_checks as S  # noqa: E402

ADF_OK, ADF_NOT_DETERMINED, ADF_NOT_UNIQUE, ADF_NO_SOLUTION = 0, 1, 4, 5
STATUS = {S.OK: ADF_OK, S.NOT_DETERMINED: ADF_NOT_DETERMINED, S.NOT_UNIQUE: ADF_NOT_UNIQUE, S.NO_SOLUTION: ADF_NO_SOLUTION}
LIMITS = (-3, -1, 0, 1, 2, 3, 5, 10, 50, 300, 1000)


def load(path):
    flint = ctypes.CDLL("libflint.so.18", mode=ctypes.RTLD_GLOBAL)
    lib = ctypes.CDLL(path)
    return flint, lib


class Bridge:
    def __init__(self, path):
        self.flint, self.lib = load(path)
        f = self.flint
        f.fmpz_set_str.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
        f.fmpz_get_str.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_void_p]
        f.fmpz_get_str.restype = ctypes.c_void_p
        f.flint_free.argtypes = [ctypes.c_void_p]
        f.fmpz_init.argtypes = [ctypes.c_void_p]
        f.fmpz_clear.argtypes = [ctypes.c_void_p]
        lib = self.lib
        lib.adf_resid_reconstruct.argtypes = [ctypes.c_void_p] * 5 + [ctypes.c_long]
        lib.adf_resid_reconstruct.restype = ctypes.c_int
        lib.adf_resid_set_fmpz2.argtypes = [ctypes.c_void_p] * 3
        lib.adf_resid_set_fmpz2.restype = ctypes.c_int
        for n in ("adf_resid_init", "adf_resid_clear", "adf_rat_init", "adf_rat_clear",
                  "adf_recon_cert_init", "adf_recon_cert_clear"):
            getattr(lib, n).argtypes = [ctypes.c_void_p]
        # storage: adf_resid 16 bytes, adf_rat 16, adf_recon_cert 40 (docs/api-s.md section 1)
        assert lib.adf_sizeof_resid() == 16 and lib.adf_sizeof_recon_cert() == 40 and lib.adf_sizeof_rat() == 16
        self.x = ctypes.create_string_buffer(16)
        self.q = ctypes.create_string_buffer(16)
        self.cert = ctypes.create_string_buffer(40)
        lib.adf_resid_init(self.x)
        lib.adf_rat_init(self.q)
        lib.adf_recon_cert_init(self.cert)
        self.fc, self.fm, self.fA, self.fB = (ctypes.c_long(0) for _ in range(4))
        for v in (self.fc, self.fm, self.fA, self.fB):
            f.fmpz_init(ctypes.byref(v))

    def set(self, fz, n):
        self.flint.fmpz_set_str(ctypes.byref(fz), str(n).encode(), 10)

    def get(self, addr):
        p = self.flint.fmpz_get_str(None, 10, addr)
        s = ctypes.string_at(p).decode()
        self.flint.flint_free(p)
        return int(s)

    def field(self, buf, off):
        return self.get(ctypes.addressof(buf) + off)

    def sentinel(self):
        self.flint.fmpz_set_str(ctypes.c_void_p(ctypes.addressof(self.q)), b"-777", 10)
        self.flint.fmpz_set_str(ctypes.c_void_p(ctypes.addressof(self.q) + 8), b"13", 10)
        for i, v in enumerate((101, -102, 103, 104)):
            self.flint.fmpz_set_str(ctypes.c_void_p(ctypes.addressof(self.cert) + 8 * i), str(v).encode(), 10)
        ctypes.c_int.from_address(ctypes.addressof(self.cert) + 32).value = 1

    def call(self, m, c, A, B, limit, with_cert=True):
        for fz, v in ((self.fc, c), (self.fm, m), (self.fA, A), (self.fB, B)):
            self.set(fz, v)
        st = self.lib.adf_resid_set_fmpz2(self.x, ctypes.byref(self.fc), ctypes.byref(self.fm))
        assert st == 0, "set_fmpz2 refused m >= 1"
        self.sentinel()
        st = self.lib.adf_resid_reconstruct(self.q, self.cert if with_cert else None, self.x,
                                            ctypes.byref(self.fA), ctypes.byref(self.fB), limit)
        q = (self.field(self.q, 0), self.field(self.q, 8))
        cert = tuple(self.field(self.cert, 8 * i) for i in range(4))
        kind = ctypes.c_int.from_address(ctypes.addressof(self.cert) + 32).value
        return st, q, cert, kind


def draw(rng):
    """One (m, c, A, B, planted) with planted the fraction (n, d) or None."""
    size = rng.choice(("small", "small", "w64", "w64", "big"))
    if size == "small":
        bits = rng.randrange(1, 9)
    elif size == "w64":
        bits = rng.choice((30, 31, 32, 33, 62, 63))
    else:
        bits = rng.randrange(200, 3000)
    A = rng.getrandbits(bits) + rng.choice((0, 0, 1))
    B = rng.getrandbits(bits) + 1
    r = rng.random()
    if r < 0.03:
        A = -rng.randrange(1, 5)
    elif r < 0.06:
        B = -rng.randrange(0, 5)
    twoAB = 2 * A * B
    mode = rng.randrange(4)
    if mode == 0:
        m = max(1, twoAB + rng.randrange(-2, 3))
    elif mode == 1:
        m = max(1, twoAB + 1 + rng.getrandbits(bits))
    elif mode == 2:
        m = max(1, twoAB + 1 + rng.getrandbits(3))
    else:
        m = rng.getrandbits(max(bits, 2)) + 1
    planted = None
    if rng.random() < 0.5 and A >= 0 and B >= 1:
        for _ in range(20):
            d = rng.randrange(1, B + 1)
            n = rng.randrange(-A, A + 1)
            g = gcd(n, d)
            n, d = n // g, d // g
            if gcd(d, m) == 1:
                c = n * pow(d, -1, m) % m + m * rng.randrange(-2, 3)
                planted = (n, d)
                return m, c, A, B, planted
    c = rng.getrandbits(max(m.bit_length(), 1) + 2) - (1 << max(m.bit_length(), 1)) if rng.random() < 0.3 \
        else rng.randrange(m)
    return m, c, A, B, None


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
    counts = {"OK": 0, "NO_SOLUTION": 0, "NOT_UNIQUE": 0, "NOT_DETERMINED": 0}
    cut = cut_unique = planted_seen = a_ge_m = empty_box = 0
    n = 0
    while time.time() < t_end:
        m, c, A, B, planted = draw(rng)
        limit = rng.choice(LIMITS)
        n += 1
        with_cert = n % 7 != 0
        st, q, cert, kind = br.call(m, c, A, B, limit, with_cert=with_cert)
        rst, sols, rcert = S.recon_partial(m, c, A, B, limit)
        where = f"case {n}: m={m} c={c} A={A} B={B} limit={limit}"
        if st != STATUS[rst]:
            print(f"FAIL {where[:400]}: status {st}, reference {rst}")
            return 1
        counts[rst] += 1
        empty = A < 0 or B < 1
        empty_box += empty
        a_ge_m += (not empty) and A >= m
        if rcert is not None and abs(rcert[3]) <= B and 2 * A * B >= m and B // abs(rcert[3]) > max(limit, 0):
            cut += 1
            cut_unique += rst == S.NOT_UNIQUE
        if st == ADF_OK:
            if q != sols[0]:
                print(f"FAIL {where[:400]}: q {q}, reference {sols[0]}")
                return 1
        elif q != (-777, 13):
            print(f"FAIL {where[:400]}: q written on status {st}: {q}")
            return 1
        if planted is not None:
            planted_seen += 1
            if st == ADF_NO_SOLUTION or (st == ADF_OK and q != planted):
                print(f"FAIL {where[:400]}: planted {planted}, status {st}, q {q}")
                return 1
        if with_cert:
            if empty or A >= m:
                ok = kind == 0 and cert == (0, 0, 0, 0)
            else:
                ok = kind == 1 and cert == tuple(rcert) and S.cert_pair_ok(m, c % m, A, cert)
            if not ok:
                print(f"FAIL {where[:400]}: certificate {cert} kind {kind}, reference {rcert}")
                return 1
        elif kind != 1 or cert != (101, -102, 103, 104):
            print(f"FAIL {where[:400]}: cert = NULL but the certificate object changed")
            return 1
    print(f"diff_resid: seed {args.seed}, {n} cases in {args.seconds} s, no disagreement; "
          + "; ".join(f"{k}: {v}" for k, v in counts.items())
          + f"; cut searches {cut} (NOT_UNIQUE found inside one: {cut_unique}); A >= m: {a_ge_m}; empty box: "
          f"{empty_box}; planted fractions: {planted_seen}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
