"""The local backend of finite balls (work package 1.8), written from the proofs.

Ground truth: docs/proofs/policies.md section 4, Definition 16 (line 305) to Summary 26 (line 541);
docs/conventions.md 5.3 (the table of operations at one context pointer) and 4.6 (the implicit
global fallback: two different context pointers give a global result).

A block context is a tuple of pairwise coprime blocks q_1..q_k >= 2 with product K (Definition 16,
conventions 5.14). A local value (d; r_1..r_k), d >= 1, 0 <= r_i < q_i, is the set (A + K Zhat)/d
for any integer A with A = r_i mod q_i (Definition 16; Lemma 17.1). Exact values are never local.

A context is compared by identity (`is`), which models the context pointer of the C code
(conventions 4.6: "operations on two local values with different context pointers give a global
result, even if the blocks agree").

Every operation takes Local or Fball (the global form of tests/ref/adfref/fball.py) and returns
Local or Fball. It is written for clarity, not speed. The C tests do not import it; they read the
vectors under tests/ref/vectors/m1-local/.
"""
from fractions import Fraction
from math import gcd

from . import fball as F


class LocalError(Exception):
    """A status of a conversion. `status` is the name without the ADF_ prefix."""

    def __init__(self, status):
        super().__init__(status)
        self.status = status


class Ctx:
    """A block context: pairwise coprime blocks q_i >= 2, product K (Definition 16)."""

    __slots__ = ("blocks", "K")

    def __init__(self, blocks):
        blocks = tuple(int(q) for q in blocks)
        for i, q in enumerate(blocks):
            if q < 2 or q >= 1 << 64:
                raise ValueError("a block must satisfy 2 <= q < 2^64")
            for p in blocks[:i]:
                if gcd(p, q) != 1:
                    raise ValueError("blocks must be pairwise coprime")
        K = 1
        for q in blocks:
            K *= q
        self.blocks = blocks
        self.K = K

    @property
    def k(self):
        return len(self.blocks)

    def reduce(self, a):
        """The residues of the integer a modulo the blocks, each in [0, q_i)."""
        return tuple(a % q for q in self.blocks)

    def recombine(self, res):
        """The unique A in [0, K) with A = res_i mod q_i (Lemma 17.1, by CRT)."""
        A = 0
        for q, r in zip(self.blocks, res):
            M = self.K // q
            A += r * M * pow(M, -1, q)
        return A % self.K


class Local:
    """The local value (d; res) of the context ctx (Definition 16)."""

    __slots__ = ("ctx", "d", "res")

    def __init__(self, ctx, d, res):
        res = tuple(int(r) for r in res)
        if d < 1:
            raise ValueError("d must be >= 1")
        if len(res) != ctx.k or ctx.k < 1:
            raise ValueError("one residue per block, and at least one block")
        for q, r in zip(ctx.blocks, res):
            if not 0 <= r < q:
                raise ValueError("a residue must lie in [0, q_i)")
        self.ctx = ctx
        self.d = d
        self.res = res

    def __repr__(self):
        return "Local(%r; d=%d, res=%r)" % (self.ctx.blocks, self.d, self.res)


# ----------------------------------------------------------------------------- the set


def lift(x):
    """The numerator A in [0, K) of x (Lemma 17.1)."""
    return x.ctx.recombine(x.res)


def cancellation_gcd(x):
    """g = gcd(A, K, d), computed blockwise: gcd(product of gcd(r_i, q_i), d) (Lemma 18)."""
    prod = 1
    for q, r in zip(x.ctx.blocks, x.res):
        prod *= gcd(r, q)
    return gcd(prod, x.d)


def canonical_triple(x):
    """The canonical triple (A/g, K/g, d/g) of the set of x, A the lift in [0, K)
    (Proposition 24.1; Summary 26, proof of the canonical column). No reduction is needed: A/g is
    already in [0, K/g)."""
    A = lift(x)
    g = cancellation_gcd(x)
    return A // g, x.ctx.K // g, x.d // g


def to_global(x):
    """The global form of x (Lemma 17.3, Proposition 24.1); a global x is returned as it is."""
    if isinstance(x, F.Fball):
        return x
    A, H, d = canonical_triple(x)
    return F.Fball(A, H, d)


def derived_context_value(x):
    """The canonical triple as a local value of the derived blocks q_i/g_i, blocks reduced to 1
    dropped (Proposition 24.3): (d/g; (r_i/g_i) w_i mod (q_i/g_i)), w_i the inverse of g/g_i modulo
    q_i/g_i. Returns (blocks, d, res); used by the self-checks only."""
    g = cancellation_gcd(x)
    blocks, res = [], []
    for q, r in zip(x.ctx.blocks, x.res):
        gi = gcd(g, q)
        qi = q // gi
        if qi == 1:
            continue
        w = pow((g // gi) % qi, -1, qi)
        blocks.append(qi)
        res.append((r // gi) * w % qi)
    return tuple(blocks), x.d // g, tuple(res)


# ----------------------------------------------------------------------------- conversions


def set_local(ball, ctx):
    """Exact conversion into ctx (Proposition 19): c + R Zhat, R > 0, lives in ctx exactly when
    K/R and c K/R are integers; then d = K/R and r_i = (c K/R) mod q_i. Raises LocalError:
    UNSUPPORTED if ctx has no block (conventions 5.14), DOMAIN if the set is not a local value of
    ctx (an exact value included, Definition 16). UNSUPPORTED is checked first; it is also the
    larger status (conventions 3.3)."""
    if ctx.k == 0:
        raise LocalError("UNSUPPORTED")
    ball = to_global(ball)
    if ball.H == 0:
        raise LocalError("DOMAIN")
    R = ball.radius
    KR = Fraction(ctx.K) / R
    cKR = ball.center * KR
    if KR.denominator != 1 or cKR.denominator != 1:
        raise LocalError("DOMAIN")
    return Local(ctx, int(KR), ctx.reduce(int(cKR)))


def set_local_enclose(ball, ctx):
    """Best enclosure in ctx (Proposition 20): K/R = n/m in lowest terms, e the denominator of a
    rational point c, d0 = lcm(n, e), result (c d0 + K Zhat)/d0; lost = (d0 != K/R). Returns
    (Local, lost). Raises LocalError UNSUPPORTED (no block) or DOMAIN (exact input)."""
    if ctx.k == 0:
        raise LocalError("UNSUPPORTED")
    ball = to_global(ball)
    if ball.H == 0:
        raise LocalError("DOMAIN")
    R = ball.radius
    KR = Fraction(ctx.K) / R
    n = KR.numerator
    c = ball.center
    e = c.denominator
    d0 = n * e // gcd(n, e)
    A0 = c * d0
    assert A0.denominator == 1
    lost = Fraction(d0) != KR
    return Local(ctx, d0, ctx.reduce(int(A0))), lost


# ----------------------------------------------------------------------------- operations


def _same_context(x, y):
    return isinstance(x, Local) and isinstance(y, Local) and x.ctx is y.ctx


def neg(x):
    """-x. Local: (d; (-r_i) mod q_i), exact (Proposition 21.1). Global: fball.neg."""
    if isinstance(x, Local):
        return Local(x.ctx, x.d, tuple((-r) % q for q, r in zip(x.ctx.blocks, x.res)))
    return F.neg(x)


def add(x, y):
    """x + y. Both local at one context: (L; (r_i L/d + s_i L/e) mod q_i), L = lcm(d, e), exact and
    tight (Proposition 21.2). Otherwise the tight global sum of the global forms (conventions 4.6,
    5.3: the implicit global fallback)."""
    if _same_context(x, y):
        L = x.d * y.d // gcd(x.d, y.d)
        u, v = L // x.d, L // y.d
        return Local(x.ctx, L, tuple((r * u + s * v) % q
                                     for q, r, s in zip(x.ctx.blocks, x.res, y.res)))
    return F.add(to_global(x), to_global(y))


def sub(x, y):
    """x - y = x + (-y) (policies Theorem 3 step 2; Proposition 21)."""
    return add(x, neg(y))


def product_h(x, y):
    """h = product of h_i, h_i = gcd(r_i, s_i, q_i) (Proposition 22; h = gcd(A, B, K), 22.1)."""
    h = 1
    for q, r, s in zip(x.ctx.blocks, x.res, y.res):
        h *= gcd(gcd(r, s), q)
    return h


def mul(x, y):
    """x y, tight. Both local at one context (Proposition 22, conventions 5.3 row "product"):
    h = 1: the blockwise product (d e; r_i s_i mod q_i) is tight and local;
    h > 1 and h | d e: local (d e/h; ((r_i s_i mod q_i h_i)/h_i) w'_i mod q_i), w'_i the inverse
    of h/h_i modulo q_i (22.3);
    otherwise: the tight product (A B + K h Zhat)/(d e) (22.1), global.
    Mixed backends or different contexts: the tight global product."""
    if not _same_context(x, y):
        return F.mul(to_global(x), to_global(y))
    ctx = x.ctx
    h = product_h(x, y)
    de = x.d * y.d
    if h == 1:
        return Local(ctx, de, tuple(r * s % q for q, r, s in zip(ctx.blocks, x.res, y.res)))
    if de % h == 0:
        res = []
        for q, r, s in zip(ctx.blocks, x.res, y.res):
            hi = gcd(gcd(r, s), q)
            t = (r * s) % (q * hi)
            assert t % hi == 0
            w = pow((h // hi) % q, -1, q) if q > 1 else 0
            res.append((t // hi) * w % q)
        return Local(ctx, de // h, tuple(res))
    A, B = lift(x), lift(y)
    return F.Fball(A * B, ctx.K * h, de)


def scale(x, q):
    """q x for an exact rational q (Proposition 23; conventions 5.3 row "exact scalar"): q = 0 gives
    the exact 0 (global); q = m/n with |m| dividing d gives the local (n d/|m|; sign(m) r_i mod q_i);
    otherwise the tight global product q x (precision.md Proposition 6(2))."""
    q = Fraction(q)
    if q == 0:
        return F.Fball(0, 0, 1)
    if isinstance(x, Local):
        m, n = q.numerator, q.denominator
        if x.d % abs(m) == 0:
            sgn = 1 if m > 0 else -1
            return Local(x.ctx, n * x.d // abs(m),
                         tuple((sgn * r) % b for b, r in zip(x.ctx.blocks, x.res)))
        return F.scale(to_global(x), q)
    return F.scale(x, q)


def div_rat(x, q):
    """x / q = (1/q) x for q != 0 (fball.h adf_fball_div_rat); q = 0 raises NOT_UNIT."""
    q = Fraction(q)
    if q == 0:
        raise LocalError("NOT_UNIT")
    return scale(x, 1 / q)


# ----------------------------------------------------------------------------- predicates


def equal_set(x, y):
    """Through the canonical triples (Proposition 24.4; conventions 5.3)."""
    return F.equal_set(to_global(x), to_global(y))


def overlaps(x, y):
    return F.overlaps(to_global(x), to_global(y))


def contains(x, y):
    return F.contains(to_global(x), to_global(y))


def contains_rat(x, r):
    return F.contains_rational(to_global(x), r)


def compare(x, y):
    return F.compare(to_global(x), to_global(y))
