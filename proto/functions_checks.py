"""Independent finite checks for docs/proofs/functions.md. Python 3, standard library only.

Run: python3 -B proto/functions_checks.py
All arithmetic is exact. Quotient checks are finite evidence, not proofs of infinite statements.
The cutoffs are proved in the companion file; a longer exact series is the numerical oracle.
"""

import traceback
from fractions import Fraction as F
from functools import lru_cache
from math import comb, factorial, gcd, inf
from pathlib import Path

PRIMES = (2, 3, 5, 13)
FAMILY = ("exp", "sin", "sinh", "cos", "cosh")


def vp(x, p):
    x = F(x)
    if not x:
        return inf
    a, b, answer = abs(x.numerator), x.denominator, 0
    while a % p == 0:
        a //= p
        answer += 1
    while b % p == 0:
        b //= p
        answer -= 1
    return answer


def cdisc(p):
    return 2 if p == 2 else 1


def ceildiv(a, b):
    return -((-a) // b)


def cutoffs(p, v, n):
    """Return literal term counts for all five factorial series and for log(1+z)."""
    assert p >= 2 and v >= 1
    answer = {"log": max(1, ceildiv(2*n, 2*v-1)) - 1}
    if v >= cdisc(p):
        k = max(1, ceildiv((p-1)*n-1, (p-1)*v-1))
        answer.update(exp=k, sin=k//2, sinh=k//2, cos=(k+1)//2, cosh=(k+1)//2)
    return answer


def ilog(k, p):
    """The largest e with p^e <= k; an integer logarithm, no floating point."""
    e = 0
    while p**(e+1) <= k:
        e += 1
    return e


def tight_log_count(p, v, n):
    """Tight log count of Proposition 7b: T = J-1 with J the least k >= 1 such that k v - e(k) >= n."""
    j = 1
    while j*v - ilog(j, p) < n:
        j += 1
    return j - 1


def check_truncation():
    scenarios = comparisons = tight_comparisons = 0
    for p in PRIMES:
        # Independent denominator oracle: accumulate factors, not Legendre's formula.
        den = [0]
        for k in range(1, 4097):
            den.append(den[-1] + int(vp(k, p)))
        logden = [0] + [int(vp(k, p)) for k in range(1, 4097)]
        for v in range(cdisc(p), cdisc(p) + 4):
            for n in range(-3, 41):
                counts = cutoffs(p, v, n)
                for fn in FAMILY:
                    t = counts[fn]
                    first = t if fn == "exp" else 2*t + (fn in ("sin", "sinh"))
                    step = 1 if fn == "exp" else 2
                    for k in range(first, 4097, step):
                        assert k*v - den[k] >= n, (p, v, n, fn, k)
                        comparisons += 1
                    scenarios += 1
        for v in range(1, 5):
            for n in range(-3, 41):
                first = cutoffs(p, v, n)["log"] + 1
                for k in range(first, 4097):
                    assert k*v - logden[k] >= n, (p, v, n, "log", k)
                    comparisons += 1
                scenarios += 1
                # Proposition 7b, tight count: safe by its own bound, and never longer than the safe count.
                tight = tight_log_count(p, v, n)
                assert 0 <= tight <= cutoffs(p, v, n)["log"], (p, v, n, tight)
                for k in range(tight+1, 4097):
                    assert k*v - ilog(k, p) >= n, (p, v, n, "log-tight-e(k)", k)
                    assert k*v - logden[k] >= n, (p, v, n, "log-tight", k)
                    tight_comparisons += 1
                scenarios += 1
    return (f"scenarios={scenarios} omitted_terms={comparisons+tight_comparisons} "
            f"max_degree=4096 max_n=40 tight_log_terms={tight_comparisons}")


def residue(x, p, n):
    """An integral rational modulo p^n; n is positive in the quotient checks."""
    x = F(x)
    assert n >= 1 and x.denominator % p
    q = p**n
    return x.numerator * pow(x.denominator, -1, q) % q


def term_degrees(fn, count):
    if fn == "log":
        return range(1, count+1)
    if fn == "exp":
        return range(count)
    return range(fn in ("sin", "sinh"), 2*count, 2)


def partial(fn, x, count):
    x = F(x)
    answer = F(0)
    for k in term_degrees(fn, count):
        sign = (-1)**(k-1) if fn == "log" else (-1)**(k//2) if fn in ("sin", "cos") else 1
        denominator = k if fn == "log" else factorial(k)
        answer += sign*x**k/denominator
    return answer


@lru_cache(maxsize=16384)
def series(fn, x, p, n):
    """Exact rational oracle with a deliberately longer independently chosen cutoff.

    For the factorial family, k*c-v_p(k!) >= k/2 on every allowed disc.
    For log, k-v_p(k) >= k/2. Degrees beyond 2*n+8 therefore cannot affect p^n.
    No call to cutoffs is used here.
    """
    x = F(x)
    arg = x-1 if fn == "log" else x
    assert not arg or vp(arg, p) >= (1 if fn == "log" else cdisc(p))
    count = 2*max(n, 1)+8
    return residue(partial(fn, arg, count), p, n)


def check_legendre():
    cases = 0
    for p in PRIMES:
        actual = 0
        for k in range(0, 1025):
            if k:
                actual += int(vp(k, p))
            digits, t, floor_sum, power = 0, k, 0, p
            while t:
                digits += t % p
                t //= p
            while power <= k:
                floor_sum += k // power
                power *= p
            assert actual == floor_sum == (k-digits)//(p-1)
            if k:
                assert actual*(p-1) <= k-1
                assert vp(k, p)*(p-1) <= k-1 and 2*vp(k, p) <= k
            if k <= 120:
                assert actual == vp(factorial(k), p)
            cases += 1
    return f"factorials={cases} max_k=1024 exact_factorials=484"


def check_domains():
    cases = 0
    for p in PRIMES:
        for v in range(-2, cdisc(p)+2):
            for k in range(1, 100):
                term_v = vp(F(p)**(v*k)/factorial(k), p)
                if v <= 0:
                    assert term_v <= 0
                if v >= cdisc(p):
                    assert 2*term_v >= k
                cases += 1
        for fn in FAMILY:
            assert series(fn, 0, p, 8) == (fn in ("exp", "cos", "cosh"))
        assert series("log", 1, p, 8) == 0
        for j in range(1, 7):
            k = p**j
            # The logarithm fails at v(z)=0 and at every negative valuation.
            for v in (-2, -1, 0):
                assert k*v-vp(k, p) <= 0
                cases += 1
    for j in range(1, 10):
        k = 2**j
        assert vp(F(2**k, factorial(k)), 2) == 1
        assert vp(F(2**(k+1), factorial(k+1)), 2) == 2
        cases += 2
    return f"term_cases={cases} zero_values=24 boundary_subsequence_pairs=9"


def check_lifting_and_groups():
    lifts = groups = 0
    for p in PRIMES:
        for r in range(1, p):
            for k in range(1, 5):
                q = p**k
                choices = [r+t*q for t in range(p) if pow(r+t*q, p-1, q*p) == 1]
                assert len(choices) == 1
                r = choices[0]
                lifts += 1
        for n in range(1, 9):
            fibers = {}
            for a in range(1, p):
                fibers.setdefault(pow(a, n, p), []).append(a)
            assert all(len(fiber) == gcd(n, p-1) for fiber in fibers.values())
            groups += 1
    return f"unique_lifts={lifts} finite_power_maps={groups}"


def teich(r, p, precision):
    if p == 2:
        return 1
    a = r % p
    q = p
    for _ in range(1, precision):
        a = next(a+t*q for t in range(p) if pow(a+t*q, p-1, q*p) == 1)
        q *= p
    return a


def components(a, p, precision):
    m = int(vp(a, p))
    unit = residue(F(a)/F(p)**m, p, precision)
    w = (1 if unit % 4 == 1 else -1) if p == 2 else teich(unit, p, precision)
    u = unit*pow(w, -1, p**precision) % p**precision
    return m, w, u


def iwasawa(a, p, precision):
    return series("log", components(a, p, precision)[2], p, precision)


def check_decomposition():
    cases = 0
    for p in PRIMES:
        precision = 4 if p == 2 else 3
        q = p**precision
        roots = [w for w in range(1, q) if pow(w, p-1, q) == 1] if p != 2 else [1, q-1]
        for unit in range(1, q):
            if unit % p == 0:
                continue
            choices = [(w, unit*pow(w, -1, q) % q) for w in roots
                       if (unit*pow(w, -1, q)-1) % p**cdisc(p) == 0]
            assert len(choices) == 1
            for m in (-2, 0, 2):
                mm, w, u = components(F(p)**m*unit, p, precision)
                assert mm == m and w*u % q == unit and (u-1) % p**cdisc(p) == 0
                cases += 1
        if p == 2:
            assert components(-1, 2, 4)[1] == -1 and teich(3, 2, 4) == 1
    return f"decompositions={cases} valuations=-2,0,2 two_adic_sign_examples=2"


def modular_partial(fn, point, degrees, denoms, p, w):
    """Partial sum by the literal algorithm of Proposition 8: powers modulo p^W, exact division by p^e.

    Returns None when a division by the p-part of a denominator is not exact or the exponent W is
    invalid. Exactness is the precondition of fmpz_divexact and is proved in repair R-1.
    """
    if w < 1:
        return None
    total = F(0)
    for k, den in zip(degrees, denoms):
        sign = (-1)**(k-1) if fn == "log" else (-1)**(k//2) if fn in ("sin", "cos") else 1
        num = 1 if k == 0 else pow(point, k, p**w)
        e = int(vp(den, p))
        if num % p**e:
            return None
        total += sign*F(num, den)
    return total


def check_working_precision():
    cases = exact_divisions = 0
    wrong_minus1 = wrong_noD = 0
    for p in PRIMES:
        for fn in FAMILY + ("log",):
            for v in range(1 if fn == "log" else cdisc(p), cdisc(p)+3):
                for n in (-2, 0, 1, 7, 20, 40):
                    count = cutoffs(p, v, n)[fn]
                    degrees = list(term_degrees(fn, count))
                    denoms = [k if fn == "log" else factorial(k) for k in degrees]
                    d = max([0] + [int(vp(den, p)) for den in denoms])
                    w = max(v, n+d)
                    for x in (F(-p**v, p+1), F(p**v), F(3*p**v, 2*p+1)):
                        for t in (0, 1, p+1):
                            # The rule keeps W >= v, so every representative is in the certified domain
                            # p^v Z_p. Repair R-1 uses v(y) >= v to make the division by p^e exact.
                            assert vp(x+F(p)**w*t, p) >= v, (p, fn, v, n, "W >= v", w)
                        # A representative modulo p^W and two further allowed lifts.
                        y = residue(x, p, w)
                        for point in (y, y+p**w, y+(p+1)*p**w):
                            assert vp(point-x, p) >= w, (p, fn, v, n, "lift")
                            assert vp(partial(fn, x, count)-partial(fn, point, count), p) >= n
                            modular = F(0)
                            for k, den in zip(degrees, denoms):
                                sign = ((-1)**(k-1) if fn == "log"
                                        else (-1)**(k//2) if fn in ("sin", "cos") else 1)
                                num = 1 if k == 0 else pow(point, k, p**w)
                                e = int(vp(den, p))
                                assert num % p**e == 0, (p, fn, v, n, k, "exact division")
                                exact_divisions += 1
                                modular += sign*F(num, den)
                            truth = partial(fn, x, count)
                            assert vp(modular-truth, p) >= n
                            cases += 1
                            if degrees:
                                got = modular_partial(fn, point, degrees, denoms, p, max(v, n+d-1))
                                wrong_minus1 += (got is None) or vp(got-truth, p) < n
                                got = modular_partial(fn, point, degrees, denoms, p, max(v, n))
                                wrong_noD += (got is None) or vp(got-truth, p) < n
                    if n > 0:
                        arg = x+1 if fn == "log" else x
                        assert residue(partial(fn, x, count), p, n) == series(fn, arg, p, n)
    assert wrong_minus1 > 0 and wrong_noD > 0, "the W-1 and W-without-D probes found no counterexample"
    return (f"rounded_partial_sums={cases} exact_divisions={exact_divisions} inputs=3 lifts=3 "
            f"target_exponents=-2,0,1,7,20,40 W-1_wrong={wrong_minus1} W_without_D_wrong={wrong_noD}")


def check_series_identities():
    cases = 0
    for p in PRIMES:
        n = 8
        q = p**n
        c = cdisc(p)
        for j in (-2, -1, 0, 1, 2):
            x = j*p**c
            ex = series("exp", x, p, n)
            assert series("log", ex, p, n) == x % q
            u = 1+x
            assert series("exp", series("log", u, p, n), p, n) == u % q
            # One extra digit before division by 2 at p=2.
            guard = n+(p == 2)
            ep, em = series("exp", x, p, guard), series("exp", -x, p, guard)
            assert series("sinh", x, p, n) == residue(F(ep-em, 2), p, n)
            assert series("cosh", x, p, n) == residue(F(ep+em, 2), p, n)
            for y in (p**c, 2*p**c):
                assert series("exp", x+y, p, n) == ex*series("exp", y, p, n) % q
                assert series("log", u*(1+y), p, n) == (series("log", u, p, n)
                                                        + series("log", 1+y, p, n)) % q
                cases += 2
            if p in (5, 13):
                i = next(r for r in range(1, p) if r*r % p == p-1)
                modulus = p
                for _ in range(1, n):
                    i = next(i+t*modulus for t in range(p) if ((i+t*modulus)**2+1) % (modulus*p) == 0)
                    modulus *= p
                ei, emi = series("exp", i*x, p, n), series("exp", -i*x, p, n)
                assert series("sin", x, p, n) == (ei-emi)*pow(2*i, -1, q) % q
                assert series("cos", x, p, n) == (ei+emi)*pow(2, -1, q) % q
                cases += 2
            cases += 4
    assert series("cos", 4, 2, 4) == 9
    assert series("sin", 3, 3, 2) == 3
    return f"certified_series_comparisons={cases+2} precision=8 golden_residues=2"


def check_series_radii():
    pairs = hulls = 0
    for p in PRIMES:
        c = cdisc(p)
        n = 2*c+5
        for fn in FAMILY:
            for x in (-p**c, 0, p**c, 2*p**c):
                for h in (p**c, p**(c+1), -p**(c+2)):
                    diff = (series(fn, x+h, p, n)-series(fn, x, p, n)) % p**n
                    actual = min(vp(diff, p), n)
                    expected = vp(h, p)
                    assert actual == expected if fn in ("exp", "sin", "sinh") else actual >= expected
                    pairs += 1
            if fn in ("cos", "cosh"):
                for exponent in (c, c+1):
                    output_n = 2*exponent-(p == 2)
                    vals = [min(vp(series(fn, j*p**exponent, p, output_n+2)-1, p), output_n+2)
                            for j in range(p*p)]
                    assert min(vals) == output_n
                    hulls += 1
    # Mandatory wrapper counterexample, computed without the wrapper.
    assert vp(series("exp", 12, 3, 8)-series("exp", 3, 3, 8), 3) == 2
    return f"distance_pairs={pairs} cosine_hulls={hulls} exp_3_12_precision=8 difference_valuation=2"


def check_log_radii():
    images = pairs = 0
    for p in PRIMES:
        c = cdisc(p)
        k = c+2 if p != 13 else 2
        q = p**k
        units = range(1, q, p)
        logs = {a: series("log", a, p, k) for a in units}
        for a in units:
            for b in units:
                if a == b:
                    continue
                va = min(vp(logs[a]-logs[b], p), k)
                vb = vp(a-b, p)
                assert va == vb if vb >= c else va >= vb
                pairs += 1
        for r in range(1, k+1):
            actual = {logs[a] for a in range(1, q, p**r)}
            expected = set(range(0, q, p**max(c, r)))
            assert actual == expected
            images += 1
        # Enumerate full valuation-shell balls, then scale them by p^m.
        for m in (-2, 0, 2):
            for aunit in (1, 2 if p != 2 else 3):
                for r in (1, c+1):
                    a = F(p)**m*aunit
                    k2 = r+2
                    values = {iwasawa(a+F(p)**(m+r)*j, p, k2) for j in range(p**2)}
                    center = iwasawa(a, p, k2)
                    out_r = 2 if p == 2 and r == 1 else r
                    expected = {(center+j*p**out_r) % p**k2 for j in range(p**(k2-out_r))}
                    assert values == expected, (p, m, aunit, r)
                    images += 1
    assert series("log", -1, 2, 16) == 0
    assert vp(series("log", 3, 2, 16), 2) == 2
    assert vp(iwasawa(12, 3, 8)-iwasawa(3, 3, 8), 3) == 1
    assert vp(iwasawa(10, 2, 8)-iwasawa(2, 2, 8), 2) == 2
    return f"distance_pairs={pairs} exact_quotient_images={images} special_cases=4"


def small_primes(limit):
    return [a for a in range(2, limit+1) if all(a % d for d in range(2, int(a**0.5)+1))]


def check_global():
    rational_points = balls = logs = 0
    primes = small_primes(101)
    for numerator in range(-12, 13):
        for denominator in range(1, 13):
            a = F(numerator, denominator)
            if a:
                assert any(p != 2 and vp(a, p) == 0 for p in primes)
            else:
                assert all(vp(a, p) >= cdisc(p) for p in primes)
            rational_points += 1
            for radius in (F(1, 6), F(4), F(25, 14)):
                p = next(p for p in primes if p != 2 and vp(a, p) >= 0 and vp(radius, p) == 0)
                h = (1-a)/radius
                assert vp(h, p) >= 0 and a+radius*h == 1 and vp(1, p) < cdisc(p)
                balls += 1
    for p in PRIMES:
        x = p**cdisc(p)
        for fn in FAMILY:
            assert vp(series(fn, x, p, 8), p) >= 0
        for a in (F(-p**2), F(1, p), F(p+1), F(2*p+1)):
            value = iwasawa(a, p, 8)
            assert vp(value, p) >= cdisc(p)
            assert vp(F(value, 4), p) >= 0
            logs += 1
    return f"rational_point_witnesses={rational_points} excluded_ball_witnesses={balls} Log_coordinates={logs}"


def root_unit_criterion(a, p, n, precision):
    _, w, u = components(a, p, precision)
    torsion = [1, -1] if p == 2 else [teich(r, p, precision) for r in range(1, p)]
    torsion_ok = any((pow(t, n, p**precision)-w) % p**precision == 0 for t in torsion)
    return torsion_ok and vp(series("log", u, p, precision), p) >= cdisc(p)+vp(n, p)


def check_root_criteria():
    cases = fibers_checked = 0
    for p in PRIMES:
        for n in (1, 2, 3, 4, 5, 6):
            c = cdisc(p)
            e = int(vp(n, p))
            precision = c+e+2
            q = p**precision
            fibers = {}
            for b in range(1, q):
                if b % p:
                    fibers.setdefault(pow(b, n, q), []).append(b)
            for a in range(1, q):
                if a % p == 0:
                    continue
                assert (a in fibers) == root_unit_criterion(a, p, n, precision), (p, n, a)
                if a in fibers:
                    branches = {b % p**c for b in fibers[a]}
                    expected = gcd(n, 2 if p == 2 else p-1)
                    assert len(branches) == expected
                    # Finite quotients have p^e extra representatives per true root branch.
                    assert len(fibers[a]) == expected*p**e
                    fibers_checked += 1
                if n == 2:
                    square_test = a % 8 == 1 if p == 2 else any(b*b % p == a % p for b in range(p))
                    assert (a in fibers) == square_test
                cases += 1
            for j in (-2, -1, 0, 1, 2):
                b = F(p)**j*(p+1)
                a = b**n
                assert vp(a, p) == n*j
                assert root_unit_criterion(a/F(p)**(n*j), p, n, precision)
    assert not root_unit_criterion(3, 2, 2, 5)
    assert root_unit_criterion(9, 2, 2, 5)
    return f"unit_classes={cases} solvable_fibers={fibers_checked} signed_valuation_roots=120 square_examples=2"


def poly_at(coeffs, t, q):
    """Evaluate sum c_i t^i (i starting at 1) modulo q, with coefficients already reduced modulo q."""
    value = 0
    power = 1
    for c in coeffs:
        power = power*t % q
        value += c*power
    return value % q


def branch_image_check(p, n, b, j, relative_in, e):
    """Proposition 15's output exponent at modulus p^3, both directions, from the exact expansion.

    root = p^j b is the branch centre, nin = n*j+relative_in the input exponent, and the output
    exponent under test is computed below from nin, e, n and j: that is the rule line of
    Proposition 15. For every offset t modulo p^3 the image offset ((root + p^nout t)^n - root^n)
    divided by p^nin is computed from the exact binomial coefficients. Each coefficient must be
    p-integral (containment, for every t at once), and the map on offsets must be a bijection modulo
    p^3 (image equality, both directions). Four offsets are recomputed from raw powers as a
    cross-check of the expansion.
    """
    root = F(p)**j*b
    nin = n*j+relative_in
    nout = nin-e-(n-1)*j
    q = p**3
    coeffs = []
    for i in range(1, n+1):
        coeff = F(comb(n, i))*root**(n-i)*F(p)**(nout*i)/F(p)**nin
        assert vp(coeff, p) >= 0, (p, n, b, j, i, coeff)
        coeffs.append(residue(coeff, p, 3))
    assert len({poly_at(coeffs, t, q) for t in range(q)}) == q, (p, n, b, j, relative_in, nout)
    for t in (0, 1, 2, p+1):
        direct = (root+F(p)**nout*t)**n-root**n
        assert vp(direct, p) >= nin, (p, n, b, j, t)
        assert residue(direct/F(p)**nin, p, 3) == poly_at(coeffs, t, q), (p, n, b, j, t)
    return 1


def check_root_precision():
    images = preimages = scaled = relaxed = unique_roots = 0
    for p in PRIMES:
        c = cdisc(p)
        roots = list(dict.fromkeys(x for x in (1, 2, 3, 5, 7, p+2, 2*p+1, 3*p+4) if x % p))[:5]
        for n in (1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 25):
            e = int(vp(n, p))
            for extra in (0, 1):
                relative_out = c+extra
                relative_in = relative_out+e
                k = relative_in+2
                q = p**k
                for b in roots:
                    a = b**n
                    candidates = range(b % p**relative_out, q, p**relative_out)
                    image = {pow(y, n, q) for y in candidates}
                    expected = {(a+t*p**relative_in) % q for t in range(p**2)}
                    assert image == expected, (p, n, relative_in, b)
                    images += 1
                    preimage = {y for y in range(b % p**c, q, p**c)
                                if (pow(y, n, q)-a) % p**relative_in == 0}
                    assert preimage == set(candidates), (p, n, relative_in, b)
                    preimages += 1
                    for j in (-2, 0, 1):
                        scaled += branch_image_check(p, n, b, j, relative_in, e)
    # Remark on Proposition 15: at 2 with odd n the guard r >= 1 suffices. SPEC 9.3.3 keeps r >= 2.
    for n in (1, 3, 5, 7, 9, 11, 25):
        for b in (1, 3, 5, 7, 9, 11, 13):
            for j in (-2, -1, 0, 1, 2):
                relaxed += branch_image_check(2, n, b, j, 1, 0)
    q2 = 2**6
    for n in (1, 3, 5, 7, 9, 11, 25):
        for y in range(1, q2, 4):
            assert sum(1 for z in range(q2) if pow(z, n, q2) == y) == 1, (n, y)
            unique_roots += 1
    assert {x*x % 8 for x in range(1, 8, 2)} == {1}
    assert 5 not in {x*x % 8 for x in range(8)}
    assert 4 not in {x**3 % 9 for x in range(9)}
    return (f"branch_images={images} branch_preimages={preimages} scaled_images_p3={scaled} "
            f"guard_r1_at_2={relaxed} unique_roots_at_2={unique_roots} guard_failures=2")


def check_global_roots():
    rationals = primes_constructed = 0
    for n in range(1, 7):
        for num in range(-12, 13):
            for den in range(1, 9):
                a = F(num, den)
                # Independent integer root searches, no factorisation in this oracle.
                rational_exists = (any(j**n == abs(a.numerator) for j in range(13))
                                   and any(j**n == a.denominator for j in range(1, 9))
                                   and (a >= 0 or n % 2))
                local_necessary = ((not a or all(vp(a, p) % n == 0 for p in small_primes(13)))
                                   and (a >= 0 or n % 2))
                assert bool(rational_exists) == bool(local_necessary)
                rationals += 1
    for ell in (2, 3, 5):
        for given in ((2,), (2, 3), (2, 3, 5)):
            blocked = set(given) | {ell}
            a = ell
            for p in blocked:
                a *= p
            h = sum(a**j for j in range(ell))
            q = next((d for d in range(2, int(h**0.5)+1) if h % d == 0), h)
            assert q not in blocked and (q-1) % ell == 0
            assert pow(a, ell, q) == 1 and a % q != 1
            assert any(pow(r, (q-1)//ell, q) != 1 for r in range(1, min(q, 100)))
            primes_constructed += 1
    tuples = {tuple(1 if mask & (1 << i) else -1 for i in range(8)) for mask in range(2**8)}
    assert len(tuples) == 256 and all(all(x*x == 1 for x in t) for t in tuples)
    assert F(2)**3 == 8 and all(j*j != 2 for j in range(3))
    return f"rational_existence_cases={rationals} unrestricted_prime_witnesses={primes_constructed} sign_tuples=256"


def check_powers():
    principal = signs = obstructions = 0
    for p in PRIMES:
        n = 8
        q = p**n
        for u in (1, 1+p**cdisc(p), 1-2*p**cdisc(p)):
            logu = series("log", u, p, n)
            for s in (-5, -1, 0, 1, 2, p, p*p):
                analytic = series("exp", s*logu, p, n)
                assert analytic == pow(u, s, q)
                principal += 1
        assert series("exp", iwasawa(p, p, n), p, n) == 1 != p
        obstructions += 1
        if p != 2:
            w = teich(2, p, n)
            assert w != 1
            assert all(pow(w, p**j, q) == w for j in range(1, 5))
            obstructions += 1
    for x in (-7, -3, -1, 1, 3, 5, 7):
        _, w, u = components(x, 2, 10)
        for s in (-3, -1, 0, 1, 2, 3, 8):
            actual = w**(s % 2)*series("exp", s*series("log", u, 2, 10), 2, 10) % 1024
            assert actual == pow(x, s, 1024)
            signs += 1
    return (f"integer_compatibility={principal} two_adic_sign_powers={signs} "
            f"Log_and_torsion_obstructions={obstructions}")


def quotient_ball(center, p, radius, precision):
    radius = min(radius, precision)
    return {(center+j*p**radius) % p**precision for j in range(p**(precision-radius))}


def check_power_precision():
    uncertain = exact = zero_centres = uncapped = 0
    uncapped_by_p = {}
    for p in PRIMES:
        c = cdisc(p)
        k = 5 if p == 2 else 3
        q = p**k
        # u0 with alpha = v_p(log u0) equal to c and strictly above c, and alpha = infinity at u0 = 1.
        for u0 in (1, 1+p**c, 1+2*p**c, 1+p**(c+1), 1+3*p**(c+1)):
            alpha = vp(series("log", u0, p, k), p)
            for a in range(c, min(c+2, k)+1):
                bases = quotient_ball(u0, p, a, k)
                for s0 in (0, 1, -1, p, p*p):
                    center = pow(u0, s0, q)
                    beta = vp(s0, p)
                    for b in range(0, min(2, k-c)+1):
                        exponent_modulus = p**max(b, k-c)
                        exponents = range(s0 % p**b, exponent_modulus, p**b)
                        actual = {pow(u, s, q) for u in bases for s in exponents}
                        r = min(a+beta, b+alpha, a+b, k)
                        assert actual == quotient_ball(center, p, r, k), (p, a, b, u0, s0, r)
                        if min(a+beta, b+alpha, a+b) < k:
                            uncapped += 1
                            uncapped_by_p[p] = uncapped_by_p.get(p, 0) + 1
                        uncertain += 1
                        zero_centres += (u0 == 1 or s0 == 0)
                    actual = {pow(u, s0, q) for u in bases}
                    r = min(a+beta, k)
                    assert actual == quotient_ball(center, p, r, k)
                    exact += 1
                for b in range(3):
                    # Exact base; enumerate exponent residue classes for centre zero.
                    period = p**max(b, k-c)
                    actual = {pow(u0, s, q) for s in range(0, period, p**b)}
                    assert actual == quotient_ball(1, p, min(b+alpha, k), k)
                    exact += 1
    return (f"independent_uncertain_images={uncertain} one_exact_input_images={exact} "
            f"zero_product_centres={zero_centres} uncapped_radii={uncapped} by_p={uncapped_by_p}")


def fractional_part(x, p):
    x = F(x)
    if vp(x, p) >= 0:
        return F(0)
    k = int(-vp(x, p))
    return F(residue(x*p**k, p, k), p**k)


def check_fractional_parts():
    fractions = ball_witnesses = 0
    for p in PRIMES:
        for num in range(-12, 13):
            for den in range(1, 16):
                x = F(num, den)
                r = fractional_part(x, p)
                assert 0 <= r < 1 and vp(x-r, p) >= 0
                k = max(0, int(-vp(x, p))) if x else 0
                candidates = [F(a, p**k) for a in range(p**k)
                              if vp(x-F(a, p**k), p) >= 0]
                assert candidates == [r]
                # Independent digit extraction proceeds from the lowest degree upwards.
                remainder, digits = x, F(0)
                for j in range(-k, 0):
                    digit = residue(remainder/F(p)**j, p, 1)
                    digits += digit*F(p)**j
                    remainder -= digit*F(p)**j
                assert digits == r and vp(remainder, p) >= 0
                fractions += 1
                for exponent in (-2, -1, 0, 1):
                    same = fractional_part(x+F(p)**exponent, p) == r
                    assert same == (exponent >= 0)
                    ball_witnesses += 1
    assert fractional_part(F(1, 3), 2) == 0 and F(1, 3).denominator != 1
    return f"unique_sections={fractions} digit_oracles={fractions} ball_witnesses={ball_witnesses}"


def check_real_and_character():
    roots = phases = additivity = 0
    for n in range(1, 9):
        for b in range(-8, 9):
            a = b**n
            actual = {t for t in range(-8, 9) if t**n == a}
            expected = {b} if n % 2 else {abs(b), -abs(b)}
            assert actual == expected
            roots += 1
        if n % 2 == 0:
            assert all(b**n != -1 for b in range(-8, 9))
    primes = small_primes(19)
    for num in range(-16, 17):
        for den in range(1, 17):
            q = F(num, den)
            phase = -q+sum((fractional_part(q, p) for p in primes), F(0))
            assert phase.denominator == 1
            phases += 1
    for p in PRIMES:
        for a in (F(-1, p), F(1, p*p), F(2, 3)):
            for b in (F(3, p), F(-1, p*p), F(4, 5)):
                delta = fractional_part(a+b, p)-fractional_part(a, p)-fractional_part(b, p)
                assert delta.denominator == 1
                additivity += 1
    # Check phases exactly. Exp(0)=1 and cos(pi/2)=0 use the stated real/complex standard theorem.
    assert -F(1, 4)+fractional_part(F(1, 4), 2) == 0
    return f"real_integer_root_sets={roots} rational_triviality_phases={phases} additive_phase_cases={additivity}"


def check_no_order():
    lifts = sums = 0
    for k in range(1, 6):
        q = 5**k
        roots = [b for b in range(q) if (b*b+1) % q == 0]
        assert len(roots) == 2 and {b % 5 for b in roots} == {2, 3}
        lifts += 1
    for p in (3, 5, 13):
        a, b = next((a, b) for a in range(p) for b in range(p) if (a*a+b*b+1) % p == 0)
        if not a:
            a, b = b, a
        modulus = p
        for _ in range(3):
            choices = [a+t*modulus for t in range(p)
                       if ((a+t*modulus)**2+b*b+1) % (modulus*p) == 0]
            assert len(choices) == 1
            a = choices[0]
            modulus *= p
        assert (a*a+b*b+1) % modulus == 0
        sums += 1
    for k in range(3, 11):
        q = 2**k
        roots = [z for z in range(q) if (z*z+7) % q == 0]
        assert len(roots) == 4
        assert all((z*z+1+1+4+1) % q == 0 for z in roots)
        sums += 1
    return f"Q5_root_quotients={lifts} sum_of_squares_quotients={sums}"


def check_typed_and_projection():
    valuations = real_balls = projections = 0
    for p in PRIMES:
        for a in (F(0), F(1, p*p), F(-1), F(p), F(p*p)):
            for n in range(-2, 4):
                values = [a+F(p)**n*j for j in range(p*p)]
                if vp(a, p) < n:
                    assert {vp(x, p) for x in values} == {vp(a, p)}
                    assert all(F(p)**(-int(vp(x, p))) == F(p)**(-int(vp(a, p))) for x in values)
                else:
                    # The ball is p^n Z_p and contains 0 and p^n, of two different valuations. The grid
                    # must exhibit at least two valuations: this branch certifies NOT_DETERMINED.
                    vals = {vp(x, p) for x in values}
                    assert len(vals) >= 2 and vp(-a, p) >= n, (p, a, n, vals)
                valuations += 1
    from math import ceil, floor
    sign = lambda x: (x > 0)-(x < 0)
    for lo, hi in ((F(-1), F(1)), (F(0), F(1, 2)), (F(1, 4), F(3, 4)),
                   (F(-3, 2), F(-1)), (F(1), F(1))):
        for f in (sign, floor, ceil):
            points = [lo+(hi-lo)*F(j, 8) for j in range(9)]
            actual = {f(x) for x in points}
            assert (len(actual) == 1) == (f(lo) == f(hi))
            real_balls += 1
    coords = {p: p**cdisc(p) for p in PRIMES}
    for named in ((2,), (3, 13), PRIMES):
        result = {p: series("sin", coords[p], p, 6) for p in named}
        assert set(result) == set(named)
        for p in named:
            assert vp(result[p], p) == cdisc(p)
            assert result[p] == residue(partial("sin", coords[p], 30), p, 6)
        projections += 1
    assert all(vp(F(1, p), p) < 0 for p in PRIMES)
    return f"valuation_balls={valuations} real_jump_balls={real_balls} named_projections={projections}"


def run_mutant(source, anchor, new, check):
    """Plant `new` for `anchor` in a copy of `source` and run `check` on the copy; rejection demanded."""
    if source.count(anchor) != 1:
        return "skipped(anchor not present exactly once)", ""
    mutated = source.replace(anchor, new)
    ns = {"__name__": "functions_checks_mutant"}
    exec(compile(mutated, "functions_checks_mutant", "exec"), ns)
    try:
        ns[check]()
    except AssertionError as err:
        detail = ""
        frames = traceback.extract_tb(err.__traceback__)
        if frames:
            line = mutated.splitlines()[frames[-1].lineno-1].strip()
            detail = f"(assert at line {frames[-1].lineno}: {line})"
        return "killed", detail
    except Exception as ex:  # noqa: BLE001  a crash is detection too, but is reported as such
        return f"killed(crash:{type(ex).__name__})", ""
    return "SURVIVED", ""


# Each mutant is (name, anchor pieces, replacement, check under test). Anchors are given as pieces and
# joined at run time, so that every full anchor occurs exactly once in this file: at the rule under test.
# Two replacements that extend an anchor are split into pieces for the same reason. This keeps the
# literal-anchor mutant table in the review's checks working unchanged.
A_K = ("k = max(1, ceildiv((p-1)*n-1, ", "(p-1)*v-1))")
A_SIN = ("sin=k//2, ", "sinh=k//2")
A_COS = ("cos=(k+1)//2, ", "cosh=(k+1)//2")
A_LOG = ('answer = {"log": max(1, ceildiv(2*n, 2*v-1)) - ', '1}')
A_W = ("w = max(v, ", "n+d)")
A_HULL = ("output_n = 2*exponent-", "(p == 2)")
A_OUTR = ("out_r = 2 if p == 2 and r == 1 else ", "r")
A_CRIT = (">= cdisc(p)", "+vp(n, p)")
A_NOUT = ("nout = nin-e-", "(n-1)*j")
A_GUARD = ("relative_in = relative_out+", "e")
A_R = ("r = min(a+beta, b+", "alpha, a+b, k)")
A_ALPHA = ("alpha = vp(series(", '"log", u0, p, k), p)')
A_VAL = ("if vp(a, p) < n", ":")
A_TIGHT_RETURN = ("return j ", "- 1")
A_TIGHT_LOOP = ("while j*v - ilog(j, p)", " < n:")

# The eleven anchors that docs/reviews/m0-proofs/functions_review_checks.py matches literally in this
# file. They must stay unique, or that review's section M stops working.
REVIEW_ANCHORS = (A_K, A_SIN, A_COS, A_LOG, A_W, A_HULL, A_OUTR, A_CRIT, A_NOUT, A_GUARD, A_R)

MUTANTS = (
    ("K-1 for the factorial series", A_K, "k = max(1, ceildiv((p-1)*n-1, (p-1)*v-1)-1)", "check_truncation"),
    ("d=(p-1)v instead of (p-1)v-1", A_K, "k = max(1, ceildiv((p-1)*n-1, (p-1)*v))", "check_truncation"),
    ("T_sin = floor((K-1)/2)", A_SIN, "sin=(k-1)//2, sinh=(k-1)//2", "check_truncation"),
    ("T_cos = floor(K/2)", A_COS, "cos=k//2, cosh=k//2", "check_truncation"),
    ("T_log = J-2", A_LOG, 'answer = {"log": max(1, ceildiv(2*n, 2*v-1)) - 2}', "check_truncation"),
    ("T_log with 2v instead of 2v-1", A_LOG, 'answer = {"log": max(1, ceildiv(2*n, 2*v)) - 1}',
     "check_truncation"),
    ("tight T_log = J*-2", A_TIGHT_RETURN, "return j - 2", "check_truncation"),
    ("tight log rule drops the e(k) bound", A_TIGHT_LOOP, "while j*v < n:", "check_truncation"),
    ("W = max(v, n+D-1)", A_W, "w = max(v, n+d-1)", "check_working_precision"),
    ("W = max(v, n) (no D)", A_W, "w = max(v, n)", "check_working_precision"),
    ("W = n+D (drop v, clamped at 1)", A_W, "w = max(1, n+d)", "check_working_precision"),
    ("W = n+D (no maximum at all)", A_W, "w = n+d", "check_working_precision"),
    ("cos hull 2N (drop v_p(2))", A_HULL, "output_n = 2*exponent", "check_series_radii"),
    ("cos hull 2N-1 everywhere", A_HULL, "output_n = 2*exponent-1", "check_series_radii"),
    ("Log r=1 at 2 gives Log(a)+2Z_2", A_OUTR, "out_r = r", "check_log_radii"),
    ("Log image r+1", A_OUTR, ("out_r = 2 if p == 2 and r == 1 else ", "r+1"), "check_log_radii"),
    ("root log condition c+v(n) -> 1+v(n)", A_CRIT, ">= 1+vp(n, p)", "check_root_criteria"),
    ("root log condition drops v(n)", A_CRIT, ">= cdisc(p)", "check_root_criteria"),
    ("root out exponent -n j", A_NOUT, "nout = nin-e-n*j", "check_root_precision"),
    ("root out exponent without v(n)", A_NOUT, "nout = nin-(n-1)*j", "check_root_precision"),
    ("root out exponent caps the p-power loss at 1 digit at odd p", A_NOUT,
     "nout = nin-(e if p == 2 else min(e, 1))-(n-1)*j", "check_root_precision"),
    ("root guard one digit lower", A_GUARD, ("relative_in = relative_out+", "e-1"), "check_root_precision"),
    ("power R drops A+B", A_R, "r = min(a+beta, b+alpha, k)", "check_power_precision"),
    ("power R drops B+alpha", A_R, "r = min(a+beta, a+b, k)", "check_power_precision"),
    ("power R = A+B+1 term", A_R, "r = min(a+beta, b+alpha, a+b+1, k)", "check_power_precision"),
    ("power R uses alpha+1", A_R, "r = min(a+beta, b+alpha+1, a+b, k)", "check_power_precision"),
    ("log u0 exponent alpha capped at c", A_ALPHA,
     'alpha = cdisc(p) if vp(series("log", u0, p, k), p) < inf else inf', "check_power_precision"),
    ("valuation determined one digit earlier", A_VAL, "if vp(a, p) < n-1:", "check_typed_and_projection"),
)


def run_mutation_tests(target=None):
    """Plant every wrong rule of MUTANTS in a copy of the target source and run the named check.

    Returns (lines, killed, survivors, skipped). check_mutation_testing runs it on this file (the green
    half of the red-green record); lanes/m0-repair-functions/run_red.py runs it on the snapshot taken
    before this lane's strengthening (the red half).
    """
    path = Path(target).resolve() if target else Path(__file__).resolve()
    source = path.read_text()
    for anchor in REVIEW_ANCHORS:
        text = "".join(anchor)
        assert source.count(text) == 1, ("review anchor must stay unique", text, source.count(text))
    lines, killed, survivors, skipped = [], 0, [], []
    for name, anchor, new, check in MUTANTS:
        old_text = "".join(anchor)
        new_text = new if isinstance(new, str) else "".join(new)
        outcome, detail = run_mutant(source, old_text, new_text, check)
        if outcome.startswith("skipped"):
            skipped.append(name)
        elif outcome == "SURVIVED":
            survivors.append(name)
        else:
            killed += 1
        lines.append(f"mutant {name!r} -> {check}: {outcome} {detail}".rstrip())
    return lines, killed, survivors, skipped


def check_mutation_testing(target=None):
    """Real mutation tests: plant each wrong rule in a copy of the rule under test and demand rejection.

    Wrong truncation counts (safe and tight), wrong working precision W including the variant
    W = n+D without the maximum, wrong radius and root rules, wrong power radius, the wrong
    valuation-determination digit and the wrong log(u0) exponent must all be rejected by an assertion
    of the named check. The fixed-value regression list this check replaces could not do that.
    """
    lines, killed, survivors, skipped = run_mutation_tests(target)
    for line in lines:
        print("     " + line, flush=True)
    assert not survivors, ("wrong rules not rejected", survivors)
    assert not skipped, ("anchors missing in the source under test", skipped)
    return (f"planted_wrong_rules={len(MUTANTS)} rejected={killed} survivors=0 "
            f"review_anchors_unique={len(REVIEW_ANCHORS)}")


def main():
    checks = (check_legendre, check_domains, check_lifting_and_groups, check_decomposition,
              check_truncation, check_working_precision, check_series_identities, check_series_radii,
              check_log_radii, check_global, check_root_criteria, check_root_precision, check_global_roots,
              check_powers, check_power_precision, check_fractional_parts, check_real_and_character,
              check_no_order, check_typed_and_projection, check_mutation_testing)
    for check in checks:
        print(f"{check.__name__}: {check()}", flush=True)



# ====================================================================================================
# Section f-slice1 (lane f-slice1, 2026-09-29): balls of Q_p at one prime, adf_lball.
# Statements L0 to L8 of docs/api-1f.md, section "Statements to add to functions.md". The reference of the
# C library (tests/test_lball.c reads the vectors of lanes/f-slice1/gen_vectors.py) and their finite checks.
# Not run by main(); run by lball_main() (called after main() below). Standard library only.
#
# A value is LB(p, exact, u, v, N): exact: the rational p^v u; ball: p^v u + p^N Z_p (conventions 5.8).
# The results are computed from the statements L2 to L5 on the RATIONAL centres, and are checked against a
# different method: enumeration of points modulo p^(K + 1) (lb_enum_check), K the exponent of the result.
# ====================================================================================================

import random


class LB:
    __slots__ = ("p", "exact", "u", "v", "N")

    def __init__(self, p, exact, u, v, N):
        self.p, self.exact, self.u, self.v, self.N = p, int(exact), F(u), v, N

    def key(self):
        return (self.p, self.exact, self.u, self.v, self.N)

    def __eq__(self, other):
        return isinstance(other, LB) and self.key() == other.key()

    def __hash__(self):
        return hash(self.key())

    def __repr__(self):
        return f"LB{self.key()}"

    def json(self):
        return {"p": self.p, "exact": self.exact, "un": self.u.numerator, "ud": self.u.denominator,
                "v": self.v, "N": self.N}


def lb_val(x):
    """The rational p^v u (the value of an exact x, the centre of a ball)."""
    return x.u * F(x.p) ** x.v


def lb_exact(p, q):
    q = F(q)
    if q == 0:
        return LB(p, 1, 0, 0, 0)
    w = vp(q, p)
    return LB(p, 1, q / F(p) ** w, w, 0)


def lb_ball(p, c, N):
    """L0: the canonical form of c + p^N Z_p."""
    c = F(c)
    if c == 0 or vp(c, p) >= N:
        return LB(p, 0, 0, 0, N)
    w = vp(c, p)
    k = N - w
    t = c / F(p) ** w
    u = (t.numerator * pow(t.denominator, -1, p ** k)) % (p ** k)
    assert 0 < u < p ** k and u % p != 0
    assert vp(c - F(p) ** w * u, p) >= N
    return LB(p, 0, u, w, N)


def lb_is_canonical(x):
    p = x.p
    if x.exact:
        return x.N == 0 and (x.u == 0 and x.v == 0 or x.u != 0 and vp(x.u, p) == 0)
    if x.u.denominator != 1:
        return False
    if x.u == 0:
        return x.v == 0
    return x.v < x.N and x.u % p != 0 and 0 < x.u < F(p) ** (x.N - x.v)


def lb_zero_v(x):
    """The valuation of the centre; infinity for the centre 0."""
    return inf if x.u == 0 else x.v


def lb_ref_neg(x):
    return lb_exact(x.p, -lb_val(x)) if x.exact else lb_ball(x.p, -lb_val(x), x.N)


def lb_ref_add(x, y):
    """L2."""
    if x.exact and y.exact:
        return lb_exact(x.p, lb_val(x) + lb_val(y))
    K = min(n.N for n in (x, y) if not n.exact)
    return lb_ball(x.p, lb_val(x) + lb_val(y), K)


def lb_ref_sub(x, y):
    return lb_ref_add(x, lb_ref_neg(y))


def lb_ref_mul(x, y):
    """L3."""
    p = x.p
    if (x.exact and x.u == 0) or (y.exact and y.u == 0):
        return lb_exact(p, 0)
    if x.exact and y.exact:
        return lb_exact(p, lb_val(x) * lb_val(y))
    vx, vy = lb_zero_v(x), lb_zero_v(y)
    terms = []
    if not y.exact:
        terms.append(vx + y.N)
    if not x.exact:
        terms.append(vy + x.N)
    if not x.exact and not y.exact:
        terms.append(x.N + y.N)
    K = min(terms)
    assert K != inf
    return lb_ball(p, lb_val(x) * lb_val(y), K)


def lb_ref_inv(x):
    """L4: an LB, or the name of a status."""
    if x.exact:
        return "NOT_UNIT" if x.u == 0 else lb_exact(x.p, 1 / lb_val(x))
    if x.u == 0:
        return "UNIT_NOT_CERTIFIED"
    return lb_ball(x.p, 1 / lb_val(x), x.N - 2 * x.v)


def lb_ref_div(x, y):
    r = lb_ref_inv(y)
    return r if isinstance(r, str) else lb_ref_mul(x, r)


def lb_ref_div_direct(x, y):
    """L4a (lane f-repair1): the quotient from the valuations and precisions alone, without forming 1/y.

    y = p^w t (+ p^M Z_p), t != 0, and x = p^v u (+ p^N Z_p): the centre is (u/t) p^(v - w), and
    K = min(v + M - 2w [y ball, u != 0], N - w [x ball], N + M - 2w [both balls]).
    """
    p = x.p
    if y.u == 0:
        return "NOT_UNIT" if y.exact else "UNIT_NOT_CERTIFIED"
    if x.exact and x.u == 0:
        return lb_exact(p, 0)
    w = y.v
    e = 0 if x.u == 0 else x.v - w
    centre = (x.u / y.u) * F(p) ** e
    if x.exact and y.exact:
        return lb_exact(p, centre)
    terms = []
    if not y.exact and x.u != 0:
        terms.append(x.v + y.N - 2 * w)
    if not x.exact:
        terms.append(x.N - w)
    if not x.exact and not y.exact:
        terms.append(x.N + y.N - 2 * w)
    return lb_ball(p, centre, min(terms))


def lb_ref_valuation(x):
    """(status, v, is_inf)."""
    if x.exact:
        return ("OK", 0, 1) if x.u == 0 else ("OK", x.v, 0)
    return ("NOT_DETERMINED", None, None) if x.u == 0 else ("OK", x.v, 0)


def lb_ref_abs(x):
    if x.exact:
        return ("OK", F(0)) if x.u == 0 else ("OK", F(x.p) ** (-x.v))
    return ("NOT_DETERMINED", None) if x.u == 0 else ("OK", F(x.p) ** (-x.v))


def lb_ref_decompose(x):
    """L6: (status, m, unit LB)."""
    if x.exact:
        if x.u == 0:
            return ("DOMAIN", None, None)
        return ("OK", x.v, lb_exact(x.p, x.u))
    if x.u == 0:
        return ("NOT_DETERMINED", None, None)
    return ("OK", x.v, LB(x.p, 0, x.u, 0, x.N - x.v))


def lb_dv(x, y):
    """v_p(value(x) - value(y)) by the last paragraph of L8 (no power of p)."""
    if x.u == 0 and y.u == 0:
        return inf
    if x.u == 0:
        return y.v
    if y.u == 0:
        return x.v
    if x.v != y.v:
        return min(x.v, y.v)
    d = vp(x.u - y.u, x.p)
    return inf if d == inf else x.v + d


def lb_ref_equal_set(x, y):
    return x.key() == y.key()


def lb_ref_overlaps(x, y):
    if x.p != y.p:
        return False
    if x.exact and y.exact:
        return lb_val(x) == lb_val(y)
    n = min(z.N for z in (x, y) if not z.exact)
    return lb_dv(x, y) >= n


def lb_ref_contains(x, y):
    """The set x is inside the set y."""
    if x.p != y.p:
        return False
    if y.exact:
        return x.exact and lb_val(x) == lb_val(y)
    if x.exact:
        return lb_dv(x, y) >= y.N
    return x.N >= y.N and lb_dv(x, y) >= y.N


def lb_ref_project(p, A, H, d):
    """L1."""
    if H == 0:
        return lb_exact(p, F(A, d))
    return lb_ball(p, F(A, d), vp(H, p) - vp(d, p))


def lb_points(x, t=1):
    """The points of x that matter modulo the next digit: c + p^N a, a in [0, p^t); the value for exact x."""
    if x.exact:
        return [lb_val(x)]
    return [lb_val(x) + F(x.p) ** x.N * a for a in range(x.p ** t)]


def lb_residue_class(r, c, K, p):
    """(r - c)/p^K mod p; asserts that r lies in c + p^K Z_p."""
    d = r - c
    if d == 0:
        return 0
    assert vp(d, p) >= K, (r, c, K)
    d = d / F(p) ** K
    return (d.numerator * pow(d.denominator, -1, p)) % p


def lb_enum_check(op, x, y=None, t=1):
    """The result ball of op contains every result of the points and no smaller ball does (finite check).

    Returns the reference result. The points are those of lb_points (a modulo p^t); by the case analysis of
    L2 to L5 t = 1 suffices, and check_lball_enumeration repeats with t = 2 on small cases as a check of that
    remark. The result R = c' + p^K Z_p contains every result when vp(r - c') >= K is asserted for each
    point result r, and is tight when the classes (r - c')/p^K mod p cover all p residues (a ball of
    exponent K + 1 or more would miss one of them).
    """
    p = x.p
    if op == "neg":
        ref = lb_ref_neg(x)
        results = [-s for s in lb_points(x, t)]
    elif op == "inv":
        ref = lb_ref_inv(x)
        if isinstance(ref, str):
            return ref
        results = [1 / s for s in lb_points(x, t) if s != 0]
    else:
        ref = {"add": lb_ref_add, "sub": lb_ref_sub, "mul": lb_ref_mul, "div": lb_ref_div}[op](x, y)
        if isinstance(ref, str):
            return ref
        f = {"add": lambda s, u: s + u, "sub": lambda s, u: s - u, "mul": lambda s, u: s * u,
             "div": lambda s, u: s / u}[op]
        results = [f(s, u) for s in lb_points(x, t) for u in lb_points(y, t) if not (op == "div" and u == 0)]
    if ref.exact:
        assert all(r == lb_val(ref) for r in results), (op, x, y, ref)
        return ref
    c = lb_val(ref)
    classes = {lb_residue_class(r, c, ref.N, p) for r in results}
    assert classes == set(range(p)), (op, x, y, ref, sorted(classes))
    return ref


def lb_universe(p, vmax, kmax, exact_extra=()):
    """All canonical balls with valuation in [-vmax, vmax], relative precision 1..kmax, the balls around 0 with
    N in [-vmax, kmax], and the exact values 0, +-p^v (|v| <= vmax) and exact_extra."""
    out = []
    for v in range(-vmax, vmax + 1):
        for k in range(1, kmax + 1):
            for u in range(1, p ** k):
                if u % p:
                    out.append(LB(p, 0, u, v, v + k))
    for N in range(-vmax, kmax + 1):
        out.append(LB(p, 0, 0, 0, N))
    out.append(lb_exact(p, 0))
    for v in range(-vmax, vmax + 1):
        for sgn in (1, -1):
            out.append(lb_exact(p, sgn * F(p) ** v))
    for q in exact_extra:
        out.append(lb_exact(p, q))
    seen, uniq = set(), []
    for x in out:
        assert lb_is_canonical(x), x
        if x not in seen:
            seen.add(x)
            uniq.append(x)
    return uniq


def check_lball_reduction():
    """L0: the canonical centre, on 3000 random rationals at p = 2, 3, 5, 7 and 2^64 - 59."""
    rng = random.Random(20260929)
    n = 0
    for p in (2, 3, 5, 7, 2 ** 64 - 59):
        for _ in range(600):
            q = F(rng.randint(-10 ** 6, 10 ** 6), rng.randint(1, 10 ** 4)) * F(p) ** rng.randint(-4, 4)
            N = rng.randint(-5, 6)
            x = lb_ball(p, q, N)
            assert lb_is_canonical(x)
            n += 1
    return f"reduced={n}"


def check_lball_projection():
    """L1: the p-th coordinates of (A + H Zhat)/d, by enumeration of z_p modulo p^2."""
    rng = random.Random(1)
    n = 0
    for p in (2, 3, 5, 7):
        for _ in range(120):
            d = rng.randint(1, 60)
            H = rng.choice([0, rng.randint(1, 500)])
            A = rng.randint(-100, 100)
            g = gcd(gcd(A, H), d)
            A, H, d = A // g, H // g, d // g
            ref = lb_ref_project(p, A, H, d)
            if H == 0:
                assert ref.exact and lb_val(ref) == F(A, d)
            else:
                e = vp(F(H, d), p)
                assert ref.N == e
                cs = {lb_residue_class(F(A, d) + F(H, d) * z, lb_val(ref), e, p) for z in range(p * p)}
                assert cs == set(range(p)), (p, A, H, d, ref)
            n += 1
    return f"projected={n}"


def check_lball_enumeration():
    """L2 to L5: every result ball contains the results of all points and is not contained in a smaller ball.

    Exhaustive on the universe of p = 2 (valuations -2..2, relative precision up to 3) and p = 3 (-1..1, up to
    2), for add, sub, mul, div, and neg, inv on every ball; a random sample of pairs at p = 5 and 7; a repeat
    with t = 2 (more digits of a) on p = 2.
    """
    counts = {}
    rng = random.Random(7)
    for p, vmax, kmax, full in ((2, 2, 3, True), (3, 1, 2, True), (5, 1, 2, False), (7, 1, 2, False)):
        uni = lb_universe(p, vmax, kmax)
        pairs = [(x, y) for x in uni for y in uni]
        if not full:
            pairs = rng.sample(pairs, 1500)
        for op in ("add", "sub", "mul", "div"):
            for x, y in pairs:
                lb_enum_check(op, x, y)
                counts[op] = counts.get(op, 0) + 1
        for x in uni:
            for op in ("neg", "inv"):
                lb_enum_check(op, x)
                counts[op] = counts.get(op, 0) + 1
    uni2 = lb_universe(2, 1, 2)
    for x in uni2:
        for y in uni2:
            for op in ("add", "mul"):
                a = lb_enum_check(op, x, y, t=1)
                b = lb_enum_check(op, x, y, t=2)
                assert a == b
                counts["t2"] = counts.get("t2", 0) + 1
    return " ".join(f"{k}={v}" for k, v in sorted(counts.items()))


def check_lball_decompose():
    """L6, L7: x = p^m U, and every element of x has valuation m (points of the ball, p = 2, 3)."""
    n = 0
    for p in (2, 3):
        for x in lb_universe(p, 2, 3):
            st, m, U = lb_ref_decompose(x)
            if st != "OK":
                assert x.u == 0
                continue
            for s in lb_points(x, 2):
                assert vp(s, p) == m
            if not x.exact:
                assert lb_is_canonical(U) and U.v == 0 and U.N == x.N - m and U.N >= 1
                assert lb_ref_mul(lb_exact(p, F(p) ** m), U) == x
            else:
                assert U.exact and lb_val(U) * F(p) ** m == lb_val(x) and vp(lb_val(U), p) == 0
            n += 1
    return f"decomposed={n}"


def check_lball_quotient():
    """L4a: the direct quotient equals the quotient by way of the inverse, on every pair of the universes of
    p = 2 (valuations -2..2, relative precision up to 3), p = 3 (-2..2, up to 2) and p = 5 (-1..1, up to 2)."""
    n = 0
    for p, vmax, kmax in ((2, 2, 3), (3, 2, 2), (5, 1, 2)):
        uni = lb_universe(p, vmax, kmax)
        for x in uni:
            for y in uni:
                a, b = lb_ref_div(x, y), lb_ref_div_direct(x, y)
                assert a == b, (x, y, a, b)
                n += 1
    return f"quotients={n}"


def lball_main():
    for check in (check_lball_reduction, check_lball_projection, check_lball_enumeration, check_lball_decompose,
                  check_lball_quotient):
        print(f"{check.__name__}: {check()}", flush=True)


# ====================================================================================================
# Section f-slice2 (lane f-slice2, 2026-09-29): partial balls adf_sball and the real functions.
# Statements S1 to S7 of docs/api-1f.md, section "Slice 1F.1-a / 1F.2-a". The reference of the C library
# (tests/test_sball.c and tests/test_rfunc.c read the vectors of lanes/f-slice2/gen_vectors.py).
# Not run by main(); run by sball_main() (called after lball_main() below). mpmath is imported inside the
# functions that need it (interval arithmetic at 800 bits: the oracle for exp, log, sin, cos); the roots are
# enclosed by exact integer arithmetic (rf_iroot), with no library at all.
#
# A real ball is RB(m, e, rm, re): the closed interval [m 2^e - rm 2^re, m 2^e + rm 2^re] (arb: exact midpoint,
# radius a mag with rm < 2^30, so the radius is exact).
# ====================================================================================================

IV_PREC = 800


def dy(m, e):
    """The rational m 2^e."""
    return F(m) * F(2) ** e


def dy_parts(q):
    """(m, e) with q = m 2^e, m odd (or 0, 0). q must be dyadic."""
    q = F(q)
    if q == 0:
        return (0, 0)
    d = q.denominator
    assert d & (d - 1) == 0, "not dyadic"
    e = -(d.bit_length() - 1)
    m = q.numerator
    while m % 2 == 0:
        m //= 2
        e += 1
    return (m, e)


def dy_json(q):
    m, e = dy_parts(q)
    return {"m": m, "e": e}


BOUND_BITS = 400


def dy_json_bound(q, up):
    """A dyadic with a mantissa of about BOUND_BITS bits that is >= q (up) or <= q: the bound as it is written into a
    vector file (shorter than the exact end point of the 800 bit enclosure; the rounding is outward)."""
    q = F(q)
    if q == 0:
        return {"m": 0, "e": 0}
    a = abs(q)
    k = a.numerator.bit_length() - a.denominator.bit_length() - BOUND_BITS - 1
    t = q / F(2) ** k
    m = -((-t.numerator) // t.denominator) if up else t.numerator // t.denominator
    return {"m": m, "e": k}


class RB:
    __slots__ = ("m", "e", "rm", "re")

    def __init__(self, m, e, rm=0, re=0):
        assert 0 <= rm < 2 ** 30
        self.m, self.e, self.rm, self.re = m, e, rm, re

    @property
    def mid(self):
        return dy(self.m, self.e)

    @property
    def rad(self):
        return dy(self.rm, self.re)

    @property
    def lo(self):
        return self.mid - self.rad

    @property
    def hi(self):
        return self.mid + self.rad

    def json(self):
        return {"m": self.m, "e": self.e, "rm": self.rm, "re": self.re}


def rf_iroot(x, n):
    """floor of the n-th root of the integer x >= 0 (Newton, exact)."""
    if x < 2:
        return x
    r = 1 << ((x.bit_length() + n - 1) // n)
    while True:
        s = ((n - 1) * r + x // r ** (n - 1)) // n
        if s >= r:
            break
        r = s
    while r ** n > x:
        r -= 1
    while (r + 1) ** n <= x:
        r += 1
    return r


ROOT_P = 700   # bits after the point of the enclosure of a root


def rf_root_enclosure(x, n):
    """[lo, hi] with lo <= x^(1/n) <= hi, x >= 0 a Fraction, by exact integers (n up to 64)."""
    if n == 1:
        return (x, x)
    if x == 0:
        return (F(0), F(0))
    X = (x * 2 ** (n * ROOT_P)).__floor__()
    r = rf_iroot(X, n)
    return (F(r, 2 ** ROOT_P), F(r + 1, 2 ** ROOT_P))


def _ivq(q):
    from mpmath import iv, mp, mpf
    mp.prec = 3000
    q = F(q)
    v = mpf(q.numerator) / mpf(q.denominator)
    iv.prec = IV_PREC
    return iv.mpf(v)


def _iv_bounds(t):
    """(lower, upper) of an mpmath interval as exact Fractions."""
    from mpmath.libmp import to_man_exp

    def conv(x):
        man, exp = to_man_exp(x)[0:2]
        man = -man if x[0] else man   # x = (sign, man, exp, bc)
        return F(man) * F(2) ** exp if man else F(0)
    return (conv(t._mpi_[0]), conv(t._mpi_[1]))


def _iv_fn(name, q):
    from mpmath import iv
    iv.prec = IV_PREC
    return _iv_bounds(getattr(iv, name)(_ivq(q)))


def rf_status(f, n, lo, hi):
    """The status of the wrapper for the ball [lo, hi] (exact end points), from rfunc.h."""
    if f in ("exp", "sin", "cos"):
        return "OK"
    if f == "log":
        return "DOMAIN" if hi <= 0 else ("OK" if lo > 0 else "NOT_DETERMINED")
    if f == "log_abs":
        if lo == 0 and hi == 0:
            return "DOMAIN"
        return "NOT_DETERMINED" if lo <= 0 <= hi else "OK"
    if f == "sqrt" or (f == "root" and n % 2 == 0 and n >= 2):
        return "DOMAIN" if hi < 0 else ("OK" if lo >= 0 else "NOT_DETERMINED")
    if f == "root":
        return "DOMAIN" if n == 0 else "OK"
    raise ValueError(f)


def _rt(x, n):
    """Enclosure of the real n-th root (odd n allowed for x < 0) of a dyadic x."""
    if x < 0:
        lo, hi = _rt(-x, n)
        return (-hi, -lo)
    if n <= 64:
        return rf_root_enclosure(x, n)
    if x == 0:
        return (F(0), F(0))
    from mpmath import iv
    iv.prec = IV_PREC
    return _iv_bounds(iv.exp(iv.log(_ivq(x)) / n))


def _crit_values(name, lo, hi):
    """The values (+1, -1) that sin or cos takes at critical points inside [lo, hi]: sin has them at pi/2 + k pi,
    cos at k pi, with the value (-1)^k. Raises ValueError if an end point is too close to one to decide."""
    from mpmath import iv, floor
    iv.prec = IV_PREC
    pi = iv.pi
    c0 = pi / 2 if name == "sin" else iv.mpf(0)

    def kk(t, upper):
        q = (_ivq(t) - c0) / pi
        fa, fb = int(floor(q.a)), int(floor(q.b))
        if q.a == q.b and fa == q.a:
            return fa
        if fa != fb or q.a == fa:
            raise ValueError("ambiguous critical point")
        return fa if upper else fa + 1
    k1, k2 = kk(lo, False), kk(hi, True)
    return {1 if k % 2 == 0 else -1 for k in range(k1, min(k2, k1 + 1) + 1)}


def rf_image(f, n, lo, hi):
    """(minlo, maxhi): minlo <= min f <= max f <= maxhi over [lo, hi], exact Fractions (mpmath intervals at 800 bits,
    or exact integer roots). The status must be OK."""
    if f == "exp":
        return (_iv_fn("exp", lo)[0], _iv_fn("exp", hi)[1])
    if f == "log":
        return (_iv_fn("log", lo)[0], _iv_fn("log", hi)[1])
    if f == "log_abs":
        if lo > 0:
            return (_iv_fn("log", lo)[0], _iv_fn("log", hi)[1])
        return (_iv_fn("log", -hi)[0], _iv_fn("log", -lo)[1])
    if f in ("sqrt", "root"):
        k = 2 if f == "sqrt" else n
        return (_rt(lo, k)[0], _rt(hi, k)[1])
    if f in ("sin", "cos"):
        cands = [_iv_fn(f, t) for t in (lo, hi)]
        lows = [c[0] for c in cands]
        highs = [c[1] for c in cands]
        for v in _crit_values(f, lo, hi):
            lows.append(F(v))
            highs.append(F(v))
        return (min(lows), max(highs))
    raise ValueError(f)


def rf_tight(f, n, lo, hi):
    """1 if the radius of the result must be within a factor 4 of the true width (plus rounding), else 0.
    exp: r <= 1. log, log_abs, sqrt, root (odd, or lo > 0): mid != 0 and r <= |mid| / 2. sin, cos: r <= 1/16 and
    |f'(mid)| >= 3/4. Otherwise only the Lipschitz bound of the input radius is asserted for sin and cos."""
    mid, r = (lo + hi) / 2, (hi - lo) / 2
    if f == "exp":
        return int(r <= 1)
    if f in ("log", "sqrt", "root", "log_abs"):
        return int(mid != 0 and r <= abs(mid) / 2 and (f != "root" or n % 2 == 1 or lo > 0))
    from mpmath import iv
    iv.prec = IV_PREC
    x = _ivq(mid)
    d = _iv_bounds(iv.cos(x) if f == "sin" else -iv.sin(x))
    return int(r <= F(1, 16) and min(abs(d[0]), abs(d[1])) >= F(3, 4) and d[0] * d[1] > 0)


def rf_vector(f, n, prec, rb):
    """One vector of tests/ref/vectors/f-slice2/rfunc_real.jsonl."""
    lo, hi = rb.lo, rb.hi
    row = {"f": f, "n": n, "prec": prec, "x": rb.json(), "status": rf_status(f, n, lo, hi)}
    if row["status"] == "OK":
        a, b = rf_image(f, n, lo, hi)
        row["lo"], row["hi"] = dy_json_bound(a, False), dy_json_bound(b, True)
        row["tight"] = rf_tight(f, n, lo, hi)
    return row


# ---- partial balls: reference of the projection and of the componentwise operations ----

def sb_json(arch, rb, loc):
    return {"arch": arch, "inf": None if rb is None else rb.json(), "loc": [x.json() for x in loc]}


def sb_ref_project(rb, A, H, d, places):
    """S1: the projection of the adele (rb ; (A + H Zhat)/d) to the places ('inf' or a prime), any order.
    Returns ('DOMAIN', repeated place) or ('OK', arch, rb or None, [LB by increasing prime])."""
    seen = set()
    for v in places:
        if v in seen:
            return ("DOMAIN", v)
        seen.add(v)
    primes = sorted(v for v in places if v != "inf")
    return ("OK", 1 if "inf" in places else 0, rb if "inf" in places else None,
            [lb_ref_project(p, A, H, d) for p in primes])


def sb_real_op(op, x, y=None):
    """The exact set of results of the real operation on the intervals: (lo, hi) as Fractions."""
    if op == "neg":
        return (-x.hi, -x.lo)
    if op == "add":
        return (x.lo + y.lo, x.hi + y.hi)
    if op == "sub":
        return (x.lo - y.hi, x.hi - y.lo)
    ps = [x.lo * y.lo, x.lo * y.hi, x.hi * y.lo, x.hi * y.hi]
    return (min(ps), max(ps))


def sb_ref_op(op, X, Y=None):
    """X, Y = (arch, RB or None, [LB]) over the same places. Returns (arch, (lo, hi) or None, [LB])."""
    fn = {"neg": lb_ref_neg, "add": lb_ref_add, "sub": lb_ref_sub, "mul": lb_ref_mul}[op]
    loc = [fn(a) if Y is None else fn(a, b) for a, b in zip(X[2], X[2] if Y is None else Y[2])]
    real = None if X[1] is None else sb_real_op(op, X[1], None if Y is None else Y[1])
    return (X[0], real, loc)


def check_sball_projection_enumeration():
    """S1: for random adeles (A + H Zhat)/d and primes, every point A/d + H z/d, z in [0, p^2), is inside the
    component at p and the p classes modulo the next digit are all met (the component is the smallest)."""
    rng = random.Random(21)
    n = 0
    for _ in range(300):
        d = rng.randint(1, 60)
        H = rng.choice((0, rng.randint(1, 500)))
        A = rng.randint(-200, 200)
        for p in (2, 3, 5, 7):
            c = lb_ref_project(p, A, H, d)
            pts = [F(A, d) + F(H, d) * z for z in range(p * p)]
            if H == 0:
                assert c.exact and all(lb_val(c) == t for t in pts)
                continue
            assert lb_is_canonical(c)
            for t in pts:
                assert lb_ref_contains(lb_exact(p, t), c)
            classes = {lb_ball(p, t, c.N + 1).key() for t in pts}
            assert len(classes) == p, (p, A, H, d)
            n += 1
    return f"cases={n}"


def check_sball_tuple_enumeration():
    """S3: over the places {inf, 2, 3, 5}, random partial balls X, Y, random tuples of points of each, and the
    result tuple of add, sub, mul: every coordinate of the result tuple lies in the result component."""
    rng = random.Random(22)
    primes = (2, 3, 5)
    n = 0

    def rnd():
        loc = []
        for p in primes:
            k = rng.choice((1, 2, 3))
            v = rng.randint(-2, 2)
            u = rng.randrange(1, p ** k)
            while u % p == 0:
                u = rng.randrange(1, p ** k)
            loc.append(rng.choice((LB(p, 0, u, v, v + k),
                                   lb_exact(p, F(rng.randint(-9, 9), rng.randint(1, 9)) * F(p) ** v))))
        return (1, RB(rng.randint(-50, 50), rng.randint(-3, 2), rng.randint(0, 40), rng.randint(-6, 0)), loc)

    for _ in range(400):
        X, Y = rnd(), rnd()
        for op in ("add", "sub", "mul"):
            arch, real, loc = sb_ref_op(op, X, Y)
            for _ in range(6):
                fx = (X[1].lo, X[1].hi, X[1].mid)[rng.randrange(3)]
                fy = (Y[1].lo, Y[1].hi, Y[1].mid)[rng.randrange(3)]
                r = {"add": fx + fy, "sub": fx - fy, "mul": fx * fy}[op]
                assert real[0] <= r <= real[1]
                for a, b, c in zip(X[2], Y[2], loc):
                    pa = rng.choice(lb_points(a))
                    pb = rng.choice(lb_points(b))
                    r = {"add": pa + pb, "sub": pa - pb, "mul": pa * pb}[op]
                    assert lb_ref_contains(lb_exact(a.p, r), c), (op, a, b, c)
                    n += 1
    return f"tuple_points={n}"


def check_real_reference_against_arb():
    """rf_image (mpmath intervals, exact integer roots) against python-flint's arb at 300 bits, at the two end points
    of the ball: the image bounds [minlo, maxhi] must satisfy minlo <= f(t) <= maxhi for t = lo and t = hi, with f(t)
    an arb ball (its own radius allowed): the arb ball must not lie wholly above maxhi or below minlo. Two enclosures
    of the same numbers; a bound that is too small, or too large by more than the width of the ball's image, fails."""
    import flint
    rng = random.Random(23)
    n = 0
    flint.ctx.prec = 300

    def arb_at(f, k, q):
        t = flint.arb(q.numerator) / q.denominator
        return {"exp": t.exp, "log": t.log, "sin": t.sin, "cos": t.cos, "sqrt": t.sqrt}.get(f, lambda: t.root(k))()

    def bounds(r):
        (m1, e1), (m2, e2) = (tuple(int(t) for t in r.mid().man_exp()), tuple(int(t) for t in r.rad().man_exp()))
        return dy(m1, e1) - dy(m2, e2), dy(m1, e1) + dy(m2, e2)

    for _ in range(200):
        f = rng.choice(("exp", "log", "sin", "cos", "sqrt", "root"))
        k = rng.choice((1, 3, 5, 7)) if f == "root" else 0
        mid = dy(rng.randint(1, 2 ** 20), rng.randint(-25, 5))
        rad = mid / rng.choice((4, 8, 100)) if f in ("log", "sqrt", "root") else dy(rng.randint(0, 100), -12)
        lo, hi = mid - rad, mid + rad
        try:
            a, b = rf_image(f, k, lo, hi)
        except ValueError:
            continue
        ends = [bounds(arb_at(f, k, t)) for t in (lo, hi)]
        # f(lo) and f(hi) lie in the image: minlo <= min f(t) and max f(t) <= maxhi; the arb balls overlap [a, b]
        # and the smallest of the upper ends is >= a, the largest of the lower ends is <= b
        assert min(e[1] for e in ends) >= a and max(e[0] for e in ends) <= b, (f, k, lo, hi)
        # the image is not much wider than the values at the end points and the critical values: for the monotone
        # functions, a is at most the smaller and b at least the larger end value, and they are close to them
        if f in ("exp", "log", "sqrt", "root"):
            assert a <= min(e[1] for e in ends) and b >= max(e[0] for e in ends)
            assert a >= min(e[0] for e in ends) - F(1, 2 ** 250) * (1 + abs(a)), (f, k, lo, hi)
            assert b <= max(e[1] for e in ends) + F(1, 2 ** 250) * (1 + abs(b)), (f, k, lo, hi)
        n += 1
    return f"cases={n}"


def sball_main():
    for check in (check_sball_projection_enumeration, check_sball_tuple_enumeration,
                  check_real_reference_against_arb):
        print(f"{check.__name__}: {check()}", flush=True)


if __name__ == "__main__":
    main()
    lball_main()
    sball_main()
