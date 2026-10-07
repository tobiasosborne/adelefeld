"""Lane m3-review1 (Claude Opus): shared helpers. Exact rationals only; nothing imported from proto/
or from the lanes' vectors. Piece encoding of h.c: mm me rm re cn cd Nn Nd bk."""
import os
import random
import subprocess
from fractions import Fraction as F
from math import gcd, floor, ceil

HERE = os.path.dirname(os.path.abspath(__file__))


def run(lines, exe='h', timeout=160, env=None):
    inp = '\n'.join(lines) + '\n'
    e = dict(os.environ)
    if env:
        e.update(env)
    r = subprocess.run([os.path.join(HERE, exe)], input=inp, capture_output=True, text=True,
                       timeout=timeout, env=e)
    if r.returncode != 0:
        raise RuntimeError('rc=%d stderr=%s' % (r.returncode, r.stderr[-3000:]))
    return r.stdout.splitlines(), r.stderr


def dyadic(v):
    """(mant, exp) with v = mant 2^exp; v a dyadic Fraction."""
    v = F(v)
    d = v.denominator
    assert d & (d - 1) == 0, v
    return v.numerator, -(d.bit_length() - 1)


def rad_enc(r):
    """r dyadic with odd mantissa < 2^30."""
    if r == 0:
        return 0, 0
    m, e = dyadic(r)
    while m % 2 == 0:
        m //= 2
        e += 1
    assert m < 2 ** 30
    return m, e


class Piece:
    """Real ball [mid - rad, mid + rad] (dyadic), finite c + N Zhat (rationals), backend bk."""
    def __init__(self, mid, rad, c, N, bk=0):
        self.mid, self.rad, self.c, self.N, self.bk = F(mid), F(rad), F(c), F(N), bk

    @property
    def lo(self):
        return self.mid - self.rad

    @property
    def hi(self):
        return self.mid + self.rad

    def enc(self):
        mm, me = dyadic(self.mid)
        rm, re_ = rad_enc(self.rad)
        return '%d %d %d %d %d %d %d %d %d' % (mm, me, rm, re_, self.c.numerator, self.c.denominator,
                                               self.N.numerator, self.N.denominator, self.bk)


def enc_class(form, pieces):
    return '%d %d %s' % (form, len(pieces), ' '.join(p.enc() for p in pieces))


def parse_S(tokens):
    """S mid_num mid_den rad_num rad_den A H d backend -> (lo, hi, c, H) exact, with c = A/d."""
    mn, md, rn, rd, A, H, d = [int(t) for t in tokens[1:8]]
    mid, rad = F(mn, md), F(rn, rd)
    return (mid - rad, mid + rad, F(A, d), F(H, d), mid, rad)


def canon_fin(c, N):
    """Canonical centre of c + N Zhat: c reduced into [0, N) for N > 0."""
    c, N = F(c), F(N)
    if N == 0:
        return c
    return c - N * floor(c / N)


# ---------------------------------------------------------------- exact membership in A/Q

def member(s, z, pieces):
    """Is the class of (s, z) (s real rational, z rational finite point) in union pi(piece)?
    pieces: (lo, hi, c, N). Exists rational q: s+q in [lo,hi], z+q in c + N Zhat.
    For N > 0: z + q - c in N Zhat intersect Q = N Z, so q in c - z + N Z. For N = 0: q = c - z."""
    for (lo, hi, c, N) in pieces:
        if member1(s, z, lo, hi, c, N):
            return True
    return False


def member1(s, z, lo, hi, c, N):
    base = c - z
    if N == 0:
        return lo <= s + base <= hi
    # need k with lo <= s + base + k N <= hi
    k0 = ceil((lo - s - base) / N)
    return s + base + k0 * N <= hi
