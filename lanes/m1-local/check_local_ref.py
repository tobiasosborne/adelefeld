#!/usr/bin/env python3
"""Self-checks of tests/ref/adfref/local_ref.py against tests/ref/adfref/fball.py and against
brute force over small contexts. Run from the repository root:

    python3 lanes/m1-local/check_local_ref.py

Each check prints its count of cases; any failure raises AssertionError. The checks are those
the proofs name (docs/proofs/policies.md section 4): check_local_repr (L17, L18), check_local_ops
(P19 to P23), check_backend_repairs (P24, P25, S26), done here independently of the proofs'
own scripts.
"""
import os
import random
import sys
from fractions import Fraction
from itertools import product
from math import gcd

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "tests", "ref"))
from adfref import fball as F  # noqa: E402
from adfref import local_ref as L  # noqa: E402
from adfref.membership import rational_in_fball  # noqa: E402


def in_ball(x, A, H, d):
    return rational_in_fball(x, F.Fball(A, H, d))

SMALL = [(2,), (4,), (6,), (9,), (12,), (2, 3), (4, 3), (4, 9), (8, 3, 5), (4, 9, 25), (7,), (16,)]


def all_locals(ctx, dmax):
    for d in range(1, dmax + 1):
        for res in product(*[range(q) for q in ctx.blocks]):
            yield L.Local(ctx, d, res)


def elements(ball, count=7):
    """Some rational points of the ball: a + N k, k in a window (precision.md Lemma 1)."""
    if ball.H == 0:
        return [ball.center]
    return [ball.center + ball.radius * k for k in range(-count // 2, count - count // 2)]


def check_repr():
    n = 0
    for blocks in SMALL:
        ctx = L.Ctx(blocks)
        for x in all_locals(ctx, 12):
            A = L.lift(x)
            assert 0 <= A < ctx.K
            assert all(A % q == r for q, r in zip(ctx.blocks, x.res))
            # Lemma 18: blockwise gcd
            assert L.cancellation_gcd(x) == gcd(gcd(A, ctx.K), x.d)
            # P24.1: canonical triple is fball.canon of the raw triple
            assert L.canonical_triple(x) == F.canon(A, ctx.K, x.d)
            # the set: every lift A + K t gives the same canonical triple (Lemma 17.1)
            assert F.canon(A + 5 * ctx.K, ctx.K, x.d) == F.canon(A, ctx.K, x.d)
            # P24.3: derived-context value is the canonical triple
            blocks2, d2, res2 = L.derived_context_value(x)
            Ac, Hc, dc = L.canonical_triple(x)
            K2 = 1
            for q in blocks2:
                K2 *= q
            assert K2 == Hc and d2 == dc
            assert all(Ac % q == r for q, r in zip(blocks2, res2))
            # Lemma 17.2 and P19: the exact conversion of the set gives x back
            y = L.set_local(L.to_global(x), ctx)
            assert y.d == x.d and y.res == x.res
            n += 1
    print("check_repr: %d local values" % n)


def check_set_local_and_enclose():
    rng = random.Random(1)
    n = 0
    for blocks in SMALL:
        ctx = L.Ctx(blocks)
        locs = list(all_locals(ctx, 8))
        for _ in range(150):
            H = rng.randint(1, 40)
            d = rng.randint(1, 40)
            A = rng.randint(-60, 60)
            b = F.Fball(A, H, d)
            # brute force: the local values (d' <= 8) that contain b, and whether one equals b
            cont = [x for x in locs if F.contains(b, L.to_global(x))]
            equal = [x for x in cont if F.equal_set(b, L.to_global(x))]
            try:
                y = L.set_local(b, ctx)
                assert L.to_global(y) == b
                if y.d <= 8:
                    assert len(equal) == 1 and equal[0].d == y.d and equal[0].res == y.res
            except L.LocalError as e:
                assert e.status == "DOMAIN"
                assert not equal
            y, lost = L.set_local_enclose(b, ctx)
            gy = L.to_global(y)
            assert F.contains(b, gy)
            assert lost == (not F.equal_set(b, gy))
            for x in cont:
                assert F.contains(gy, L.to_global(x)), (b, x, y)
            n += 1
        # exact values are never local
        for v in (Fraction(0), Fraction(3, 2)):
            b = F.Fball(v.numerator, 0, v.denominator)
            for fn in (L.set_local, L.set_local_enclose):
                try:
                    fn(b, ctx)
                    raise AssertionError("exact value converted")
                except L.LocalError as e:
                    assert e.status == "DOMAIN"
    print("check_set_local_and_enclose: %d balls" % n)


def check_ops():
    rng = random.Random(2)
    n = 0
    local_mul = global_mul = hbig = 0
    for blocks in SMALL + [(2**61 - 1, 4, 9), (2**64 - 59,)]:
        ctx = L.Ctx(blocks)
        other = L.Ctx(blocks)
        for _ in range(300):
            def rnd():
                d = rng.choice([1, 2, 3, 4, 6, 8, 9, 12, 5, 10, rng.randint(1, 50)])
                res = []
                for q in ctx.blocks:
                    if rng.random() < 0.5 and q < 1000:
                        divs = [t for t in range(1, q + 1) if q % t == 0]
                        t = rng.choice(divs)
                        res.append((t * rng.randint(0, q)) % q)
                    else:
                        res.append(rng.randrange(q))
                return L.Local(ctx, d, res)
            x, y = rnd(), rnd()
            gx, gy = L.to_global(x), L.to_global(y)
            yo = L.Local(other, y.d, y.res)
            # negation and sum: local, and equal to the global result
            r = L.neg(x)
            assert isinstance(r, L.Local) and L.to_global(r) == F.neg(gx)
            for a, b in ((x, y), (x, x)):
                r = L.add(a, b)
                assert isinstance(r, L.Local) and r.d == a.d * b.d // gcd(a.d, b.d)
                assert L.to_global(r) == F.add(L.to_global(a), L.to_global(b))
                r = L.sub(a, b)
                assert isinstance(r, L.Local)
                assert L.to_global(r) == F.sub(L.to_global(a), L.to_global(b))
            # different context objects with equal blocks: global
            assert isinstance(L.add(x, yo), F.Fball) and L.add(x, yo) == F.add(gx, gy)
            assert isinstance(L.mul(x, yo), F.Fball) and L.mul(x, yo) == F.mul(gx, gy)
            # mixed backends: global
            assert isinstance(L.add(x, gy), F.Fball) and L.add(x, gy) == F.add(gx, gy)
            # product
            r = L.mul(x, y)
            t = F.mul(gx, gy)
            assert L.to_global(r) == t
            h = L.product_h(x, y)
            assert h == gcd(gcd(L.lift(x), L.lift(y)), ctx.K)          # P22.1
            try:
                L.set_local(t, ctx)
                representable = True
            except L.LocalError:
                representable = False
            assert representable == ((x.d * y.d) % h == 0)            # P22.3 against P19
            assert isinstance(r, L.Local) == representable
            if h > 1:
                hbig += 1
                # P22.2: the blockwise product is an enclosure that loses the factor h
                bw = L.Local(ctx, x.d * y.d, tuple(a * b % q for q, a, b in zip(ctx.blocks, x.res, y.res)))
                assert F.contains(t, L.to_global(bw))
                assert not F.equal_set(t, L.to_global(bw))
            if isinstance(r, L.Local):
                local_mul += 1
            else:
                global_mul += 1
            # enclosure of the product by sampling elements
            for ea in elements(gx, 3):
                for eb in elements(gy, 3):
                    assert in_ball(ea * eb, t.A, t.H, t.d)
            # scalar
            for qv in (Fraction(0), Fraction(1), Fraction(-1), Fraction(2), Fraction(-3, 2),
                       Fraction(1, 7), Fraction(rng.randint(-12, 12), rng.randint(1, 9))):
                r = L.scale(x, qv)
                assert L.to_global(r) == F.scale(gx, qv)
                if qv == 0:
                    assert isinstance(r, F.Fball)
                else:
                    assert isinstance(r, L.Local) == (x.d % abs(qv.numerator) == 0)
                if qv != 0:
                    r = L.div_rat(x, qv)
                    assert L.to_global(r) == F.scale(gx, 1 / qv)
            n += 1
    print("check_ops: %d pairs; products local %d, global %d; h > 1 in %d" % (n, local_mul, global_mul, hbig))


def check_examples():
    # P25.3 / brief: A = d = 2, block 4: the set is 1 + 2 Zhat; its lifts 2 and 6 have centres 1 and 3
    c4 = L.Ctx((4,))
    x = L.Local(c4, 2, (2,))
    assert L.to_global(x) == F.Fball(1, 2, 1)
    assert in_ball(Fraction(1), 1, 2, 1) and in_ball(Fraction(3), 1, 2, 1)
    # P24 example: (2; 2) in (4): canonical (1, 2, 1), derived context (2) as (1; 1)
    assert L.canonical_triple(x) == (1, 2, 1)
    assert L.derived_context_value(x) == ((2,), 1, (1,))
    assert L.set_local(F.Fball(1, 2, 1), c4).res == (2,)
    # S26: (1 + 2 Zhat)/2 + (1 + 2 Zhat)/2: raw (2 + 2 Zhat)/2, canonical (0, 1, 1)
    c2 = L.Ctx((2,))
    y = L.Local(c2, 2, (1,))
    s = L.add(y, y)
    assert s.d == 2 and s.res == (0,) and L.canonical_triple(s) == (0, 1, 1)
    # S26: tight product of (1 + 6 Zhat)/2 and (2 + 6 Zhat)/3: h = 1, raw (2 + 6 Zhat)/6, canonical (1, 3, 3)
    c6 = L.Ctx((6,))
    p = L.mul(L.Local(c6, 2, (1,)), L.Local(c6, 3, (2,)))
    assert isinstance(p, L.Local) and p.d == 6 and p.res == (2,) and L.canonical_triple(p) == (1, 3, 3)
    # P22 example: (4), x = y = 2 + 4 Zhat: h = 2, tight 4 + 8 Zhat, global
    z = L.Local(c4, 1, (2,))
    p = L.mul(z, z)
    assert isinstance(p, F.Fball) and p == F.Fball(4, 8, 1)
    print("check_examples: 5 examples of the proofs")


def check_mutants():
    """Mutants of local_ref that the checks above must kill."""
    killed = 0
    orig_add, orig_mul, orig_scale = L.add, L.mul, L.scale

    def add_bad(x, y):   # uses d e instead of lcm(d, e)
        if L._same_context(x, y):
            Lm = x.d * y.d
            u, v = Lm // x.d, Lm // y.d
            return L.Local(x.ctx, Lm, tuple((r * u + s * v) % q for q, r, s in zip(x.ctx.blocks, x.res, y.res)))
        return orig_add(x, y)

    def mul_bad(x, y):   # blockwise product always (the enclosure of P22.2)
        if L._same_context(x, y):
            return L.Local(x.ctx, x.d * y.d, tuple(r * s % q for q, r, s in zip(x.ctx.blocks, x.res, y.res)))
        return orig_mul(x, y)

    def scale_bad(x, q):  # forgets the sign of m
        q = Fraction(q)
        if q != 0 and isinstance(x, L.Local) and x.d % abs(q.numerator) == 0:
            return L.Local(x.ctx, q.denominator * x.d // abs(q.numerator), x.res)
        return orig_scale(x, q)

    for name, bad in (("add", add_bad), ("mul", mul_bad), ("scale", scale_bad)):
        setattr(L, name, bad)
        try:
            check_ops()
        except AssertionError:
            killed += 1
        finally:
            L.add, L.mul, L.scale = orig_add, orig_mul, orig_scale
    print("check_mutants: %d of 3 mutants killed" % killed)
    assert killed == 3


if __name__ == "__main__":
    check_repr()
    check_set_local_and_enclose()
    check_ops()
    check_examples()
    check_mutants()
    print("all checks passed")
