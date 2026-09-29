"""proto/lfunc_checks.py: the reference and the oracles of lane f-slice4 (milestone 1F.4): the power series exp and
log at a prime and the Iwasawa logarithm Log, on exact rationals and on local balls (include/adelefeld/lfunc.h,
docs/api-1f4.md).

Exact arithmetic only: int and fractions.Fraction. No floating point, no FLINT.

The reference is written to be independent of the C code (src/lfunc.c) in the places where the C code does
something of its own:
  - values at a point are exact rational partial sums of the series (Definition 1 of docs/proofs/functions.md,
    lines 20-27), reduced modulo p^n at the end; the number of terms is decided by a term bound with a margin of
    three digits (MARGIN), so the reference sums MORE terms than the counts of Propositions 7 and 7b, and a count
    that is too short in C shows as a different residue;
  - log at a point is the raw series in x - 1, also at p = 2 for x = 3 modulo 4 (where the C code uses -x);
  - Log at odd p goes through the Teichmueller representative w, computed as t^(p^(M-1)) modulo p^M (statement T
    below), and not through the power a^(p-1) that the C code uses (docs/api-1f4.md F3); at p = 2 it is
    log(a^2)/2, not log(+-a).
  - the result of a ball is decided by the formulas of Propositions 10 and 11 (the exponent of the image) and the
    value at the centre; the formulas are checked here by enumeration of the points of the ball
    (check_ball_enumeration).

Statement T (Teichmueller by powering; own proof). Let p be odd, t an integer prime to p, w the root of unity
with w = t modulo p (functions.md Proposition 4, step 1), k >= 0. Then t^(p^k) = w modulo p^(k+1).
Proof. t/w is in 1 + p Z_p, t = w (1 + p y). (i) w^(p-1) = 1 and p^k = 1 modulo p - 1, so w^(p^k) = w.
(ii) If s = 1 + p^j s' with j >= 1, then s^p = sum_i binom(p, i) p^(ij) s'^i; for 1 <= i <= p - 1 the term is
divisible by p^(1 + ij), hence by p^(j+1); the term i = p is divisible by p^(pj), and pj >= j + 1. So s^p is in
1 + p^(j+1) Z_p, and by induction (1 + p y)^(p^k) is in 1 + p^(k+1) Z_p. With (i): t^(p^k) = w (1 + p^(k+1) Z_p).

Run: python3 -B proto/lfunc_checks.py (the self-checks; each prints its count and fails with an assertion).
"""

from fractions import Fraction
import random
import sys

MARGIN = 3          # extra digits of the term bound of the reference beyond the requested precision


# ------------------------------------------------------------------------------------------------ basic p-adic

def c_of(p):
    """c = 1 for odd p, 2 for p = 2 (functions.md Definition 1, line 16)."""
    return 2 if p == 2 else 1


def vp_int(a, p):
    """v_p of a nonzero integer."""
    assert a != 0
    k = 0
    while a % p == 0:
        a //= p
        k += 1
    return k


def val(q, p):
    """v_p of a rational; None for 0 (infinity)."""
    q = Fraction(q)
    if q == 0:
        return None
    return vp_int(q.numerator, p) - vp_int(q.denominator, p)


def residue(q, p, n):
    """The integer r in [0, p^n) with v_p(q - r) >= n, for a p-integral rational q and n >= 1; 0 for n <= 0."""
    q = Fraction(q)
    if n <= 0:
        return 0
    P = p ** n
    assert q.denominator % p != 0, "not p-integral"
    return q.numerator * pow(q.denominator, -1, P) % P


def floor_log(k, p):
    """e(k): the largest e with p^e <= k, k >= 1 (functions.md Proposition 7b)."""
    e, t = 0, p
    while t <= k:
        e += 1
        t *= p
    return e


# ------------------------------------------------------------------------------------------------ values at points

def exp_point(p, x, n):
    """exp(x) modulo p^n for a rational x with x = 0 or v(x) >= c; an integer in [0, p^n).

    Tail bound: the term of degree k has valuation k w - v_p(k!) >= k w - (k-1)/(p-1), w = v(x) (Lemma 5, line 121).
    b(k) = k w - (k-1)/(p-1) is increasing (w >= c > 1/(p-1)). The sum is taken over degrees 0 .. k0 - 1, k0 the
    first k >= 1 with b(k) >= n + MARGIN; every omitted term then has valuation >= n + MARGIN, and the tail lies in
    p^(n + MARGIN) Z_p (Lemma 3, item 1)."""
    x = Fraction(x)
    if n <= 0:
        return 0
    if x == 0:
        return residue(1, p, n)
    w = val(x, p)
    assert w >= c_of(p)
    target = n + MARGIN
    s = Fraction(0)
    term = Fraction(1)
    k = 0
    while True:
        if k >= 1 and k * w * (p - 1) - (k - 1) >= target * (p - 1):
            break
        s += term
        k += 1
        term = term * x / k
    return residue(s, p, n)


def log1p_point(p, z, n):
    """log(1 + z) modulo p^n for a rational z with z = 0 or v(z) >= 1 (every p, also v(z) = 1 at p = 2).

    Tail bound: the term of degree k has valuation k v - v_p(k) >= k v - e(k), which is nondecreasing in k and tends
    to infinity (Proposition 7b, step 1). Summed over degrees 1 .. k0 - 1, k0 the first k with
    k v - e(k) >= n + MARGIN."""
    z = Fraction(z)
    if n <= 0:
        return 0
    if z == 0:
        return 0
    v = val(z, p)
    assert v >= 1
    target = n + MARGIN
    s = Fraction(0)
    zk = Fraction(1)
    k = 1
    while True:
        if k * v - floor_log(k, p) >= target:
            break
        zk *= z
        s += zk / k if k % 2 == 1 else -zk / k
        k += 1
    return residue(s, p, n)


def log_point(p, x, n):
    """The log series at x in 1 + p Z_p, modulo p^n: the raw series in z = x - 1 (Definition 1, line 27)."""
    x = Fraction(x)
    assert x != 0 and x != 1 and val(x - 1, p) >= 1 or x == 1
    return log1p_point(p, x - 1, n)


def teichmuller(t, p, M):
    """w modulo p^M for an integer t prime to p, odd p: t^(p^(M-1)) modulo p^M (statement T)."""
    assert p != 2 and t % p != 0 and M >= 1
    return pow(t, p ** (M - 1), p ** M)


def Log_point(p, x, n):
    """Log(x) = log(u) for x = p^m w u (Proposition 4, Proposition 11), modulo p^n, for a rational x != 0.

    Odd p: a = x / p^m, t = a modulo p^M, M = n + 1, w by statement T, u_rep = t / w modulo p^M, and
    log(u_rep) by the series; u_rep = u modulo p^M and log is an isometry on 1 + p Z_p (Proposition 11), so
    log(u_rep) = log(u) modulo p^M, M >= n.
    p = 2: a = x / 2^m is odd; a^2 = u^2 (w = +-1), and log(a^2) = 2 log(u) (Lemma 9), so Log(x) = log(a^2)/2; the
    series in a^2 - 1 (valuation >= 3) is taken modulo 2^(n+1) and halved (it is even: log(u) is in 4 Z_2)."""
    x = Fraction(x)
    assert x != 0
    if n <= 0:
        return 0
    m = val(x, p)
    a = x / Fraction(p) ** m
    if p == 2:
        r = log1p_point(2, a * a - 1, n + 1)
        assert r % 2 == 0
        return (r // 2) % (2 ** n)
    M = n + 1
    P = p ** M
    t = residue(a, p, M)
    w = teichmuller(t, p, M)
    u_rep = t * pow(w, -1, P) % P
    assert u_rep % p == 1
    return log1p_point(p, Fraction(u_rep - 1), n)


# ------------------------------------------------------------------------------------------------ local balls

def lb_exact(p, q):
    """The canonical exact value q at p (conventions 5.8): dict with p, exact, un, ud, v, N."""
    q = Fraction(q)
    if q == 0:
        return {"p": p, "exact": 1, "un": 0, "ud": 1, "v": 0, "N": 0}
    w = val(q, p)
    u = q / Fraction(p) ** w
    return {"p": p, "exact": 1, "un": u.numerator, "ud": u.denominator, "v": w, "N": 0}


def lb_ball(p, q, N):
    """The canonical ball q + p^N Z_p (api-1f.md L0)."""
    q = Fraction(q)
    w = val(q, p)
    if q == 0 or w >= N:
        return {"p": p, "exact": 0, "un": 0, "ud": 1, "v": 0, "N": N}
    t = q / Fraction(p) ** w
    return {"p": p, "exact": 0, "un": residue(t, p, N - w), "ud": 1, "v": w, "N": N}


def lb_value(x):
    """The rational p^v u of an lball dict (the value of an exact x, the centre of a ball)."""
    return Fraction(x["un"], x["ud"]) * Fraction(x["p"]) ** x["v"]


def lb_contains_point(x, t):
    """1 if the rational t lies in the set x."""
    t = Fraction(t)
    c = lb_value(x)
    if x["exact"]:
        return t == c
    d = t - c
    return d == 0 or val(d, x["p"]) >= x["N"]


def domain_status(f, x):
    """The status of the domain test (conventions 3.1; Proposition 6), decided on the definition of the domain:
    'OK' inside, 'DOMAIN' disjoint, 'NOT_DETERMINED' meeting both."""
    p = x["p"]
    c = c_of(p)
    a = lb_value(x)
    if f == "exp":
        if x["exact"]:
            return "OK" if a == 0 or val(a, p) >= c else "DOMAIN"
        if a == 0:
            return "OK" if x["N"] >= c else "NOT_DETERMINED"
        return "OK" if val(a, p) >= c else "DOMAIN"
    if f == "log":
        d = None if a == 1 else val(a - 1, p)          # None: infinity
        if x["exact"]:
            return "OK" if a != 0 and (d is None or d >= 1) else "DOMAIN"
        M = x["N"]
        if M >= 1:
            return "OK" if d is None or d >= 1 else "DOMAIN"
        return "NOT_DETERMINED" if d is None or d >= M else "DOMAIN"
    if f == "Log":
        if a == 0:
            return "DOMAIN" if x["exact"] else "NOT_DETERMINED"
        return "OK"
    raise ValueError(f)


def point_value(f, p, t, n):
    if f == "exp":
        return exp_point(p, t, n)
    if f == "log":
        return log_point(p, t, n)
    return Log_point(p, t, n)


def image_exponent(f, x):
    """E of lfunc.h for a ball x inside the domain (Propositions 10 and 11)."""
    p = x["p"]
    c = c_of(p)
    M = x["N"]
    if f == "exp":
        return M
    if f == "log":
        return M if M >= c else 2
    a = lb_value(x)
    r = M - val(a, p)
    return r if r >= c else 2


def exact_value_is_rational(f, p, a):
    """The exact results of lfunc.h (F2): exp(0) = 1, log(1) = 0, log(-1) = 0 at 2, Log(+-p^m) = 0. None otherwise."""
    if f == "exp":
        return Fraction(1) if a == 0 else None
    if f == "log":
        if a == 1 or (p == 2 and a == -1):
            return Fraction(0)
        return None
    m = val(a, p)
    if abs(a / Fraction(p) ** m) == 1:
        return Fraction(0)
    return None


def result(f, x, N):
    """(status, result dict or None) of the C function for the lball dict x and the requested precision N,
    by lfunc.h: exact where F2 says so, else f(a) + p^K Z_p with K = N (exact x) or min(N, E) (ball x)."""
    st = domain_status(f, x)
    if st != "OK":
        return st, None
    p = x["p"]
    a = lb_value(x)
    if x["exact"]:
        e = exact_value_is_rational(f, p, a)
        if e is not None:
            return "OK", lb_exact(p, e)
        K = N
    else:
        K = min(N, image_exponent(f, x))
    if K <= 0:
        return "OK", lb_ball(p, 0, K)
    return "OK", lb_ball(p, point_value(f, p, a, K), K)


# ------------------------------------------------------------------------------------------------ self-checks

def balls_of(p, lo_v, hi_N, max_rel):
    """Canonical balls p^v u + p^N Z_p with lo_v <= v < N <= hi_N, N - v <= max_rel, and the balls O(p^N) around 0."""
    out = []
    for N in range(lo_v, hi_N + 1):
        out.append(lb_ball(p, 0, N))
        for v in range(lo_v, N):
            k = N - v
            if k > max_rel:
                continue
            for u in range(1, p ** k):
                if u % p:
                    out.append(lb_ball(p, Fraction(u) * Fraction(p) ** v, N))
    return out


def check_teichmuller():
    """w^(p-1) = 1 modulo p^M and w = t modulo p, for p = 3, 5, 7, 13 and all residues t."""
    count = 0
    for p in (3, 5, 7, 13):
        for M in (1, 2, 5):
            P = p ** M
            for t in range(1, p ** 2):
                if t % p == 0:
                    continue
                w = teichmuller(t, p, M)
                assert pow(w, p - 1, P) == 1 % P and (w - t) % p == 0
                count += 1
    return count


def check_log_routes():
    """The raw series log(x) and the Teichmueller route Log(x) agree on 1 + p Z_p (Proposition 11, step 3), at
    p = 2 also for x = 3 modulo 4 (log(-1) = 0), and on rationals with denominators."""
    rng = random.Random(4)
    count = 0
    for p in (2, 3, 5, 7):
        for _ in range(150):
            num = p * rng.randrange(-300, 300)
            den = rng.randrange(1, 60)
            while den % p == 0:
                den += 1
            x = 1 + Fraction(num, den)
            if x == 0:
                continue
            if val(x - 1, p) is not None and val(x - 1, p) < 1:
                continue
            n = rng.randrange(1, 12)
            assert log_point(p, x, n) == Log_point(p, x, n), (p, x, n)
            count += 1
    # the witnesses of SPEC 9.3.2 and functions.md Proposition 11, step 5
    assert log_point(2, -1, 20) == 0 and Log_point(2, -1, 20) == 0
    assert val(Fraction(log_point(2, 3, 20)), 2) == 2                     # v_2(log 3) = 2 (SPEC 9.3.2)
    assert val(Fraction(Log_point(3, 4, 10)), 3) == 1                     # v_3(Log 12 - Log 3) = v_3(log 4) = 1
    assert val(Fraction(Log_point(2, 5, 10)), 2) == 2                     # v_2(Log 10 - Log 2) = v_2(log 5) = 2
    return count + 4


def check_exp_log_inverse():
    """log(exp(x)) = x on p^c Z_p and exp(log(x)) = x on 1 + p^c Z_p (Lemma 9), modulo p^n."""
    count = 0
    for p in (2, 3, 5):
        c = c_of(p)
        n = 8
        for t in range(0, p ** 4):
            x = Fraction(t * p ** c)
            e = exp_point(p, x, n)
            assert log_point(p, e, n) == residue(x, p, n), (p, x)
            y = 1 + x
            # r = log(y) modulo p^(n+2) has v(r) >= c or r = 0; exp is an isometry (Proposition 10)
            assert exp_point(p, log_point(p, y, n + 2), n) == residue(y, p, n), (p, y)
            count += 2
    return count


def check_ball_enumeration():
    """For every ball of a small universe inside the domain and every requested N in a set, the result of
    result() contains the value of f at every point of the ball modulo p^(M + 2) (M the exponent of the ball) and,
    when K = E, the values modulo p^(K + 1) are not all equal (so no ball of exponent K + 1 contains the image). The
    point values are computed to precision K + 1 by exact sums. Returns (cases, points)."""
    cases = points = 0
    for p, lo_v, hi_N, max_rel in ((2, -1, 5, 4), (3, -1, 4, 3), (5, -1, 3, 2)):
        for f in ("exp", "log", "Log"):
            for x in balls_of(p, lo_v, hi_N, max_rel):
                if domain_status(f, x) != "OK":
                    continue
                E = image_exponent(f, x)
                for N in (E, E - 1, E + 2):
                    st, y = result(f, x, N)
                    assert st == "OK"
                    K = y["N"]
                    a = lb_value(x)
                    M = x["N"]
                    extra = 2
                    vals = []
                    for t in range(p ** extra):
                        pt = a + Fraction(t) * Fraction(p) ** M
                        if pt == 0:
                            continue
                        r = point_value(f, p, pt, max(K + 1, 1))
                        assert lb_contains_point(y, r), (f, p, x, N, pt, r, y)
                        vals.append(r % (p ** max(K + 1, 0)) if K + 1 > 0 else 0)
                        points += 1
                    if N >= E and K + 1 >= 1:
                        assert len(set(vals)) > 1, (f, p, x, N, "not the smallest ball")
                    cases += 1
    return cases, points


def check_domains():
    """The statuses of domain_status against points: an OK ball has all sampled points inside, a DOMAIN ball none,
    a NOT_DETERMINED ball both."""
    def inside(f, p, t):
        c = c_of(p)
        if f == "exp":
            return t == 0 or val(t, p) >= c
        if f == "log":
            return t == 1 or (t != 0 and val(t - 1, p) >= 1)
        return t != 0
    count = 0
    for p in (2, 3, 5):
        for f in ("exp", "log", "Log"):
            for x in balls_of(p, -2, 3, 3):
                a = lb_value(x)
                pts = [a + Fraction(t) * Fraction(p) ** x["N"] for t in range(-p ** 3, p ** 3)]
                ins = [inside(f, p, t) for t in pts]
                st = domain_status(f, x)
                if st == "OK":
                    assert all(ins)
                elif st == "DOMAIN":
                    assert not any(ins)
                else:
                    assert any(ins) and not all(ins)
                count += 1
    return count


def main():
    print("check_teichmuller:", check_teichmuller())
    print("check_log_routes:", check_log_routes())
    print("check_exp_log_inverse:", check_exp_log_inverse())
    print("check_domains:", check_domains())
    print("check_ball_enumeration (cases, points):", check_ball_enumeration())
    print("all checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
