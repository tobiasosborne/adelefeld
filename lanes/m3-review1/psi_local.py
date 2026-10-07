#!/usr/bin/env python3
"""Lane m3-review1: the local and place calls of slices 3.2-b, 3.2-c, by sampled exact phases.
lball a + p^N Z_p: points a + p^N k, phase E(fp_p(point)) (own fp from psi_check). psi_at: at inf E(-s) for s in
the real ball (notes.txt:693-694), at p E(fp_p(z)) for z = c + N k. Product over places of the exact x against
E(c - m) with mpmath interval products. E3 witness."""
import random
import sys
from fractions import Fraction as F
from math import floor, ceil, gcd
import mpmath
from mpmath import iv
from common import Piece, enc_class, run

mpmath.mp.dps = 130
iv.dps = 130
TOL = mpmath.mpf(10) ** -115
seed = int(sys.argv[1]) if len(sys.argv) > 1 else 1
NC = int(sys.argv[2]) if len(sys.argv) > 2 else 1000
EXE = sys.argv[3] if len(sys.argv) > 3 else 'h'
rng = random.Random(seed)

BIGP = [18446744073709551557, 18446744073709551533, 2305843009213693951]


def fp(q, p):
    q = F(q)
    d, h = q.denominator, 0
    while d % p == 0:
        d //= p
        h += 1
    if h == 0:
        return F(0)
    P = p ** h
    if P <= 2000:
        sols = [k for k in range(P) if (q - F(k, P)).denominator % p != 0]
        r = F(sols[0], P)
    else:
        r = F((q.numerator * pow(d, -1, P)) % P, P)
    assert (q - r).denominator % p != 0 and 0 <= r < 1
    return r


def frac(t):
    return t - floor(t)


def cs(t):
    a = 2 * mpmath.pi * mpmath.mpf(t.numerator) / t.denominator
    return mpmath.cos(a), mpmath.sin(a)


def box(t, off):
    v = [F(int(t[off + 2 * j]), int(t[off + 2 * j + 1])) for j in range(4)]
    m = lambda x: mpmath.mpf(x.numerator) / x.denominator
    return (m(v[0] - v[1]), m(v[0] + v[1]), m(v[2] - v[3]), m(v[2] + v[3])), v


def inb(t, b):
    c, s = cs(t)
    return b[0] - TOL <= c <= b[1] + TOL and b[2] - TOL <= s <= b[3] + TOL


def vp(q, p):
    q = F(q)
    if q == 0:
        return None
    v, n, d = 0, q.numerator, q.denominator
    while n % p == 0:
        n //= p
        v += 1
    while d % p == 0:
        d //= p
        v -= 1
    return v


def main():
    bad = 0
    lines, cases = [], []
    # lball
    for _ in range(NC):
        p = rng.choice([2, 2, 3, 5, 7, 11] + BIGP)
        h = rng.randint(-6, 6)
        u = F(rng.randint(-10 ** 6, 10 ** 6) or 1, rng.choice([1, 3, 7, 9, 25, 11]))
        c = u * F(p) ** h if p < 100 else u * F(p) ** max(min(h, 2), -2)
        ex = rng.random() < 0.25
        N = rng.randint(-6, 6) if p < 100 else rng.randint(-2, 2)
        prec = rng.choice([2, 20, 53, 128])
        lines.append('L %d %d %d %d %d %d' % (prec, p, c.numerator, c.denominator, N, int(ex)))
        cases.append(('L', p, c, N, ex, prec))
    # psi_at
    for _ in range(NC):
        mid = F(rng.randint(-2 ** 20, 2 ** 20), 2 ** rng.randint(0, 30))
        rad = rng.choice([F(0), F(0), F(rng.randint(1, 2 ** 20), 2 ** rng.randint(10, 40))])
        p0 = rng.choice([2, 3, 5, 7] + BIGP[:1])
        den = rng.choice([1, 2, 4, 8, 3, 9, 360, 2 ** 20, 5 ** 7]) * (p0 if p0 > 100 and rng.random() < 0.5 else 1)
        c = F(rng.randint(-10 ** 8, 10 ** 8), den)
        N = rng.choice([F(0), F(0), F(1), F(12), F(360), F(1, 2), F(5, 8), F(2, 9), F(7, 360), F(3, 2 ** 20)])
        pc = Piece(mid, rad, c, N)
        prec = rng.choice([2, 20, 53, 128])
        places = [0, 2, 3, 5, 7, 13] + BIGP[:1]
        v = rng.choice(places)
        lines.append('T %d %d %s' % (prec, v, enc_class(0, [pc])))
        cases.append(('T', pc, v, prec))
    # product over places for exact x
    for _ in range(NC // 4):
        mid = F(rng.randint(-2 ** 20, 2 ** 20), 2 ** rng.randint(0, 20))
        den = rng.choice([1, 2, 6, 360, 8 * 27 * 25 * 7, 2 ** 10 * 3 ** 5])
        c = F(rng.randint(-10 ** 8, 10 ** 8), den)
        N = rng.choice([F(0), F(1), F(12)])
        pc = Piece(mid, F(0), c, N)
        prec = rng.choice([53, 128])
        ps = [p for p in (2, 3, 5, 7) if c.denominator % p == 0]
        for v in [0] + ps:
            lines.append('T %d %d %s' % (prec, v, enc_class(0, [pc])))
        cases.append(('X', pc, [0] + ps, prec))
    out, _ = run(lines, EXE)
    i = 0
    cnt = {}
    for cs_ in cases:
        kind = cs_[0]
        if kind == 'L':
            _, p, c, N, ex, prec = cs_
            if out[i].startswith('X'):
                cnt['Lskip'] = cnt.get('Lskip', 0) + 1
                i += 1
                continue
            C, K, H, V = [out[i + j].split() for j in range(4)]
            i += 4
            vN, exact = int(V[2]), int(V[3])
            frac_ = (not exact) and vN < 0
            pts = [c]
            if not exact:
                ks = [rng.randint(-10 ** 6, 10 ** 6) for _ in range(10)]
                if frac_ and p ** (-vN) <= 400:
                    ks += list(range(p ** (-vN)))
                pts += [c + F(p) ** vN * k for k in ks]
            for tag, t in (('C', C), ('K', K)):
                st = int(t[1])
                if tag == 'K' and frac_:
                    if st != 1 or t[2] != '1':
                        print('FINDING lball strict', p, c, N, ex, t); bad += 1
                    continue
                if st != 0:
                    print('FINDING lball status', tag, p, c, N, ex, t); bad += 1
                    continue
                b, _ = box(t, 3)
                for z in pts:
                    if not inb(fp(z, p), b):
                        print('FINDING lball enclosure', tag, p, c, N, ex, z, fp(z, p)); bad += 1
                        break
            st = int(H[1])
            if frac_:
                if st != 1 or H[2] != '1':
                    print('FINDING lball phase nd', p, c, N, ex, H); bad += 1
            elif st != 0 or F(int(H[3]), int(H[4])) != fp(c, p):
                print('FINDING lball phase', p, c, N, ex, H, fp(c, p)); bad += 1
            cnt['L'] = cnt.get('L', 0) + 1
        elif kind == 'T':
            _, pc, v, prec = cs_
            if out[i].startswith('X'):
                i += 1
                continue
            I, J = out[i].split(), out[i + 1].split()
            i += 2
            if v == 0:
                ss = [pc.lo, pc.hi, pc.mid] + [pc.lo + (pc.hi - pc.lo) * F(rng.randint(0, 99), 99) for _ in range(5)]
                phs = [frac(-s) for s in ss]
                fracr = False
            else:
                ks = [0, 1, -1] + [rng.randint(-10 ** 6, 10 ** 6) for _ in range(10)]
                vpN = vp(pc.N, v)
                fracr = vpN is not None and vpN < 0
                if fracr and v ** (-vpN) <= 400:
                    ks += list(range(v ** (-vpN)))
                phs = [fp(pc.c + pc.N * k, v) for k in ks]
            for tag, t in (('I', I), ('J', J)):
                st = int(t[1])
                if tag == 'J' and fracr:
                    if st != 1 or t[2] != '1' or t[3] != '1':
                        print('FINDING at strict', v, pc.enc(), t); bad += 1
                    continue
                if st != 0 or t[3] != '1':
                    print('FINDING at status/where', tag, v, pc.enc(), t); bad += 1
                    continue
                b, _ = box(t, 4)
                for th in phs:
                    if not inb(th, b):
                        print('FINDING at enclosure', tag, v, pc.enc(), th, prec); bad += 1
                        break
            cnt['T'] = cnt.get('T', 0) + 1
        else:
            _, pc, places, prec = cs_
            prod = iv.mpc(1, 0)
            okall = True
            for v in places:
                I = out[i].split()
                i += 2
                if int(I[1]) != 0:
                    okall = False
                    continue
                _, vals = box(I, 4)
                m = lambda x: iv.mpf([mpmath.mpf(x.numerator) / x.denominator] * 2)
                re = m(vals[0]) + iv.mpf([-1, 1]) * m(vals[1])
                im = m(vals[2]) + iv.mpf([-1, 1]) * m(vals[3])
                prod = iv.mpc(prod.real * re - prod.imag * im, prod.real * im + prod.imag * re)
            theta = frac(pc.c - pc.mid)
            c_, s_ = cs(theta)
            if not okall or not (prod.real.a - TOL <= c_ <= prod.real.b + TOL and prod.imag.a - TOL <= s_ <= prod.imag.b + TOL):
                print('FINDING product over places', pc.enc(), places, prod, c_, s_); bad += 1
            cnt['X'] = cnt.get('X', 0) + 1
    # E3 witness
    e3l = enc_class(0, [Piece(0, 0, 0, F(1, 2))])
    e3r = enc_class(1, [Piece(0, 0, 0, 1), Piece(F(1, 2), 0, 0, 1)])
    o, _ = run(['P 53 ' + e3l, 'P 53 ' + e3r], EXE)
    print('E3 lift   :', [l[:40] for l in o if l[0] in 'CK'])
    print('E3 reduced:', [l[:40] for l in o[7:] if l[0] in 'CK'])
    print('counts', cnt, 'findings', bad)


main()
