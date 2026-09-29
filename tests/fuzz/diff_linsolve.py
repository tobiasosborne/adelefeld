#!/usr/bin/env python3
"""tests/fuzz/diff_linsolve.py: differential run of adf_linsolve_mod (slice 1 of S.1, lane s1-slice1) against
linsolve_mod of proto/solvers_checks.py.

    sh tests/test_exports.sh                     # builds build/libadelefeld.so
    python3 tests/fuzz/diff_linsolve.py --seconds 180 --seed 1

Random systems A x = b modulo N with r, c <= 6 (the sizes 0 included); N of five kinds: small (1 to 40), highly
composite (products of small prime powers), prime (word-size and Mersenne primes up to 2^127 - 1), 64 bits,
300 bits (random, and powers 2^300, 3^189); A with zero divisors, zero rows and repeated rows now and then; b
planted from a random x half of the time; the entries of A and b lifted by random multiples of N of up to 400
bits, of both signs. The same input goes to the C function (ctypes on build/libadelefeld.so, one adf_linsol
reused for every call) and to the reference. The run asserts, for every system:
  - the status is the reference status (OK 0 or NO_SOLUTION 5);
  - G, E, V and x0 (for OK) or y (for NO_SOLUTION) equal those of the reference entry by entry (Algorithm L of
    docs/proofs/solvers.md 2.8 determines them; the Howell form is unique, P2.4);
  - the accessors agree with the fields (kind, kernel_rows, get_kernel, get_particular, get_dual);
  - the C checker adf_linsol_verify accepts the result, and the reference checker linsol_check accepts the
    reference result.
It prints the number of systems of each kind of modulus and of each status. Exit status 1 on the first
disagreement."""
import argparse
import ctypes
import os
import random
import sys
import time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "proto"))
import solvers_checks as S  # noqa: E402

ADF_OK, ADF_NO_SOLUTION = 0, 5
SIZEOF_FMPZ_MAT = 32                        # FLINT 3.0.1 fmpz_types.h: entries, r, c, rows
OFF_G, OFF_E, OFF_V, OFF_X0, OFF_Y = 32, 64, 96, 128, 160   # include/adelefeld/linsolve.h, the layout


class Bridge:
    def __init__(self, path):
        self.flint = ctypes.CDLL("libflint.so.18", mode=ctypes.RTLD_GLOBAL)
        self.lib = ctypes.CDLL(path)
        f, lib = self.flint, self.lib
        f.fmpz_set_str.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
        f.fmpz_get_str.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_void_p]
        f.fmpz_get_str.restype = ctypes.c_void_p
        f.flint_free.argtypes = [ctypes.c_void_p]
        f.fmpz_init.argtypes = [ctypes.c_void_p]
        f.fmpz_mat_init.argtypes = [ctypes.c_void_p, ctypes.c_long, ctypes.c_long]
        f.fmpz_mat_clear.argtypes = [ctypes.c_void_p]
        f.fmpz_mat_entry.argtypes = [ctypes.c_void_p, ctypes.c_long, ctypes.c_long]
        f.fmpz_mat_entry.restype = ctypes.c_void_p
        for n in ("adf_linsol_init", "adf_linsol_clear"):
            getattr(lib, n).argtypes = [ctypes.c_void_p]
        lib.adf_linsolve_mod.argtypes = [ctypes.c_void_p] * 4
        lib.adf_linsolve_mod.restype = ctypes.c_int
        lib.adf_linsol_verify.argtypes = [ctypes.c_void_p] * 4
        lib.adf_linsol_verify.restype = ctypes.c_int
        lib.adf_linsol_kind.argtypes = [ctypes.c_void_p]
        lib.adf_linsol_kind.restype = ctypes.c_int
        lib.adf_linsol_kernel_rows.argtypes = [ctypes.c_void_p]
        lib.adf_linsol_kernel_rows.restype = ctypes.c_long
        for n in ("adf_linsol_get_kernel", "adf_linsol_get_particular", "adf_linsol_get_dual"):
            getattr(lib, n).argtypes = [ctypes.c_void_p, ctypes.c_void_p]
        lib.adf_linsol_get_particular.restype = ctypes.c_int
        lib.adf_linsol_get_dual.restype = ctypes.c_int
        lib.adf_sizeof_linsol.restype = ctypes.c_size_t
        assert lib.adf_sizeof_linsol() == 192
        self.sol = ctypes.create_string_buffer(192)
        lib.adf_linsol_init(self.sol)
        self.N = ctypes.c_long(0)
        f.fmpz_init(ctypes.byref(self.N))
        self.out = ctypes.create_string_buffer(SIZEOF_FMPZ_MAT)
        f.fmpz_mat_init(self.out, 0, 0)

    def set_fmpz(self, addr, n):
        assert self.flint.fmpz_set_str(addr, str(n).encode(), 10) == 0

    def get_fmpz(self, addr):
        p = self.flint.fmpz_get_str(None, 10, addr)
        s = ctypes.string_at(p).decode()
        self.flint.flint_free(p)
        return int(s)

    def mat_new(self, rows, r, c):
        m = ctypes.create_string_buffer(SIZEOF_FMPZ_MAT)
        self.flint.fmpz_mat_init(m, r, c)
        for i in range(r):
            for j in range(c):
                self.set_fmpz(self.flint.fmpz_mat_entry(m, i, j), rows[i][j])
        return m

    def mat_get(self, addr):
        r = ctypes.c_long.from_address(addr + 8).value
        c = ctypes.c_long.from_address(addr + 16).value
        return [[self.get_fmpz(self.flint.fmpz_mat_entry(addr, i, j)) for j in range(c)] for i in range(r)], r, c

    def solve(self, A, b, r, c, N):
        f, lib = self.flint, self.lib
        mA = self.mat_new(A, r, c)
        mb = self.mat_new([[x] for x in b], r, 1)
        self.set_fmpz(ctypes.byref(self.N), N)
        st = lib.adf_linsolve_mod(self.sol, mA, mb, ctypes.byref(self.N))
        base = ctypes.addressof(self.sol)
        res = {"status": st, "kind": lib.adf_linsol_kind(self.sol), "k": lib.adf_linsol_kernel_rows(self.sol)}
        for name, off in (("G", OFF_G), ("E", OFF_E), ("V", OFF_V), ("x0", OFF_X0), ("y", OFF_Y)):
            res[name] = self.mat_get(base + off)
        lib.adf_linsol_get_kernel(self.out, self.sol)
        res["get_kernel"] = self.mat_get(ctypes.addressof(self.out))
        res["has_x0"] = lib.adf_linsol_get_particular(self.out, self.sol)
        res["get_x0"] = self.mat_get(ctypes.addressof(self.out))
        res["has_y"] = lib.adf_linsol_get_dual(self.out, self.sol)
        res["get_y"] = self.mat_get(ctypes.addressof(self.out))
        res["verify"] = lib.adf_linsol_verify(self.sol, mA, mb, ctypes.byref(self.N))
        f.fmpz_mat_clear(mA)
        f.fmpz_mat_clear(mb)
        return res


PRIMES = [2, 3, 5, 7, 31, 101, 65537, 10 ** 9 + 7, 998244353, 2 ** 61 - 1, 2 ** 89 - 1, 2 ** 107 - 1,
          2 ** 127 - 1, 18446744073709551557]   # the last is the largest prime below 2^64


def draw_modulus(rng):
    kind = rng.choice(("small", "composite", "prime", "w64", "b300"))
    if kind == "small":
        N = rng.randint(1, 40)
    elif kind == "composite":
        N = 1
        for p in (2, 3, 5, 7, 11, 13):
            N *= p ** rng.randrange(0, 5 if p < 5 else 2)
        N = max(N, 2)
    elif kind == "prime":
        N = rng.choice(PRIMES)
    elif kind == "w64":
        N = rng.getrandbits(64) | (1 << 63) if rng.random() < 0.7 else rng.choice((2 ** 64, 2 ** 63, 2 ** 64 - 1))
    else:
        N = rng.choice((rng.getrandbits(300) | (1 << 299), 2 ** 300, 3 ** 189, 2 ** 150 * 3 ** 90))
    return kind, N


def draw_system(rng, N):
    r, c = rng.randint(0, 6), rng.randint(0, 6)
    A = [[rng.randrange(N) for _ in range(c)] for _ in range(r)]
    u = rng.random()
    if u < 0.3:                                     # zero divisors
        d = rng.choice([2, 3, 4, 6, 8, 9, 12, 2 ** 20, 3 ** 10])
        A = [[(x * d) % N for x in row] for row in A]
    elif u < 0.4 and r:                             # a zero row
        A[rng.randrange(r)] = [0] * c
    elif u < 0.5 and r >= 2:                        # a repeated row, scaled
        i, k = rng.randrange(r), rng.randrange(r)
        s = rng.randrange(N)
        A[i] = [(s * x) % N for x in A[k]]
    if rng.random() < 0.5 and c:
        x = [rng.randrange(N) for _ in range(c)]
        b = S.matvec(A, x, N) if r else []
    else:
        b = [rng.randrange(N) for _ in range(r)]
    if rng.random() < 0.6:                          # entries of any sign and size
        bits = rng.choice((8, 64, 400))
        A = [[x + N * (rng.getrandbits(bits) - (1 << (bits - 1))) for x in row] for row in A]
        b = [x + N * (rng.getrandbits(bits) - (1 << (bits - 1))) for x in b]
    return A, b, r, c


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
    by_kind = {}
    by_status = {"OK": 0, "NO_SOLUTION": 0}
    n = 0
    while time.time() < t_end:
        kind, N = draw_modulus(rng)
        A, b, r, c = draw_system(rng, N)
        n += 1
        ref = S.linsolve_mod(A, b, r, c, N)
        got = br.solve(A, b, r, c, N)
        where = f"case {n}: N={N} r={r} c={c} A={A} b={b}"[:600]
        want = ADF_OK if ref["status"] == S.OK else ADF_NO_SOLUTION
        fails = []
        if not S.linsol_check(ref, A, b, r, c, N):
            fails.append("the reference checker refuses the reference result")
        if got["status"] != want:
            fails.append(f"status {got['status']}, reference {ref['status']}")
        else:
            for name, cols in (("G", c), ("E", r), ("V", c)):
                rows, gr, gc = got[name]
                if rows != ref[name] or gr != len(ref[name]) or gc != cols:
                    fails.append(f"{name} = {rows} ({gr} by {gc}), reference {ref[name]}")
            if want == ADF_OK:
                x0 = [row[0] for row in got["x0"][0]]
                if x0 != ref["x0"] or got["x0"][1:] != (c, 1) or got["y"][1:] != (0, 0):
                    fails.append(f"x0 = {got['x0']}, reference {ref['x0']}")
                if got["kind"] != 0 or got["has_x0"] != 1 or got["has_y"] != 0 or got["get_x0"] != got["x0"]:
                    fails.append("kind or accessors of a COSET result")
            else:
                y = [row[0] for row in got["y"][0]]
                if y != ref["y"] or got["y"][1:] != (r, 1) or got["x0"][1:] != (0, 0):
                    fails.append(f"y = {got['y']}, reference {ref['y']}")
                if got["kind"] != 1 or got["has_x0"] != 0 or got["has_y"] != 1 or got["get_y"] != got["y"]:
                    fails.append("kind or accessors of an EMPTY result")
            if got["k"] != len(ref["G"]) or got["get_kernel"] != got["G"]:
                fails.append("kernel_rows or get_kernel")
            if got["verify"] != 1:
                fails.append("adf_linsol_verify refuses the result")
        if fails:
            print(f"FAIL {where}: " + "; ".join(fails))
            return 1
        by_kind[kind] = by_kind.get(kind, 0) + 1
        by_status[ref["status"]] += 1
    print(f"diff_linsolve: {n} systems in {args.seconds:.0f} s (seed {args.seed}), 0 disagreements; "
          f"moduli {dict(sorted(by_kind.items()))}; statuses {by_status}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
