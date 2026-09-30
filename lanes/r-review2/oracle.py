import sys; sys.set_int_max_str_digits(0)
"""Own oracle: exact Sturm chain over Q (python-flint fmpq_poly for the rational arithmetic only; the chain and
sign counting are written here), and a checker for the lists printed by drv.c."""
import subprocess, sys
from fractions import Fraction
from flint import fmpq_poly, fmpz_poly, fmpq

import os
DRV = os.environ.get("ADF_DRV", "/tmp/x/drv")


def sqfree(coeffs):
    f = fmpz_poly(coeffs)
    if f.degree() < 1:
        return fmpq_poly([1]) if f.degree() == 0 else fmpq_poly([0])
    q = fmpq_poly(coeffs)
    g = q.gcd(q.derivative())
    s = q // g
    return s


def sturm(p):
    chain = [p, p.derivative()]
    while chain[-1].degree() > 0:
        r = chain[-2] % chain[-1]
        if r == 0:
            break
        chain.append(-r)
    if chain[-1].degree() == 0 and len(chain) > 2 or (chain[-1].degree() == 0 and chain[-1] != 0):
        pass
    return chain


def sgn(x):
    return (x > 0) - (x < 0)


def var_at(chain, x):
    """sign variations of the chain at the rational x (fmpq) or +-inf ('+','-')"""
    sg = []
    for q in chain:
        if q == 0:
            continue
        if x == '+':
            s = sgn(q[q.degree()])
        elif x == '-':
            s = sgn(q[q.degree()]) * (-1) ** q.degree()
        else:
            s = sgn(q(x))
        if s:
            sg.append(s)
    return sum(1 for i in range(len(sg) - 1) if sg[i] != sg[i + 1])


def nroots(coeffs):
    s = sqfree(coeffs)
    if s.degree() < 1:
        return 0, s, None
    ch = sturm(s)
    return var_at(ch, '-') - var_at(ch, '+'), s, ch


def roots_in(ch, lo, hi):
    """number of distinct roots in (lo, hi] of the squarefree chain[0]"""
    return var_at(ch, lo) - var_at(ch, hi)


def run(lines):
    inp = "".join(lines)
    out = subprocess.run([DRV], input=inp, capture_output=True, text=True, timeout=170)
    return out.stdout.splitlines(), out.returncode, out.stderr


def check_one(prec, coeffs, line, strict_acc=True):
    """returns list of defect strings"""
    bad = []
    t = line.split()
    st = int(t[0])
    n, s, ch = nroots(coeffs)
    if st != 0:
        return [f"status {st} (true count {n})"]
    cnt, m, ve, vc = int(t[1]), int(t[2]), int(t[3]), int(t[4])
    if cnt != n:
        bad.append(f"count {cnt} != sturm {n}")
    if m != n:
        bad.append(f"n {m} != sturm {n}")
    if not (ve and vc):
        bad.append(f"verifiers {ve} {vc}")
    vals = t[6:]
    prev_hi = None
    for i in range(m):
        a, b, e = int(vals[3 * i]), int(vals[3 * i + 1]), int(vals[3 * i + 2])
        lo = fmpq(a) * fmpq(2) ** e if e >= 0 else fmpq(a, 2 ** (-e))
        hi = fmpq(b) * fmpq(2) ** e if e >= 0 else fmpq(b, 2 ** (-e))
        if prev_hi is not None and not prev_hi < lo:
            bad.append("unordered")
        prev_hi = hi
        if ch is not None:
            if lo == hi:
                c = 1 if s(lo) == 0 else 0
            else:
                c = roots_in(ch, lo, hi) + (1 if s(lo) == 0 else 0)
            if c != 1:
                bad.append(f"ball {i} holds {c} roots")
        if strict_acc and lo != hi:
            mid = (lo + hi) / 2
            rad = (hi - lo) / 2
            if mid == 0 or rad > abs(mid) * fmpq(1, 2 ** (prec - int(os.environ.get("SLACK", "1")))):
                bad.append(f"ball {i} accuracy: rad/|mid|={float(rad/abs(mid)) if mid != 0 else 'inf'} prec {prec}")
    return bad
