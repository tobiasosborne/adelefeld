#!/usr/bin/env python3
"""Exact oracle for docs/api-3.md. Run: timeout 120 python3 proto/quotient3_checks.py.

No C implementation is imported. All finite work and interval endpoints use Fraction.
mpmath evaluations use 90 decimal digits and a 1e-75 comparison margin; they are
numerical evidence, not interval certificates. The angle output itself is exact.
Sources: docs/proofs/quotient.md:103,125,177,203,232;
docs/proofs/analysis.md:78; refs/src/tate-poonen/notes.txt:693-700.
"""
from dataclasses import dataclass
from fractions import Fraction as F
from math import ceil, floor, gcd, lcm
from pathlib import Path
import random
import re


@dataclass(frozen=True)
class Ball:
    a: F
    N: F

    def __post_init__(self):
        object.__setattr__(self, 'a', F(self.a))
        object.__setattr__(self, 'N', F(self.N))
        if self.N < 0:
            raise ValueError('negative finite radius')


@dataclass(frozen=True)
class Adele:
    mid: F
    rad: F
    fin: Ball

    def __post_init__(self):
        object.__setattr__(self, 'mid', F(self.mid))
        object.__setattr__(self, 'rad', F(self.rad))
        if self.rad < 0:
            raise ValueError('negative real radius')


@dataclass(frozen=True, order=True)
class Piece:
    lo: F
    hi: F
    m: int
    N: int

    def __post_init__(self):
        object.__setattr__(self, 'lo', F(self.lo))
        object.__setattr__(self, 'hi', F(self.hi))
        if self.lo > self.hi or self.N < 0 or int(self.N) != self.N or int(self.m) != self.m:
            raise ValueError('invalid piece')
        object.__setattr__(self, 'N', int(self.N))
        object.__setattr__(self, 'm', int(self.m) % int(self.N) if self.N else int(self.m))


class LimitError(Exception):
    pass


def piece_key(p):
    return p.lo, p.hi, p.N, p.m


def reduce(real_mid, real_rad, finite_ball, piece_limit=None):
    """P6/P8 construction, sorted, duplicates removed; exact BEFORE arb rounding.

    The optional limit counts constructed pieces before deduplication or merging.
    None means unbounded Python reference work. Inputs must be rationals, not floats.
    """
    x = Adele(real_mid, real_rad, finite_ball)
    a, N = x.fin.a, x.fin.N
    A, B = (N.numerator, N.denominator) if N else (0, 1)
    if piece_limit is not None and B > piece_limit:
        raise LimitError('finite split exceeds piece limit')
    bounds, total = [], 0
    for j in range(B):
        lo, hi = x.mid - x.rad - a - j*N, x.mid + x.rad - a - j*N
        first = floor(lo)
        stop = first + 1 if lo == hi else ceil(hi)
        total += stop - first
        if piece_limit is not None and total > piece_limit:
            raise LimitError('closed construction exceeds piece limit')
        bounds.append((lo, hi, first, stop))
    out = [Piece(max(lo, n)-n, min(hi, n+1)-n, -n, A)
           for lo, hi, first, stop in bounds for n in range(first, stop)]
    return sorted(set(out), key=piece_key)


def normalize(family):
    """Re-reduce arbitrary stored intervals; do not clip rounding spill."""
    out = []
    for p in family:
        out += reduce((p.lo+p.hi)/2, (p.hi-p.lo)/2, Ball(p.m, p.N))
    return sorted(set(out), key=piece_key)


def cells(family):
    e = sorted({F(0), F(1)} | {t for p in family for t in (p.lo, p.hi)})
    return [s for a, b in zip(e, e[1:]) for s in (a, (a+b)/2) if 0 <= s < 1]


def fibers(family, s, L):
    """Positive cosets modulo L plus residual integer singleton fibers (Statement Q2)."""
    residues, points = set(), set()
    for p in family:
        centers = ([p.m] if p.lo <= s <= p.hi and s < 1 else [])
        if s == 0 and p.hi == 1:
            centers.append(p.m-1)
        for m in centers:
            if p.N:
                residues.update(range(m % p.N, L, p.N))
            else:
                points.add(m)
    return residues, {m for m in points if m % L not in residues}


def compare(x, y):
    """Returns (equal, first_inside_second, overlap) for represented SETS, all radii."""
    x, y = normalize(x), normalize(y)
    L = lcm(*(p.N for p in x+y if p.N))
    inside_xy, inside_yx, overlap = True, True, False
    for s in cells(x+y):
        R, E = fibers(x, s, L)
        S, D = fibers(y, s, L)
        inside_xy &= R <= S and all(e % L in S or e in D for e in E)
        inside_yx &= S <= R and all(d % L in R or d in E for d in D)
        overlap |= bool(R & S or E & D or any(e % L in S for e in E)
                        or any(d % L in R for d in D))
    return bool(inside_xy and inside_yx), bool(inside_xy), bool(overlap)


def direct_member(family, s, w):
    """Independent definition: enumerate integer translations, no normalization or fibers."""
    for p in family:
        for n in range(ceil(p.lo-s), floor(p.hi-s)+1):
            delta = w+n-p.m
            if (delta == 0 if p.N == 0 else delta % p.N == 0):
                return True
    return False


def lift_member(x, s, w):
    """Definition using rational finite points a+Nk, without P8 splitting."""
    lo, hi, a, N = x.mid-x.rad, x.mid+x.rad, x.fin.a, x.fin.N
    if not N:
        return lo <= s+a-w <= hi
    first, last = ceil((lo-s-a+w)/N), floor((hi-s-a+w)/N)
    return first <= last


@dataclass(frozen=True)
class PhaseImage:
    base: F
    rad: F
    order: int


def psi(adele):
    """Rational angle mod 1 for a singleton; else exact compact union of arcs.

    PhaseImage(b,r,B) means union_k E([b+k/B-r,b+k/B+r]), 0 <= k < B.
    No enumeration is needed to store even an enormous B.
    """
    x = adele
    b, B = (x.fin.a-x.mid) % 1, x.fin.N.denominator
    if x.rad == 0 and B == 1:
        return b
    return PhaseImage(b, x.rad, B)


def phase_arcs(value):
    """Materialize small images on [0,1], with 0 and 1 identified; closed intervals."""
    if isinstance(value, F):
        return [(value, value)]
    if 2*value.rad*value.order >= 1:
        return [(F(0), F(1))]
    out = []
    for k in range(value.order):
        c = (value.base + F(k, value.order)) % 1
        lo, hi = c-value.rad, c+value.rad
        if lo < 0:
            out += [(F(0), hi), (lo+1, F(1))]
        elif hi > 1:
            out += [(lo, F(1)), (F(0), hi-1)]
        else:
            out.append((lo, hi))
    out.sort()
    merged = []
    for lo, hi in out:
        if merged and lo <= merged[-1][1]:
            merged[-1] = merged[-1][0], max(hi, merged[-1][1])
        else:
            merged.append((lo, hi))
    return merged


def nearest_distance(value, target):
    if isinstance(value, F):
        value = PhaseImage(value, F(0), 1)
    t = (value.order*(target-value.base)) % 1
    return max(F(0), min(t, 1-t)/value.order-value.rad)


def hull(value):
    """Numerical rectangular hull, constant number of evaluations for any denominator."""
    import mpmath as mp
    def cosdist(t):
        d = nearest_distance(value, F(t))
        # Exact rational extrema, including quarter turns, avoid a numerical fake zero.
        if d in (F(0), F(1, 4), F(1, 2)):
            return mp.mpf({F(0): 1, F(1, 4): 0, F(1, 2): -1}[d])
        return mp.cos(2*mp.pi*mp.mpf(d.numerator)/d.denominator)
    return -cosdist(F(1, 2)), cosdist(0), -cosdist(F(3, 4)), cosdist(F(1, 4))


def local_phase(a, p):
    """fp_p(a), exact; p is a prime precondition."""
    a = F(a)
    power, d = 1, a.denominator
    while d % p == 0:
        d //= p
        power *= p
    return F((a.numerator*pow(d, -1, power)) % power, power) if power > 1 else F(0)


def local_image(a, radius, p):
    """radius is None for exact, else exponent e of p^e Z_p."""
    b = local_phase(a, p)
    return b if radius is None or radius >= 0 else PhaseImage(b, F(0), p**(-radius))


def rgcd(a, b):
    a, b = F(a), F(b)
    return F(gcd(a.numerator, b.numerator), lcm(a.denominator, b.denominator))


def add(x, y):
    return Adele(x.mid+y.mid, x.rad+y.rad, Ball(x.fin.a+y.fin.a, rgcd(x.fin.N, y.fin.N)))


def neg(x):
    return Adele(-x.mid, x.rad, Ball(-x.fin.a, x.fin.N))


def round_binary(q, bits, upward=False):
    """Positive rational to bits significant binary digits, nearest-even or upward."""
    q = F(q)
    if not q:
        return F(0)
    if q < 0:
        raise ValueError('nonnegative rounding kernel')
    e = q.numerator.bit_length()-q.denominator.bit_length()+1
    while q < F(2)**(e-1):
        e -= 1
    while q >= F(2)**e:
        e += 1
    unit = F(2)**(e-bits)
    t = q/unit
    n = floor(t)
    if upward:
        n = ceil(t)
    elif t-n > F(1, 2) or (t-n == F(1, 2) and n % 2):
        n += 1
    return n*unit


def q1_radius(d):
    """Q1 radius stated again independently, in exact rationals and integer arithmetic.

    A 30-bit number is k*2^(e-30) with 2^29 <= k < 2^30. The kernel is the least such number that
    is >= d, then its successor. Written without round_binary, so that a change of round_binary's
    rounding direction cannot hide behind this function.
    """
    if d <= 0:
        return F(0)
    e = d.numerator.bit_length()-d.denominator.bit_length()+1
    while d < F(2)**(e-1):
        e -= 1
    while d >= F(2)**e:
        e += 1
    u = F(ceil(d/F(2)**(e-30)))*F(2)**(e-30)
    return u + F(2)**((e if u < F(2)**e else e+1)-30)


def round_piece(p, prec):
    """Q1 kernel: midpoint RN_p, radius strictly above a positive required radius.

    One successor after RU30 gives a specified, bounded outward margin. Zero stays
    zero. This models the proposed kernel, not a claim of bit identity with arb.
    """
    m = round_binary((p.lo+p.hi)/2, max(prec, 2))
    r = round_binary(max(m-p.lo, p.hi-m), 30, True)
    if r:
        # Least 30-bit successor: r has an exact binade found without floats.
        e = r.numerator.bit_length()-r.denominator.bit_length()+1
        if r < F(2)**(e-1):
            e -= 1
        r += F(2)**(e-30)
    return Piece(m-r, m+r, p.m, p.N)


CHECKS = []


def record(name, count, detail):
    CHECKS.append(name)
    print(f'PASS {name}: {count} cases; {detail}')


def check_reduction():
    n = points = 0
    for a in (F(-2, 3), F(0), F(5, 4)):
        for N in (F(0), F(1), F(2), F(1, 2), F(2, 3), F(3, 2)):
            for lo in (F(-5, 4), F(0), F(9, 10), F(1)):
                for width in (F(0), F(1, 10), F(1), F(5, 2)):
                    x = Adele(lo+width/2, width/2, Ball(a, N))
                    ps = reduce(x.mid, x.rad, x.fin)
                    q = F(-7, 11)
                    assert ps == reduce(x.mid+q, x.rad, Ball(a+q, N))
                    for s in cells(ps):
                        for w in range(-5, 6):
                            assert direct_member(ps, s, w) == lift_member(x, s, w)
                            points += 1
                    n += 1
    record('reduction', n, f'{points} independent rational membership comparisons; translation invariant')


def check_count_limit():
    n = 0
    for lo in (F(-2), F(-1, 3), F(0), F(1, 3)):
        for width in (F(0), F(1, 3), F(1), F(3)):
            hi = lo+width
            k = sum(lo < j < hi for j in range(floor(lo), ceil(hi)+1))
            expected = k+1
            assert len(reduce((lo+hi)/2, width/2, Ball(0, 7), expected)) == expected
            try:
                reduce((lo+hi)/2, width/2, Ball(0, 7), expected-1)
                raise AssertionError('missing piece limit')
            except LimitError:
                pass
            n += 1
    for x in (Adele(0, 0, Ball(0, F(1, 10**100))), Adele(0, 10**100, Ball(0, 0))):
        try:
            reduce(x.mid, x.rad, x.fin, 10)
            raise AssertionError('large count was not refused')
        except LimitError:
            n += 1
    record('count_limit', n, 'closed endpoints, singleton, zero limit, enormous counts')


def brute_sets(x, y, wmax=20):
    """The three set relations by direct membership on a finite witness grid; no fibers, no glue."""
    eq, inc, ov, points = True, True, False, 0
    for s in cells(normalize(x+y)):
        for w in range(-wmax, wmax+1):
            a, b = direct_member(x, s, w), direct_member(y, s, w)
            eq &= a == b
            inc &= not a or b
            ov |= a and b
            points += 1
    return eq, inc, ov, points


def check_sets():
    rng = random.Random(3102)
    n = points = glue = 0
    for _ in range(120):
        fams = []
        for _side in range(2):
            fam = []
            for _piece in range(3):
                lo = F(rng.randrange(-4, 5), 4)
                fam.append(Piece(lo, lo+F(rng.randrange(5), 4), rng.randrange(-3, 4), rng.randrange(4)))
            fams.append(fam)
        x, y = fams
        norm = normalize(x+y)
        L = lcm(*(p.N for p in norm if p.N))
        # For this bounded generator [-20,20] covers all exact centers and every residue
        # with at least one non-exceptional integer. The symbolic proof handles all Zhat.
        eq, inc, ov, got = brute_sets(x, y)
        points += got
        assert compare(x, y) == (eq, inc, ov)
        assert L <= 6
        n += 1
    # The boundary glue (1 ; z) = (0 ; z - 1): pairs that meet only in the glued class at real 0.
    # Direct membership sees the class; compare must see it too.
    for mod in range(2, 9):
        for c in range(mod):
            x = [Piece(F(1, 2), F(1), c, mod)]
            y = [Piece(F(0), F(1, 2), c-1, mod)]
            assert compare(x, y) == (False, False, True) == brute_sets(x, y)[:3]
            glue += 1
            if mod >= 3:
                z = [Piece(F(0), F(1, 2), c+1, mod)]
                assert compare(x, z) == (False, False, False) == brute_sets(x, z)[:3]
                glue += 1
    record('sets', n+glue,
           f'{points} direct membership pairs; {glue} glued-endpoint pairs; mixed moduli, exact '
           f'fibers, spill')


def check_arithmetic():
    n = 0
    xs = [Adele(F(a, 3), F(r, 4), Ball(F(c, 3), F(N, 2)))
          for a in (-1, 1) for r in (0, 1) for c in (0, 1) for N in (0, 1, 4)]
    for x in xs:
        for y in xs:
            z = add(x, y)
            px, py = reduce(x.mid, x.rad, x.fin), reduce(y.mid, y.rad, y.fin)
            sums = [Piece(a.lo+b.lo, a.hi+b.hi, a.m+b.m, gcd(a.N, b.N)) for a in px for b in py]
            assert compare(reduce(z.mid, z.rad, z.fin), sums)[0]
            n += 1
        nx = neg(x)
        assert compare(reduce(nx.mid, nx.rad, nx.fin),
                       [Piece(-p.hi, -p.lo, -p.m, p.N) for p in px])[0]
    record('arithmetic', n+len(xs), 'sum distributes over pieces; negation includes exact fibers')


def check_rounding():
    n = 0
    for d in range(2, 14):
        for j in range(d):
            p = Piece(F(j, d), F(j+1, d), 0, 2)
            for prec in (2, 20, 53, 128):
                q = round_piece(p, prec)
                assert q.lo <= p.lo <= p.hi <= q.hi
                assert 0 <= (q.lo+q.hi)/2 <= 1
                m = (q.lo+q.hi)/2
                eta = abs(m-(p.lo+p.hi)/2)
                bound = 2*eta + F(1, 2**28)*max(m-p.lo, p.hi-m)
                assert p.lo-q.lo <= bound and q.hi-p.hi <= bound
                n += 1
    # Rounding direction, against the independent kernel q1_radius: the radius is rounded UP to 30
    # bits and the successor is taken. The successor alone encloses, so only cases whose NEAREST
    # 30-bit value is strictly below the required radius pin the direction.
    inward = 0
    for den in (7, 9, 11, 13, 17, 19, 23, 29, 31, 37):
        for num in range(1, den):
            p = Piece(F(num, den), F(num+1, den), 0, 2)
            for prec in (20, 53):
                q = round_piece(p, prec)
                m = round_binary((p.lo+p.hi)/2, max(prec, 2))
                need = max(m-p.lo, p.hi-m)
                assert (q.lo, q.hi) == (m-q1_radius(need), m+q1_radius(need))
                assert q.lo <= p.lo <= p.hi <= q.hi
                if need and round_binary(need, 30, False) < need:
                    assert round_binary(need, 30, False) < need <= q1_radius(need)
                    inward += 1
                n += 1
    assert inward
    for prec in (20, 53, 128):
        q = round_piece(Piece(F(9, 10), 1, 0, 2), prec)
        assert q.hi > 1
        print(f'EXAMPLE non_dyadic prec={prec} excess_hi={q.hi-1}')
    print(f'EXAMPLE rounding_direction: {inward} of {n} required radii are rounded inward to nearest')
    record('rounding', n+3, f'RN midpoint, RU30 plus successor radius; {inward} inward-to-nearest '
                          f'radii pinned against q1_radius; containment and CV-45')


def check_phases():
    n = 0
    for den in range(1, 13):
        for num in range(-12, 13):
            a = F(num, den)
            assert sum((local_phase(a, p) for p in (2, 3, 5, 7, 11)), F(0)) % 1 == a % 1
            assert psi(Adele(a, 0, Ball(a, 0))) == 0
            for b in (F(-1, 7), F(0), F(2, 3)):
                for p in (2, 3, 5, 7, 11):
                    assert (local_phase(a, p)+local_phase(b, p)) % 1 == local_phase(a+b, p)
            n += 1
    for A in range(1, 8):
        for B in range(1, 8):
            if gcd(A, B) != 1:
                continue
            x = Adele(F(1, 7), 0, Ball(F(-2, 5), F(A, B)))
            exact = sorted({(x.fin.a-x.mid+F(k*A, B)) % 1 for k in range(B)})
            assert phase_arcs(psi(x)) == [(t, t) for t in exact]
            n += 1
    record('phases', n, 'product formula; local additivity at 2,3,5,7,11; finite split phases')


def check_hulls():
    import mpmath as mp
    mp.mp.dps = 90
    margin = mp.mpf('1e-75')
    n = 0
    for B in range(1, 9):
        for base in (F(0), F(1, 7), F(3, 4)):
            for r in (F(0), F(1, 100), F(1, 8), F(1)):
                image = PhaseImage(base, r, B)
                values = []
                for lo, hi in phase_arcs(image):
                    targets = {lo, hi} | {F(k, 4) for k in range(5) if lo <= F(k, 4) <= hi}
                    for t in targets:
                        values.append(mp.exp(2j*mp.pi*mp.mpf(t.numerator)/t.denominator))
                want = min(z.real for z in values), max(z.real for z in values)
                want += min(z.imag for z in values), max(z.imag for z in values)
                assert all(abs(a-b) < margin for a, b in zip(hull(image), want))
                n += 1
    assert hull(PhaseImage(F(0), F(0), 2)) == (-1, 1, 0, 0)
    assert hull(PhaseImage(F(0), F(1, 100), 10**100)) == (-1, 1, -1, 1)
    record('hulls', n+2, '90 digits; margin 1e-75; extrema oracle versus enumerated arc endpoints')


def check_local_images():
    n = 0
    for p in (2, 3, 5, 7):
        for a in (F(-1, 6), F(1, 3), F(0), F(2, 5)):
            for exponent in range(-3, 4):
                size = p**max(0, -exponent)
                expected = sorted({local_phase(a+F(p)**exponent*k, p) for k in range(size)})
                assert phase_arcs(local_image(a, exponent, p)) == [(t, t) for t in expected]
                n += 1
            assert local_image(a, None, p) == local_phase(a, p)
            n += 1
    record('local_images', n, 'exact and exponents -3 through 3 at 2,3,5,7')


def check_ball_additivity():
    rng = random.Random(3202)
    n = points = 0
    def member(arcs, t):
        return any(lo <= t <= hi or (t == 0 and hi == 1) for lo, hi in arcs)
    for _ in range(180):
        xs = [Adele(F(rng.randrange(-4, 5), 5), rng.choice((F(0), F(1, 20), F(1, 3))),
                    Ball(F(rng.randrange(-3, 4), 7), rng.choice((F(0), F(3), F(1, 2), F(2, 3)))))
              for _side in range(2)]
        left = phase_arcs(psi(add(*xs)))
        right = []
        for lo, hi in phase_arcs(psi(xs[0])):
            for lo2, hi2 in phase_arcs(psi(xs[1])):
                a, b = lo+lo2, hi+hi2
                if b-a >= 1:
                    right.append((F(0), F(1)))
                else:
                    for j in range(floor(a), floor(b)+1):
                        right.append((max(a, j)-j, min(b, j+1)-j))
        ends = sorted({F(0), F(1)} | {t for arc in left+right for t in arc})
        for a, b in zip(ends, ends[1:]):
            for t in (a, (a+b)/2):
                assert member(left, t) == member(right, t)
                points += 1
        n += 1
    record('ball_additivity', n, f'{points} endpoint/gap witnesses; independent arc sums')


def check_golden():
    n = invalid = 0
    root = Path(__file__).resolve().parents[1]
    for line in (root/'tests/golden/psi_phases.tsv').read_text().splitlines():
        if not line or line.startswith('#'):
            continue
        source, expected = line.split('\t')
        if expected.startswith('!'):
            invalid += 1
            continue
        body = source.removeprefix('(* ; ').removesuffix(')')
        parts = body.split(' mod ')
        value = psi(Adele(0, 0, Ball(F(parts[0]), F(parts[1]) if len(parts) == 2 else 0)))
        assert phase_arcs(value) == [(F(t), F(t)) for t in expected.split()]
        n += 1
    record('golden_phases', n, f'{invalid} syntax/domain rows reserved for C parser tests')


def parse_qclass_exact(text):
    """Small exact value-form reader for the golden subset; not a C parser oracle."""
    text = text.strip()
    if not text.endswith('+ Q') and not text.endswith('+Q'):
        raise ValueError('PARSE')
    body = text[:text.rfind('+')].strip()
    pieces = body.startswith('union(')
    if pieces:
        if not body.endswith(')'):
            raise ValueError('PARSE')
        body = body[6:-1]
    entries = re.findall(r'\(([^();]+);([^();]+)\)', body)
    residue = re.sub(r'\(([^();]+);([^();]+)\)', '', body).replace(',', '').strip()
    if residue or not entries or (not pieces and len(entries) != 1):
        raise ValueError('PARSE')
    out = []
    for real, finite in entries:
        if '*' in real:
            raise ValueError('PARSE')
        rs = real.split('+/-')
        fs = finite.split('mod')
        try:
            m, r = F(rs[0].strip()), F(rs[1].strip()) if len(rs) == 2 else F(0)
            a, N = F(fs[0].strip()), F(fs[1].strip()) if len(fs) == 2 else F(0)
        except ZeroDivisionError:
            raise ValueError('DOMAIN') from None
        if r < 0 or N < 0:
            raise ValueError('DOMAIN')
        if pieces and (not 0 <= m <= 1 or a.denominator != 1 or N.denominator != 1):
            raise ValueError('DOMAIN')
        out += reduce(m, r, Ball(a, N))
    return out


def check_golden_qclass():
    root = Path(__file__).resolve().parents[1]
    valid = invalid = 0
    for line in (root/'tests/golden/qclass.tsv').read_text().splitlines():
        if not line or line.startswith('#'):
            continue
        source, expected = line.split('\t')
        if expected.startswith('!'):
            try:
                parse_qclass_exact(source)
                raise AssertionError('accepted invalid vector')
            except ValueError as exc:
                assert str(exc) == expected[1:]
            invalid += 1
        else:
            assert compare(parse_qclass_exact(source), parse_qclass_exact(expected))[1]
            valid += 1
    record('golden_qclass', valid+invalid,
           f'{valid} exact text-set enclosure checks; {invalid} status rows; no printer implemented')


def check_full_and_width():
    import mpmath as mp
    mp.mp.dps = 90
    full = [Piece(0, 1, 0, 1)]
    n = 0
    for A in range(1, 6):
        for B in range(1, 5):
            N = F(A, B)
            for width in (N/2, N, N+F(1, 5)):
                ps = reduce(F(1, 7), width/2, Ball(F(2, 3), N))
                assert compare(ps, full)[0] == (width >= N)
                n += 1
    for width in (F(0), F(1, 20), F(1, 3), F(1, 2), F(3, 4), F(1)):
        phi = PhaseImage(F(1, 7), width/2, 1)
        delta = min(width, F(1, 2))
        chord = abs(mp.exp(2j*mp.pi*mp.mpf(delta.numerator)/delta.denominator)-1)
        bound = 2*mp.sin(mp.pi*mp.mpf(delta.numerator)/delta.denominator)
        H = hull(phi)
        assert abs(chord-bound) < mp.mpf('1e-75')
        assert H[1]-H[0] <= bound+mp.mpf('1e-75')
        assert H[3]-H[2] <= bound+mp.mpf('1e-75')
        n += 1
    record('full_and_width', n, 'positive fractional threshold; six arc diameter and coordinate-width cases')


def check_gauss_boundary():
    """Exact FLINT character angles, direct mpmath sum; no Gauss-sum API imported."""
    import flint
    import mpmath as mp
    mp.mp.dps = 90
    margin = mp.mpf('1e-75')
    root = Path(__file__).resolve().parents[1]
    n = negative = 0
    def angle(c, q, a):
        return F(int(c.chi_exponent(a)), int(flint.dirichlet_group(q).exponent())) % 1
    for line in (root/'tests/golden/gauss.tsv').read_text().splitlines():
        if not line or line.startswith('#'):
            continue
        source, expected = line.split('\t')
        q, label = map(int, re.search(r'q=(\d+), n=(\d+)', source).groups())
        if q == 1:
            tau, W, parity, minus = mp.mpc(1), mp.mpc(1), 0, mp.mpc(1)
        else:
            c = flint.dirichlet_char(q, label)
            f = int(c.conductor())
            if f != q:
                units = [a for a in range(1, q+1) if gcd(a, q) == 1]
                matches = [flint.dirichlet_char(f, m) for m in range(1, f+1) if gcd(m, f) == 1
                           and all(angle(flint.dirichlet_char(f, m), f, a) == angle(c, q, a)
                                   for a in units)]
                assert len(matches) == 1
                c, q = matches[0], f
            parity = int(c.parity())
            tau, minus = mp.mpc(0), mp.mpc(0)
            for a in range(1, q+1):
                if gcd(a, q) == 1:
                    t = angle(c, q, a)
                    plus_t, minus_t = (t+F(a, q)) % 1, (t-F(a, q)) % 1
                    tau += mp.expjpi(2*mp.mpf(plus_t.numerator)/plus_t.denominator)
                    minus += mp.expjpi(2*mp.mpf(minus_t.numerator)/minus_t.denominator)
            W = tau/(mp.j**parity*mp.sqrt(q))
        assert parity == int(expected[2])
        assert abs(minus-(-1)**parity*tau) < margin
        negative += parity
        boxes = re.findall(r'\(([-+\d.e]+) \+/- ([-+\d.e]+)\)', expected)
        assert len(boxes) == 4
        for value, (mid, rad) in zip((tau.real, tau.imag, W.real, W.imag), boxes):
            assert abs(value-mp.mpf(mid)) <= mp.mpf(rad)+margin
        n += 1
    record('gauss_boundary', n, f'{negative} odd-character sign controls; exact lowering; 90 digits, 1e-75 margin')


def check_examples_findings():
    # E1 wrap, E2 endpoint, E3 fractional split, E4 exact finite point.
    examples = [Adele(1, F(1, 10), Ball(0, 2)), Adele(F(1, 2), F(1, 2), Ball(0, 2)),
                Adele(0, 0, Ball(0, F(1, 2))), Adele(0, 0, Ball(0, 0))]
    for i, x in enumerate(examples, 1):
        print(f'EXAMPLE E{i}: {reduce(x.mid, x.rad, x.fin)}; psi={psi(x)}')
    # F1: width >= N does not characterize full image at N=0.
    assert not direct_member(reduce(0, 0, Ball(0, 0)), F(1, 2), 0)
    # Implementation warning, not a refutation: literal pieces use canonical finite centers.
    # Canonical finite center 0 mod 2 for a+q=2 changes the integer shift by 2;
    # normalized output is equal, raw -n differs by 2.
    assert -floor(F(1, 4)) != -floor(F(9, 4))
    assert (-floor(F(1, 4))) % 2 == (-floor(F(9, 4))) % 2
    # F3: strict status from constituents changes with representation, phase set does not.
    x = examples[2]
    ps = reduce(x.mid, x.rad, x.fin)
    assert x.fin.N.denominator == 2 and all(p.N.denominator == 1 for p in ps)
    assert phase_arcs(psi(x)) == [(F(0), F(0)), (F(1, 2), F(1, 2))]
    # F4: translation through rounded adeles enlarges the represented set.
    p = Piece(F(0), F(0), 0, 0)
    q = round_binary(F(1, 3), 8)
    r = round_binary(abs(q-F(1, 3)), 30, True)
    translated = reduce(q, r, Ball(F(1, 3), 0))
    assert compare([p], translated) == (False, True, True)
    # F2: concrete set/point distinction; also a spill witness for the comparison extension.
    assert compare([Piece(0, 1, 0, 2)], [Piece(0, 1, 0, 2)])[0]
    assert direct_member([Piece(F(-1, 16), F(1, 16), 0, 2)], F(31, 32), 1)
    # F5: an inside-domain dyadic enclosure exists, despite a non-dyadic endpoint.
    inside = Piece(F(15, 16)-F(1, 16), F(15, 16)+F(1, 16), 0, 2)
    assert 0 <= inside.lo <= F(9, 10) <= 1 == inside.hi
    print(f'EXAMPLE inside_domain: {inside}; encloses [9/10,1]')
    print(f'EXAMPLE rounded_translate: midpoint={q}, radius={r}; proper inclusion')
    record('examples_findings', 11,
           '4 reductions; zero-radius, strictness, rounding, set and inside-domain witnesses')


def check_fault_witnesses():
    """Twelve explicit wrong alternatives; these are controls, not a source mutation sweep.

    What this group proves: for each of the twelve named alternatives there is a concrete input on
    which the oracle's answer differs from the required one, so that a C implementation making that
    alternative is rejected by that witness. It does not prove that no other wrong alternative
    passes; the mutation run of lanes/q-review1/mutate_oracle.py is separate evidence for that.
    The invariant block below restates the storage rule of section 1 (the stored midpoint lies in
    [0,1], the stored ball encloses the exact one) on pieces whose exact end points are not dyadic.
    It is a second, independent place where a C implementation that violates the rule is caught; it
    is not evidence against a mutation that only deletes an assertion of a true fact.
    """
    invariant = 0
    p = reduce(1, F(1, 10), Ball(0, 2))
    assert len(p) != 1                         # Q1: k instead of k+1
    assert reduce(F(1, 2), F(1, 2), Ball(0, 2)) != p  # Q2: spurious endpoint wrap
    assert not compare([Piece(1, 1, 0, 3)], [Piece(0, 0, 1, 3)])[0]  # Q3: wrong glue
    assert len(reduce(0, 0, Ball(0, F(1, 3)))) != 1   # Q4: missing fractional split
    assert direct_member([Piece(F(1, 4), F(1, 2), 0, 2)], F(1, 3), 2)  # Q5: refinement
    assert compare([Piece(0, 0, 0, 0)], [Piece(0, 0, 0, 1)]) != (True, True, True)  # Q6
    assert psi(Adele(0, 0, Ball(F(1, 3), 0))) != F(2, 3)  # P1: finite sign
    assert psi(Adele(F(1, 3), 0, Ball(0, 0))) != F(1, 3)  # P2: real sign
    assert local_phase(F(1, 6), 2) != F(1, 6)             # P3: ordinary fractional part
    assert len(phase_arcs(psi(Adele(0, 0, Ball(0, F(2, 3)))))) == 3  # P4: numerator roots
    assert nearest_distance(PhaseImage(F(0), F(1, 10), 1), F(0)) == 0  # P5: miss extrema
    assert hull(PhaseImage(F(0), F(0), 2)) != (-1, 1, -1, 1)  # P6: universal square
    for p in (Piece(F(9, 10), 1, 0, 2), Piece(F(-1, 16), F(1, 16), 0, 2),
              Piece(F(0), F(1), 1, 3), Piece(F(-1, 3), F(1, 3), 5, 0)):
        for prec in (2, 20, 53):
            q = round_piece(p, prec)
            assert 0 <= (q.lo+q.hi)/2 <= 1  # CV-45: the midpoint, not the end points
            assert q.lo <= p.lo <= p.hi <= q.hi
            invariant += 1
    record('fault_witnesses', 12+invariant,
           f'6 quotient and 6 phase wrong alternatives rejected; {invariant} midpoint and enclosure '
           f'checks of the section 1 invariant')


def main():
    for check in (check_reduction, check_count_limit, check_sets, check_arithmetic, check_rounding,
                  check_phases, check_hulls, check_local_images, check_ball_additivity,
                  check_golden, check_golden_qclass, check_full_and_width,
                  check_gauss_boundary, check_examples_findings, check_fault_witnesses):
        check()
    print(f'{len(CHECKS)} checks')


if __name__ == '__main__':
    main()
