#!/usr/bin/env python3
"""Sustained exact differential check of all three exported queries on dyadic lifts.

Run under timeout, with a library path and duration. No sampled point is used as a set oracle.
"""
import ctypes as C
from fractions import Fraction as F
from math import ceil, floor, lcm
from pathlib import Path
import random
import sys
import time

sys.dont_write_bytecode = True
sys.path.insert(0, str(Path('proto').resolve()))
import quotient3_checks as q


class Qclass(C.Structure):
    _fields_ = [('form', C.c_int), ('length', C.c_long), ('piece', C.c_void_p)]


def decimal(v):
    assert v.denominator & (v.denominator-1) == 0
    k = v.denominator.bit_length()-1
    n = abs(v.numerator)*5**k
    s = str(n).rjust(k+1, '0')
    return ('-' if v < 0 else '')+(s[:-k]+'.'+s[-k:] if k else s)


def main():
    lib = C.CDLL(sys.argv[1])
    seconds = float(sys.argv[2]) if len(sys.argv) > 2 else 120
    assert lib.adf_sizeof_qclass() == C.sizeof(Qclass)
    assert lib.adf_alignof_qclass() == C.alignment(Qclass)
    for name in ('init', 'clear'):
        fn = getattr(lib, 'adf_qclass_'+name)
        fn.argtypes = [C.POINTER(Qclass)]; fn.restype = None
    read = lib.adf_adele_set_str
    read.argtypes = [C.c_void_p, C.c_char_p, C.c_size_t, C.c_long, C.c_void_p]
    queries = [getattr(lib, 'adf_qclass_'+s) for s in ('equal_set', 'contains', 'overlaps')]
    for fn in queries:
        fn.argtypes = [C.POINTER(C.c_int), C.POINTER(Qclass), C.POINTER(Qclass), C.c_long]
    x, y = Qclass(), Qclass()
    lib.adf_qclass_init(C.byref(x)); lib.adf_qclass_init(C.byref(y))
    rng = random.Random(310607)
    start = time.monotonic(); cases = calls = brute = witnesses = 0
    try:
        while time.monotonic()-start < seconds:
            inputs, norms, raw = [], [], 0
            for dst in (x, y):
                mid = F(rng.randrange(-32, 33), 16)
                rad = F(rng.randrange(17), 16)
                N = rng.choice((F(0), F(1), F(2), F(3), F(4), F(6), F(12), F(1, 2), F(2, 3)))
                a = F(rng.randrange(-20, 21), rng.choice((1, 2, 3, 4)))
                if cases % 97 == 0:
                    huge = F(2**2000+cases)
                    mid += huge; a += huge
                if N:
                    a %= N
                text = f'({decimal(mid)} +/- {decimal(rad)} ; {a}'+(f' mod {N}' if N else '')+')'
                data = text.encode()
                assert read(dst.piece, data, len(data), 4096, None) == 0
                lo, hi = mid-rad, mid+rad
                B = N.denominator
                for j in range(B):
                    l, h = lo-a-j*N, hi-a-j*N
                    raw += 1 if l == h else ceil(h)-floor(l)
                norms.append(q.reduce(mid, rad, q.Ball(a, N)))
                inputs.append(text)
            nx, ny = norms
            L = lcm(*(p.N for p in nx+ny if p.N))
            E = len({t for p in nx+ny for t in (p.lo, p.hi)})
            budget = (2*E+1)*raw*L
            expected = [q.compare(nx, ny), q.compare(ny, nx)]
            for side, (a, b) in enumerate(((x, y), (y, x))):
                for k, fn in enumerate(queries):
                    truth = C.c_int(719)
                    st = fn(C.byref(truth), C.byref(a), C.byref(b), budget); calls += 1
                    assert (st, truth.value) == (0, expected[side][k]), (cases, inputs, side, k, st, truth.value)
                    truth.value = 719
                    st = fn(C.byref(truth), C.byref(a), C.byref(b), budget-1); calls += 1
                    assert (st, truth.value) == (10, 719), (cases, inputs, side, k, st, truth.value)
            if cases % 100 == 0:
                pointmax = max((abs(p.m) for p in nx+ny if not p.N), default=0)
                if pointmax < 100:
                    b = q.brute_sets(nx, ny, 2*L+2+pointmax)
                    assert b[:3] == expected[0]; brute += 1; witnesses += b[3]
            cases += 1
    finally:
        lib.adf_qclass_clear(C.byref(x)); lib.adf_qclass_clear(C.byref(y)); lib.flint_cleanup()
    print(f'seed 310607: {cases} pairs, {calls} calls, {brute} brute grids, '
          f'{witnesses} membership pairs, 0 disagreements, {time.monotonic()-start:.3f} s')


if __name__ == '__main__':
    main()
