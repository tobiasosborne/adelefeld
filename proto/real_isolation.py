#!/usr/bin/env python3
"""Reference of docs/design/real-roots.md (issue adf-8di): the real roots of an integer polynomial by bisection
with Descartes' rule of signs in exact integer arithmetic, then refinement of each isolating interval.

Plain Python 3, standard library only. A polynomial is a list of integers, the constant term first.

Sources (CLAUDE.md rule 3), under refs/src/:
  [SM] sagraloff-mehlhorn/tex/arxivfinal.tex
       :547 to 553   Descartes' rule of signs for an interval (a, b): with P_I(x) = (x+1)^n P((a x + b)/(x + 1))
                     and v_I the number of sign variations of its coefficients, v_I >= m_I and v_I = m_I modulo 2,
                     m_I the number of roots in I with multiplicity; so v_I = m_I if v_I <= 1. Zero
                     coefficients are not considered (footnote of :547).
       :557 to 569   Obreshkoff; the one-circle and two-circle theorems; "each interval I of width
                     w(I) < sigma_P / 2 yields v_I = 0 or v_I = 1" (:569). Used for the termination only.
       :302          the size of the tree, O(n (tau + log n)), and the cost of the method (a citation of
                     Eigenwillig, Sharma and Yap there; that paper is not on disk).
  [KS] kerber-sagraloff/tex/arxiv.tex
       :278 to 298   Algorithm EQIR (exact quadratic interval refinement); :338 to 341 how N changes.

The design, in the words of the design file:
  Algorithm D   isolation: design section 4, Propositions R2 and R3.
  Algorithm F   refinement: design section 5, Proposition R4 (steps G, stop, Q, B; lane r-slice1 added step G,
                the static floor and the removal of the cap on N, as in src/roots_real.c).
  Algorithm RR2 the whole: design section 5, Proposition R5.

What is exact here: everything. No floating point number occurs. A point is c 2^k with integers c and k; the
cell (c, k) is the open interval (c 2^k, (c + 1) 2^k).
"""
from fractions import Fraction as F
from math import gcd

OK, NOT_DETERMINED, DOMAIN = "OK", "NOT_DETERMINED", "DOMAIN"


# ---------------------------------------------------------------------------------------------------------------
# polynomials

def ptrim(f):
    f = list(f)
    while f and f[-1] == 0:
        f.pop()
    return f


def sgn(x):
    return (x > 0) - (x < 0)


def pderiv(f):
    return [i * f[i] for i in range(1, len(f))]


def _primitive(f):
    c = 0
    for x in f:
        c = gcd(c, x)
    f = [x // c for x in f]
    return [-x for x in f] if f[-1] < 0 else f


def _prem(a, b):
    """A nonzero integer multiple of the remainder of a by b in Q[X] (pseudo-remainder), made primitive; []
    if b divides a over Q."""
    a = list(a)
    while len(a) >= len(b):
        if a[-1] != 0:
            m, s = a[-1], len(a) - len(b)
            a = [b[-1] * x for x in a]
            for i, y in enumerate(b):
                a[s + i] -= m * y
        a.pop()
        a = ptrim(a) if not any(a) else a
        if not a:
            return []
    a = ptrim(a)
    return _primitive(a) if a else []


def _divexact(f, h):
    """f / h for integer polynomials with h | f in Q[X] and f / h in Z[X] (h primitive: the lemma of Gauss)."""
    f, q = list(f), [0] * (len(f) - len(h) + 1)
    for s in range(len(f) - len(h), -1, -1):
        t, r = divmod(f[s + len(h) - 1], h[-1])
        assert r == 0
        q[s] = t
        for i, y in enumerate(h):
            f[s + i] -= t * y
    assert not any(f)
    return q


def squarefree_part(f):
    """f / gcd(f, f'), primitive, with a positive leading coefficient (solvers.md Lemma 3.1(2))."""
    f = _primitive(ptrim(f))
    if len(f) == 1:
        return [1]
    a, b = f, _primitive(pderiv(f))
    while b:
        a, b = b, _prem(a, b)
    return _primitive(_divexact(f, a)) if len(a) > 1 else f


def ev(g, m, e):
    """The integer 2^(max(-e, 0) deg g) g(m 2^e); its sign is the sign of g(m 2^e). Horner's rule in the
    homogeneous form of src/roots.c:1262 to 1298."""
    d = len(g) - 1
    if e >= 0:
        x, r = m << e, 0
        for c in reversed(g):
            r = r * x + c
        return r
    k, r = -e, g[d]
    for i in range(d - 1, -1, -1):
        r = r * m + (g[i] << (k * (d - i)))
    return r


def var(c):
    """The number of sign variations of a sequence; zeros are not considered ([SM]:547, footnote)."""
    v, last = 0, 0
    for x in c:
        s = sgn(x)
        if s and last and s != last:
            v += 1
        if s:
            last = s
    return v


def shift1(q):
    """q(X + 1), by the additions of the scheme of Horner (deg^2 / 2 additions)."""
    q = list(q)
    n = len(q)
    for i in range(n - 1):
        for j in range(n - 2, i - 1, -1):
            q[j] += q[j + 1]
    return q


def var01(q):
    """v_I of [SM]:549 to 552 for I = (0, 1): the sign variations of (X + 1)^n q(1 / (X + 1)), which is the
    reversed polynomial shifted by 1. The map x -> 1 / (x + 1) sends (0, infinity) onto (0, 1); [SM] use
    x -> (a x + b) / (x + 1) = b / (x + 1) for a = 0, b = 1, the same map."""
    return var(shift1(list(reversed(q))))


def _strip2(q):
    """q divided by the largest power of 2 that divides all its coefficients (the signs do not change)."""
    t = min(((x & -x).bit_length() - 1) for x in q if x)
    return [x >> t for x in q] if t else q


def root_bound_exp(g):
    """The least integer K (it may be negative) with |c_d| 2^(K d) > sum_(i < d) |c_i| 2^(K i), for g of degree
    d >= 1 with g(0) != 0. Then every complex root z of g has |z| < 2^K (design, Lemma R1). Found by bisection
    of K between -bits(c_d) - 1, where the test fails, and bits(max |c_i|) + 1, where it holds."""
    d = len(g) - 1
    a = [abs(c) for c in g]

    def holds(K):
        if K >= 0:
            return (a[d] << (K * d)) > sum(a[i] << (K * i) for i in range(d))
        return a[d] > sum(a[i] << (-K * (d - i)) for i in range(d))

    lo, hi = -a[d].bit_length() - 1, max(a[:d]).bit_length() + 1
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if holds(mid):
            hi = mid
        else:
            lo = mid
    return hi


# ---------------------------------------------------------------------------------------------------------------
# Algorithm D: isolation

def isolate_positive(g, stats):
    """The positive roots of the squarefree g, g(0) != 0: a list of exact roots (c, k), meaning c 2^k, and a
    list of cells (c, k), each an open interval with exactly one root of g, no root of g elsewhere in (0, oo)."""
    d = len(g) - 1
    K = root_bound_exp(g)
    if K >= 0:
        q = [g[i] << (K * i) for i in range(d + 1)]
    else:
        q = [g[i] << (-K * (d - i)) for i in range(d + 1)]
    points, cells, work = [], [], [(_strip2(q), 0, K)]
    while work:
        q, c, k = work.pop()
        if len(q) == 1:
            continue
        stats["nodes"] += 1
        stats["bits"] = max(stats["bits"], max(abs(x).bit_length() for x in q))
        v = var01(q)
        if v == 0:
            continue
        if v == 1:
            cells.append((c, k))
            continue
        n = len(q) - 1
        left = [q[i] << (n - i) for i in range(n + 1)]             # 2^n q(X / 2): the left half on (0, 1)
        right = shift1(left)                                       # the right half on (0, 1)
        if right[0] == 0:                                          # the midpoint is a root: exact, divided out
            points.append((2 * c + 1, k - 1))
            right = right[1:]                                      # by X
            left = _divexact(left, [-1, 1])                        # by X - 1
        work.append((_strip2(left), 2 * c, k - 1))
        work.append((_strip2(right), 2 * c + 1, k - 1))
    return points, cells


def isolate(g, stats):
    """All real roots of the squarefree g of degree >= 1: (points, cells), cells (c, k) with c of any sign."""
    points, cells = [], []
    if g[0] == 0:
        points.append((0, 0))
        g = g[1:]                                                  # squarefree: g(0) != 0 now
    if len(g) > 1:
        p, c = isolate_positive(g, stats)
        points += p
        cells += c
        h = [x if i % 2 == 0 else -x for i, x in enumerate(g)]     # g(-X)
        p, c = isolate_positive(h, stats)
        points += [(-m, k) for m, k in p]
        cells += [(-m - 1, k) for m, k in c]                       # (m 2^k, (m+1) 2^k) mirrored
    return points, cells


# ---------------------------------------------------------------------------------------------------------------
# Algorithm F: refinement

def value(c, k):
    return F(c) * F(2) ** k


def accuracy(c, k):
    """arb_rel_accuracy_bits of the ball with the end points c 2^k and (c + 1) 2^k (flint-3.0.1:arb.rst:499 to
    509): the midpoint is (2c + 1) 2^(k-1), the radius 2^(k-1), so the accuracy is bits(|2c + 1|) - 2."""
    return abs(2 * c + 1).bit_length() - 2


def gallop(g, dg, c, k, anchor_hi, sign_at):
    """Step G (design R4): the cell (c, k) holds exactly one root r; p, its left end (anchor_hi False) or its right
    end, is 0 or a root of g. P(t): sign g(p + sigma 2^t) = v0 holds exactly for 2^t < |r - p|. Returns
    ("point", m, t) for the root m 2^t, or ("cell", c', T, far_is_old) with T = floor(log2 |r - p|)."""
    sigma = -1 if anchor_hi else 1
    pc = c + 1 if anchor_hi else c                                 # p = pc 2^k
    v0 = sgn(ev(g, pc, k))
    if v0 == 0:
        v0 = sigma * sgn(ev(dg, pc, k))

    def at(t):                                                     # the point p + sigma 2^t as (m, t)
        return (pc << (k - t)) + sigma

    hi_t, lo_t, step = k, None, 1
    while lo_t is None:                                            # galloping: t = k - 1, k - 2, k - 4, ...
        t = min(k - step, hi_t - 1)
        s = sign_at(at(t), t)
        if s == 0:
            return ("point", at(t), t)
        if s == v0:
            lo_t = t
        else:
            hi_t = t
        step *= 2
    while hi_t - lo_t > 1:                                         # bisection of the exponent
        mid = (lo_t + hi_t) // 2
        s = sign_at(at(mid), mid)
        if s == 0:
            return ("point", at(mid), mid)
        if s == v0:
            lo_t = mid
        else:
            hi_t = mid
    T = lo_t
    m = pc << (k - T)
    return ("cell", m + 1 if sigma > 0 else m - 2, T, T == k - 1, v0)


def refine(g, dg, c, k, floor, need, method, stats):
    """Algorithm F of the design (as src/roots_real.c refine_cell). The cell (c, k) holds exactly one root of the
    squarefree g and no other root in its interior. Returns (lo, hi), rationals, with lo = hi a root, or lo < hi,
    g(lo) g(hi) < 0, lo > floor (if floor is not None), and accuracy >= need; [lo, hi] lies in the closed cell.
    floor is the right end of the previous item as isolated (static), so the sequence of cells does not depend
    on need nor on floor (design R4(4)). method "qir" uses step Q, "bisect" does not; both use step G."""
    def sign_at(m, e):
        stats["evaluations"] += 1
        return sgn(ev(g, m, e))

    s_lo = sign_at(c, k)
    lo_clean = s_lo != 0
    if not lo_clean:                                               # the sign of g right of lo
        s_lo = sgn(ev(dg, c, k))
    hi_clean = sign_at(c + 1, k) != 0
    j = 2                                                          # N = 2^j, N = 4 at the start ([KS]:337)
    while True:
        if c in (0, -1) or not lo_clean or not hi_clean:          # step G: an anchor, 0 or a root at an end
            anchor_hi = (c == -1) if c in (0, -1) else lo_clean
            res = gallop(g, dg, c, k, anchor_hi, sign_at)
            if res[0] == "point":
                return value(res[1], res[2]), value(res[1], res[2])
            _, c2, T, far_is_old, v0 = res
            if not anchor_hi:
                lo_clean, s_lo = True, v0
                if not far_is_old:
                    hi_clean = True
            else:
                hi_clean = True
                if not far_is_old:
                    lo_clean, s_lo = True, -v0
            c, k = c2, T
            continue
        apart = floor is None or value(c, k) > floor
        if apart and accuracy(c, k) >= need:
            return value(c, k), value(c + 1, k)
        if method == "qir" and j > 1:
            jj = j                                                 # no cap: the sequence must not depend on need
            e = k - jj
            a = c << jj
            fa, fb = ev(g, a, e), ev(g, a + (1 << jj), e)          # the same scaling: the ratio is exact
            stats["evaluations"] += 2
            num, den = abs(fa) << jj, abs(fa) + abs(fb)
            t = (2 * num + den) // (2 * den)                       # round(N fa / (fa - fb)), [KS]:288
            s = sign_at(a + t, e)
            if s == 0:
                return value(a + t, e), value(a + t, e)
            new = None
            if s == s_lo and t < (1 << jj):
                s2 = sign_at(a + t + 1, e)
                if s2 == 0:
                    return value(a + t + 1, e), value(a + t + 1, e)
                if s2 == -s_lo:
                    new = a + t
            elif s == -s_lo and t > 0:
                s2 = sign_at(a + t - 1, e)
                if s2 == 0:
                    return value(a + t - 1, e), value(a + t - 1, e)
                if s2 == s_lo:
                    new = a + t - 1
            if new is not None:                                    # successful: N becomes N^2 ([KS]:338 to 339)
                c, k, j = new, e, 2 * j
                continue
            j //= 2                                                # failing: N becomes sqrt(N) ([KS]:339 to 340)
            if j > 1:
                continue
        # one bisection; after it N = 4 ([KS]:340 to 341)
        j = 2
        s = sign_at(2 * c + 1, k - 1)
        if s == 0:
            return value(2 * c + 1, k - 1), value(2 * c + 1, k - 1)
        if s == -s_lo:
            c, k, hi_clean = 2 * c, k - 1, True
        else:
            c, k, lo_clean, s_lo = 2 * c + 1, k - 1, True, s


# ---------------------------------------------------------------------------------------------------------------
# Algorithm RR2

def entry_ok(g, lo, hi):
    """The exact test of solvers.md Proposition 3.8 for one interval."""
    def at(x):
        r = F(0)
        for c in reversed(g):
            r = r * x + c
        return r
    if lo == hi:
        return at(lo) == 0
    return lo < hi and sgn(at(lo)) * sgn(at(hi)) < 0


def real_roots(f, prec, refine="qir", count=None):
    """Algorithm RR2. Returns (status, n, balls, stats): balls is the ascending list of (lo, hi), rationals with
    a denominator that is a power of 2; n their number. count, if given, is a function g -> the number of
    distinct real roots of the squarefree g by another method (FLINT's count in the C code); a disagreement
    gives NOT_DETERMINED."""
    stats = {"nodes": 0, "evaluations": 0, "bits": 0}
    f = ptrim(f)
    if not f:
        return DOMAIN, 0, [], stats
    need = max(prec, 2)
    g = squarefree_part(f)
    if len(g) == 1:
        return OK, 0, [], stats
    points, cells = isolate(g, stats)
    items = sorted([(value(m, k), 0, m, k) for m, k in points] + [(value(c, k), 1, c, k) for c, k in cells])
    dg = pderiv(g)
    balls, floor = [], None
    for x, kind, c, k in items:
        if kind == 0:
            ball = (x, x)
        else:
            ball = globals()["refine"](g, dg, c, k, floor, need, refine, stats)
        balls.append(ball)
        floor = x if kind == 0 else value(c + 1, k)                 # static: the item as isolated (design R4(4))
    # the final tests, on the balls that are output (solvers.md 3.10, steps 5 to 7)
    ok = all(entry_ok(g, lo, hi) for lo, hi in balls)
    ok = ok and all(balls[i][1] < balls[i + 1][0] for i in range(len(balls) - 1))
    ok = ok and all(lo == hi or (lo + hi != 0 and accuracy_of(lo, hi) >= need) for lo, hi in balls)
    if count is not None:
        ok = ok and count(g) == len(balls)
    if not ok:
        return NOT_DETERMINED, 0, [], stats
    return OK, len(balls), balls, stats


def accuracy_of(lo, hi):
    """arb_rel_accuracy_bits for exact end points lo < hi with a midpoint that is not 0, from the definition
    (flint-3.0.1:arb.rst:499 to 509), not from the formula of `accuracy`."""
    def top(x):
        e = x.numerator.bit_length() - x.denominator.bit_length()
        return e - 1 if F(2) ** e > x else e
    return -(top((hi - lo) / 2) - top(abs(lo + hi) / 2) + 1)


if __name__ == "__main__":
    import sys
    coeffs = [int(x) for x in sys.argv[2:]]
    st, n, balls, stats = real_roots(coeffs, int(sys.argv[1]))
    print(st, n, stats)
    for lo, hi in balls:
        print(lo, hi)
