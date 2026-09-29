#!/usr/bin/env python3
"""tests/fuzz/diff_resid.py: differential run of adf_resid_reconstruct (slice s3-slice1) against
recon_partial of proto/solvers_checks.py.

    sh tests/test_exports.sh                     # builds build/libadelefeld.so
    python3 tests/fuzz/diff_resid.py --seconds 180 --seed 1

Random (m, c, A, B) of three sizes (small; near the 64 bit boundary, where 2 A B crosses 2^64 and 2^63;
thousands of bits), half of them built from a planted fraction n/d with |n| <= A, d <= B, gcd(d, m) = 1,
and with m drawn near 2 A B (2 A B - 2 .. 2 A B + 2, so the boundary of the range is hit), also with
A < 0 and B < 1. The same input goes to the C function (ctypes on build/libadelefeld.so) and, where the
contract fixes an answer, to the reference. The run asserts
  - UNSUPPORTED exactly when A >= 0, B >= 1 and 2 A B >= m (the C function then writes nothing);
  - otherwise the status equals the reference status (OK, NO_SOLUTION), q equals the reference solution
    on OK and is untouched (sentinel) otherwise, and the certificate is the reference certificate
    (kind 1) or none (kind 0, empty box);
  - the certificate satisfies (C1) to (C4) (cert_pair_ok of the reference);
  - a planted fraction is returned whenever 2 A B < m (Proposition 1.6 (c): the solution is unique).
It prints the number of cases of each status. Exit status 1 on the first disagreement."""
import argparse
import ctypes
import os
import random
import sys
import time
from math import gcd

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "proto"))
import solvers_checks as S  # noqa: E402

ADF_OK, ADF_NO_SOLUTION, ADF_UNSUPPORTED = 0, 5, 8


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

    def call(self, m, c, A, B, with_cert=True):
        for fz, v in ((self.fc, c), (self.fm, m), (self.fA, A), (self.fB, B)):
            self.set(fz, v)
        st = self.lib.adf_resid_set_fmpz2(self.x, ctypes.byref(self.fc), ctypes.byref(self.fm))
        assert st == 0, "set_fmpz2 refused m >= 1"
        self.sentinel()
        st = self.lib.adf_resid_reconstruct(self.q, self.cert if with_cert else None, self.x,
                                            ctypes.byref(self.fA), ctypes.byref(self.fB), 0)
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
    counts = {"OK": 0, "NO_SOLUTION (b)": 0, "NO_SOLUTION (c)": 0, "NO_SOLUTION (empty box)": 0,
              "UNSUPPORTED": 0}
    planted_seen = 0
    n = 0
    while time.time() < t_end:
        m, c, A, B, planted = draw(rng)
        n += 1
        st, q, cert, kind = br.call(m, c, A, B, with_cert=(n % 7 != 0))
        empty = A < 0 or B < 1
        in_range = empty or 2 * A * B < m
        where = f"case {n}: m={m} c={c} A={A} B={B}"
        if not in_range:
            if st != ADF_UNSUPPORTED or q != (-777, 13) or (n % 7 != 0 and (kind != 1 or cert != (101, -102, 103, 104))):
                print(f"FAIL {where[:400]}: want UNSUPPORTED untouched, got status {st} q {q}")
                return 1
            counts["UNSUPPORTED"] += 1
            continue
        rst, sols, rcert = S.recon_partial(m, c, A, B, 0)
        assert rst in (S.OK, S.NO_SOLUTION)
        want_st = ADF_OK if rst == S.OK else ADF_NO_SOLUTION
        if st != want_st:
            print(f"FAIL {where[:400]}: status {st}, reference {rst}")
            return 1
        if st == ADF_OK:
            if q != sols[0]:
                print(f"FAIL {where[:400]}: q {q}, reference {sols[0]}")
                return 1
            counts["OK"] += 1
            if planted is not None:
                planted_seen += 1
                if q != planted:
                    print(f"FAIL {where[:400]}: planted {planted}, got {q}")
                    return 1
        else:
            if q != (-777, 13):
                print(f"FAIL {where[:400]}: q written on NO_SOLUTION: {q}")
                return 1
            if empty:
                counts["NO_SOLUTION (empty box)"] += 1
            elif abs(rcert[3]) > B:
                counts["NO_SOLUTION (b)"] += 1
            else:
                counts["NO_SOLUTION (c)"] += 1
            if planted is not None:
                print(f"FAIL {where[:400]}: planted {planted} not found in the range 2AB < m")
                return 1
        if n % 7 != 0:
            if empty:
                ok = kind == 0 and cert == (0, 0, 0, 0)
            else:
                ok = kind == 1 and cert == tuple(rcert) and S.cert_pair_ok(m, c % m, A, cert)
            if not ok:
                print(f"FAIL {where[:400]}: certificate {cert} kind {kind}, reference {rcert}")
                return 1
        else:
            if kind != 1 or cert != (101, -102, 103, 104):
                print(f"FAIL {where[:400]}: cert = NULL but the certificate object changed")
                return 1
    print(f"diff_resid: seed {args.seed}, {n} cases in {args.seconds} s, no disagreement; "
          + "; ".join(f"{k}: {v}" for k, v in counts.items()) + f"; planted fractions returned: {planted_seen}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
