"""m4-review1 common model code: encoding for gen/h, parsing of its exact output, exact finite functions,
the finite transform by exact cyclotomic bookkeeping (integer coefficient vectors over E(h/L)) followed by one
fixed-point evaluation with a stated error bound. No adelefeld or proto code is imported."""
import subprocess, os, random
from fractions import Fraction as Fr
from math import gcd
import mpmath

HERE = os.path.dirname(os.path.abspath(__file__))
S = 400  # fixed-point scale 2^S for root-of-unity values
_roots = {}


def lcm(a, b):
    return a // gcd(a, b) * b


def roots(L):
    """[(c_h, s_h)] with E(h/L) = (c_h + i s_h) / 2^S, |error| <= 1 per component (mpmath at S+60 bits)."""
    if L not in _roots:
        mpmath.mp.prec = S + 60
        out = []
        for h in range(L):
            t = 2 * mpmath.pi * h / L
            out.append((int(mpmath.nint(mpmath.cos(t) * 2 ** S)), int(mpmath.nint(mpmath.sin(t) * 2 ** S))))
        _roots[L] = out
    return _roots[L]


# ---------------------------------------------------------------- encoding for gen/h
def enc_real(q, rm=0, re=0):
    q = Fr(q)
    return "%d %d %d %d" % (q.numerator, q.denominator, rm, re)


def enc_c(z, rad=(0, 0, 0, 0)):
    """z = (re, im) Fractions; rad = (rm_re, re_re, rm_im, re_im)."""
    return enc_real(z[0], rad[0], rad[1]) + " " + enc_real(z[1], rad[2], rad[3])


def enc_ffun(D, M, vals, rads=None):
    parts = ["%d %d" % (D, M)]
    for j, v in enumerate(vals):
        parts.append(enc_c(v, rads[j] if rads else (0, 0, 0, 0)))
    return " ".join(parts)


def enc_rfun(terms):
    """terms: list of (P list of complex-ball, A, B, C) each complex-ball = ((re, im), rad4)."""
    parts = [str(len(terms))]
    for P, A, B, C in terms:
        parts.append(str(len(P)))
        for c in P:
            parts.append(enc_c(*c))
        for c in (A, B, C):
            parts.append(enc_c(*c))
    return " ".join(parts)


# ---------------------------------------------------------------- parsing
def dy(m, e):
    m, e = int(m), int(e)
    return Fr(m) * Fr(2) ** e


class Tok:
    def __init__(self, line):
        self.t = line.split()
        self.i = 0

    def nxt(self):
        v = self.t[self.i]
        self.i += 1
        return v

    def int(self):
        return int(self.nxt())

    def arb(self):
        a, b, c, d = self.nxt(), self.nxt(), self.nxt(), self.nxt()
        if a == 'nan' or c == 'nan':
            return (None, None)
        return (dy(a, b), dy(c, d))

    def acb(self):
        return (self.arb(), self.arb())

    def ffun(self):
        D, M = self.int(), self.int()
        return (D, M, [self.acb() for _ in range(D * M)])

    def rfun(self):
        n = self.int()
        out = []
        for _ in range(n):
            l = self.int()
            P = [self.acb() for _ in range(l)]
            A, B, C = self.acb(), self.acb(), self.acb()
            out.append((P, A, B, C))
        return out


def run(lines, prog="h", timeout=160, env=None):
    p = subprocess.run([os.path.join(HERE, prog)], input="\n".join(lines) + "\n", capture_output=True, text=True,
                       timeout=timeout, env=env)
    if p.returncode != 0:
        raise RuntimeError("rc %d: %s %s" % (p.returncode, p.stdout[-2000:], p.stderr[-4000:]))
    return p.stdout.splitlines()


# ---------------------------------------------------------------- containment
def arb_has(ball, x, err=0):
    """1 if x (Fraction) with numeric error <= err is certainly inside, 0 if certainly outside, 2 if undecided."""
    m, r = ball
    if m is None:
        return 0
    d = abs(x - m)
    if d + err <= r:
        return 1
    if d - err > r:
        return 0
    return 2


def acb_has(ball, z, err=0):
    a = arb_has(ball[0], z[0], err)
    b = arb_has(ball[1], z[1], err)
    if a == 0 or b == 0:
        return 0
    return 1 if (a == 1 and b == 1) else 2


# ---------------------------------------------------------------- exact finite functions
def f_point(D, M, vals, x):
    """Value at the rational x of the function zero off (1/D) Zhat with value vals[j] on j/D + M Zhat."""
    x = Fr(x)
    y = D * x
    if y.denominator != 1:
        return (Fr(0), Fr(0))
    return vals[int(y) % (D * M)]


def ftransform_exact(D, M, vals):
    """g_k = (1/M) sum_j f_j E(-jk/L) for Gaussian-rational f_j (own derivation, conventions 6.1/6.3).
    Returns list of (re, im, err) as Fractions: re/im within err of the exact value."""
    L = D * M
    den = 1
    for v in vals:
        den = lcm(den, lcm(v[0].denominator, v[1].denominator))
    a = [(int(v[0] * den), int(v[1] * den)) for v in vals]
    R = roots(L)
    out = []
    for k in range(L):
        cre = [0] * L
        cim = [0] * L
        for j in range(L):
            if a[j][0] or a[j][1]:
                h = (-j * k) % L
                cre[h] += a[j][0]
                cim[h] += a[j][1]
        re = im = 0
        tot = 0
        for h in range(L):
            if cre[h] or cim[h]:
                c, s = R[h]
                re += cre[h] * c - cim[h] * s
                im += cre[h] * s + cim[h] * c
                tot += abs(cre[h]) + abs(cim[h])
        sc = Fr(1, den * M * 2 ** S)
        out.append((re * sc, im * sc, Fr(tot + 1, den * M * 2 ** S)))
    return out
