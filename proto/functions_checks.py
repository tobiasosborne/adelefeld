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


# ======================================================================================================
# Part 2. Reference and oracles of milestone 1F, work packages 1F.1 to 1F.4 (docs/api-1f.md).
#
# Added by the lane d-functions on 2026-09-29. Everything above this line is the check program of
# docs/proofs/functions.md and is unchanged.
#
# The functions ref_* are the reference algorithms: each states one rule of the design (status, set,
# canonical form). The functions check_1f_* compare them with oracles that do not use the rule:
# enumeration of the points of a ball on a grid, the literal truncations of Proposition 7, the
# decomposition of Part 1 (found by search), and values quoted from sources on disk.
# The status LIMIT (sizes, overflow of an exponent) is not modelled: Python integers have no bound.
# ======================================================================================================

ST = {"OK": 0, "NOT_DETERMINED": 1, "UNIT_NOT_CERTIFIED": 2, "NEEDS_SPLIT": 3, "NOT_UNIQUE": 4,
      "NO_SOLUTION": 5, "NOT_UNIT": 6, "DOMAIN": 7, "UNSUPPORTED": 8, "PARSE": 9, "LIMIT": 10}
P1F = (2, 3, 5, 13)
PLACE_INF = "inf"


def lb_centre(a, p, expo):
    """The element of Z[1/p] in [0, p^expo) that lies in a + p^expo Z_p (conventions 5.8)."""
    a = F(a)
    if vp(a, p) >= expo:
        return F(0)
    m = int(vp(a, p))
    unit = a/F(p)**m
    q = p**(expo-m)
    return F(p)**m*(unit.numerator*pow(unit.denominator, -1, q) % q)


class LBall:
    """A value of adf_lball. expo None: the exact rational cen. Else the ball cen + p^expo Z_p."""

    def __init__(self, p, cen, expo=None):
        self.p, self.expo = p, expo
        self.cen = F(cen) if expo is None else lb_centre(cen, p, expo)

    @property
    def exact(self):
        return self.expo is None

    def key(self):
        return (self.p, self.cen, self.expo)

    def __eq__(self, other):
        return isinstance(other, LBall) and self.key() == other.key()

    def __hash__(self):
        return hash(self.key())

    def __repr__(self):
        body = str(self.cen) if self.exact else f"{self.cen} + O({self.p}^{self.expo})"
        return f"[p={self.p}: {body}]"

    def val(self):
        return vp(self.cen, self.p)

    def has(self, z):
        z = F(z)
        return z == self.cen if self.exact else vp(z-self.cen, self.p) >= self.expo

    def grid(self, depth):
        if self.exact:
            return [self.cen]
        return [self.cen+F(self.p)**self.expo*j for j in range(self.p**depth)]

    def fields(self):
        """(p, u, v, N, exact) as in the struct of conventions 5.8."""
        v = 0 if self.cen == 0 else int(self.val())
        return (self.p, self.cen/F(self.p)**v, v, 0 if self.exact else self.expo, int(self.exact))


def lb_is_canonical(p, u, v, n, exact):
    """The predicate of conventions 5.8, written from its text and not from the class above."""
    u = F(u)
    if exact not in (0, 1) or p < 2 or any(p % k == 0 for k in range(2, p) if k*k <= p):
        return False
    if u == 0:
        return v == 0 and (n == 0 or not exact)
    if u.numerator % p == 0 or u.denominator % p == 0:
        return False
    if exact:
        return n == 0
    return u.denominator == 1 and v < n and 0 < u < p**(n-v)


# ---- 1F.1: projection of a finite ball and of an adele ----

def ref_project_fball(big_a, big_h, d, p):
    """(A + H Zhat)/d at the prime p: the rational A/d if H = 0, else A/d + p^(v_p(H)-v_p(d)) Z_p."""
    if big_h == 0:
        return LBall(p, F(big_a, d))
    return LBall(p, F(big_a, d), int(vp(big_h, p)-vp(d, p)))


def place_key(v):
    return (0, 0) if v == PLACE_INF else (1, v)


def ref_project_adele(real, fin, places):
    """The partial ball over `places` of the adele (real ; fin): a dict from the place to its ball."""
    return {v: (real if v == PLACE_INF else ref_project_fball(*fin, v)) for v in places}


# ---- 1F.3: arithmetic, predicates, typed values, decomposition ----

def ref_add(x, y, sign=1):
    if x.p != y.p:
        return "DOMAIN", None
    expos = [t.expo for t in (x, y) if not t.exact]
    return "OK", LBall(x.p, x.cen+sign*y.cen, min(expos) if expos else None)


def ref_neg(x):
    return LBall(x.p, -x.cen, x.expo)


def ref_mul(x, y):
    if x.p != y.p:
        return "DOMAIN", None
    if (x.exact and x.cen == 0) or (y.exact and y.cen == 0):
        return "OK", LBall(x.p, 0)
    if x.exact and y.exact:
        return "OK", LBall(x.p, x.cen*y.cen)
    nx = inf if x.exact else x.expo
    ny = inf if y.exact else y.expo
    return "OK", LBall(x.p, x.cen*y.cen, int(min(x.val()+ny, y.val()+nx, nx+ny)))


def ref_inv(x):
    if x.cen == 0:
        return ("NOT_UNIT" if x.exact else "UNIT_NOT_CERTIFIED"), None
    if x.exact:
        return "OK", LBall(x.p, 1/x.cen)
    return "OK", LBall(x.p, 1/x.cen, x.expo-2*int(x.val()))


def ref_contains(x, y):
    """1 if the set x lies in the set y."""
    if x.p != y.p:
        return False
    if y.exact:
        return x.exact and x.cen == y.cen
    if not x.exact and x.expo < y.expo:
        return False
    return vp(x.cen-y.cen, x.p) >= y.expo


def ref_overlaps(x, y):
    return ref_contains(x, y) or ref_contains(y, x)


def ref_equal_set(x, y):
    return x == y


def ref_valuation(x):
    if x.cen == 0:
        return ("OK", inf) if x.exact else ("NOT_DETERMINED", None)
    return "OK", int(x.val())


def ref_abs(x):
    status, m = ref_valuation(x)
    if status != "OK":
        return status, None
    return "OK", F(0) if m == inf else F(x.p)**(-m)


def ref_frac(x):
    if not x.exact and x.expo < 0:
        return "NOT_DETERMINED", None
    return "OK", lb_centre(x.cen, x.p, 0)


def teich_power(r, p, k):
    """The Teichmueller representative of the unit residue r modulo p^k, as r^(p^k); 1 at p = 2."""
    if p == 2:
        return 1
    q = p**k
    t = r % q
    for _ in range(k):
        t = pow(t, p, q)
    return t


def unit_residue(unit, modulus):
    return unit.numerator*pow(unit.denominator, -1, modulus) % modulus


def ref_decompose(x, prec_p):
    """x = p^m w u (Proposition 4). Returns (status, (m, wres, w, u)); w and u are LBall values.

    wres is the residue of x/p^m modulo p (modulo 4 at p = 2): it names the root of unity w.
    """
    p, c = x.p, cdisc(x.p)
    if x.cen == 0:
        return ("DOMAIN" if x.exact else "NOT_DETERMINED"), None
    m = int(x.val())
    unit = x.cen/F(p)**m
    if not x.exact and x.expo-m < c:
        return "NOT_DETERMINED", None
    wres = unit_residue(unit, 4 if p == 2 else p)
    sign = 1 if wres == 1 else -1 if wres == (3 if p == 2 else p-1) else 0
    if sign:
        w = LBall(p, sign)
        u = LBall(p, sign*unit, None if x.exact else x.expo-m)
        return "OK", (m, wres, w, u)
    w = LBall(p, teich_power(wres, p, max(prec_p, 1)), prec_p)
    uexpo = prec_p if x.exact else x.expo-m
    k = max(uexpo, 1)
    ucen = unit_residue(unit, p**k)*pow(teich_power(wres, p, k), -1, p**k)
    return "OK", (m, wres, w, LBall(p, ucen, uexpo))


# ---- 1F.4: exp, the series log, the Iwasawa Log ----

def series_centre(fn, z, p, expo):
    """The series value at the rational z modulo p^expo; 0 for expo <= 0, since the value is integral."""
    return F(series(fn, z, p, expo)) if expo >= 1 else F(0)


def ref_exp(x, prec_p):
    p, c = x.p, cdisc(x.p)
    if x.exact:
        if x.cen == 0:
            return "OK", LBall(p, 1)
        if x.val() < c:
            return "DOMAIN", None
        return "OK", LBall(p, series_centre("exp", x.cen, p, prec_p), prec_p)
    if x.cen == 0:
        if x.expo < c:
            return "NOT_DETERMINED", None
        return "OK", LBall(p, 1, x.expo)
    if x.val() < c:
        return "DOMAIN", None
    return "OK", LBall(p, series_centre("exp", x.cen, p, x.expo), x.expo)


def ref_log(x, prec_p):
    p, c = x.p, cdisc(x.p)
    if x.exact:
        if x.cen == 1:
            return "OK", LBall(p, 0)
        if vp(x.cen-1, p) < 1:
            return "DOMAIN", None
        return "OK", LBall(p, series_centre("log", x.cen, p, prec_p), prec_p)
    if x.expo < 1:
        return ("NOT_DETERMINED" if x.cen == 0 else "DOMAIN"), None
    if vp(x.cen-1, p) < 1:
        return "DOMAIN", None
    expo = max(x.expo, c)
    return "OK", LBall(p, series_centre("log", x.cen, p, expo), expo)


def ref_ilog(x, prec_p):
    """The Iwasawa Log, computed without a Teichmueller lift: Log x = log((x/p^m)^(p-1))/(p-1)."""
    p, c = x.p, cdisc(x.p)
    if x.cen == 0:
        return ("DOMAIN" if x.exact else "NOT_DETERMINED"), None
    m = int(x.val())
    unit = x.cen/F(p)**m
    if x.exact and unit in (1, -1):
        return "OK", LBall(p, 0)
    expo = prec_p if x.exact else max(x.expo-m, c)
    if expo < 1:
        return "OK", LBall(p, 0, expo)
    if p == 2:
        t = unit if unit_residue(unit, 4) == 1 else -unit
        return "OK", LBall(p, series("log", t, p, expo), expo)
    value = series("log", unit**(p-1), p, expo)*pow(p-1, -1, p**expo)
    return "OK", LBall(p, value, expo)


# ---- 1F.2: statuses at the real place, and the real root of odd and even degree ----

REAL_ENTIRE = ("exp", "sin", "cos", "sinh", "cosh", "atan", "tanh")


def real_in_domain(fn, x, degree=2):
    """Whether the real number x is in the domain of fn (the definition, used as the oracle)."""
    if fn in REAL_ENTIRE or (fn == "root" and degree % 2 == 1):
        return True
    if fn in ("log", "lgamma"):
        return x > 0
    if fn in ("log_abs", "inv"):
        return x != 0
    if fn == "root":
        return x >= 0
    if fn == "gamma":
        return not (x <= 0 and x.denominator == 1)
    assert fn == "zeta"
    return x != 1


def ref_real_status(fn, lo, hi, degree=2):
    """The status of fn on the closed real interval [lo, hi], lo <= hi rationals."""
    assert lo <= hi
    if fn in REAL_ENTIRE or (fn == "root" and degree % 2 == 1):
        return "OK"
    if fn in ("log", "lgamma"):
        return "OK" if lo > 0 else "DOMAIN" if hi <= 0 else "NOT_DETERMINED"
    if fn == "root":
        return "OK" if lo >= 0 else "DOMAIN" if hi < 0 else "NOT_DETERMINED"
    if fn in ("log_abs", "inv"):
        if lo > 0 or hi < 0:
            return "OK"
        if lo == hi:
            return "NOT_UNIT" if fn == "inv" else "DOMAIN"
        return "UNIT_NOT_CERTIFIED" if fn == "inv" else "NOT_DETERMINED"
    if fn == "gamma":
        first = -((-lo).numerator // (-lo).denominator)   # the least integer >= lo
        if first > hi or first > 0:
            return "OK"
        return "DOMAIN" if lo == hi else "NOT_DETERMINED"
    assert fn == "zeta"
    if not lo <= 1 <= hi:
        return "OK"
    return "DOMAIN" if lo == hi else "NOT_DETERMINED"


def iroot(n, k):
    """floor(n^(1/k)) for integers n >= 0, k >= 1, by bisection on integers."""
    assert n >= 0 and k >= 1
    lo, hi = 0, 1
    while hi**k <= n:
        hi *= 2
    while hi-lo > 1:
        mid = (lo+hi)//2
        if mid**k <= n:
            lo = mid
        else:
            hi = mid
    return lo


def root_bounds(x, k, bits):
    """Rationals (below, above) with below <= x^(1/k) <= above, above - below <= 2^-bits / den(x)."""
    x = F(x)
    if x < 0:
        assert k % 2 == 1
        below, above = root_bounds(-x, k, bits)
        return -above, -below
    scale = x.denominator*2**bits
    r = iroot(x.numerator*x.denominator**(k-1)*2**(bits*k), k)
    exact = r**k == x.numerator*x.denominator**(k-1)*2**(bits*k)
    return F(r, scale), F(r if exact else r+1, scale)


def ref_real_root(lo, hi, k, bits):
    """An enclosure [below, above] of the k-th roots of [lo, hi]; the non-negative root for even k."""
    status = ref_real_status("root", lo, hi, k)
    if status != "OK":
        return status, None
    return "OK", (root_bounds(lo, k, bits)[0], root_bounds(hi, k, bits)[1])


# ---- 1F.1: f at the places of a partial ball ----

LOCAL_RULE = {"exp": lambda x, n: ref_exp(x, n), "log": lambda x, n: ref_log(x, n),
              "ilog": lambda x, n: ref_ilog(x, n), "inv": lambda x, n: ref_inv(x)}


def ref_real(fn, ball):
    status = ref_real_status("log" if fn == "ilog" else fn, *ball)
    return status, (("real", fn)+tuple(ball) if status == "OK" else None)


def combine(statuses):
    """conventions 3.3: the maximum, reported at the first place in canonical order that has it."""
    if not statuses:
        return "OK", None
    top = max(statuses.values(), key=ST.get)
    if top == "OK":
        return "OK", None
    return top, min((v for v in statuses if statuses[v] == top), key=place_key)


def ref_sball_apply(fn, sball, prec_p):
    """fn at every place of a partial ball. Returns (status, where, value or None, statuses, partial)."""
    statuses, values = {}, {}
    for v in sorted(sball, key=place_key):
        rule = ref_real(fn, sball[v]) if v == PLACE_INF else LOCAL_RULE[fn](sball[v], prec_p)
        statuses[v], values[v] = rule
    status, where = combine(statuses)
    partial_value = {v: values[v] for v in values if statuses[v] == "OK"}
    return status, where, (values if status == "OK" else None), statuses, partial_value


def ref_f_at(fn, real, fin, places, prec_p):
    return ref_sball_apply(fn, ref_project_adele(real, fin, places), prec_p)


# ---- oracles ----

def oracle_in_domain(fn, z, p):
    z = F(z)
    if fn == "exp":
        return z == 0 or vp(z, p) >= cdisc(p)
    if fn == "log":
        return vp(z-1, p) >= 1
    return z != 0


def oracle_status(fn, x):
    """The status of a local function from the points of the ball: all inside, none inside, or mixed."""
    depth = 2 if x.exact else 2+max(0, -x.expo)
    inside = {oracle_in_domain(fn, z, x.p) for z in x.grid(depth)}
    if inside == {True}:
        return "OK"
    if fn == "inv":
        return "NOT_UNIT" if inside == {False} else "UNIT_NOT_CERTIFIED"
    return "DOMAIN" if inside == {False} else "NOT_DETERMINED"


def oracle_value(fn, z, p, n):
    """The value at the rational z modulo p^n, n >= 1, by another route than the reference."""
    z = F(z)
    if fn == "inv":
        m = int(vp(z, p))
        return F(p)**(-m)*residue(F(p)**m/z, p, n+m) if n+m >= 1 else F(0)
    if fn == "ilog":
        return F(iwasawa(z, p, n))
    if fn == "log":
        if z == 1:
            return F(0)
        if p == 2 and residue(z, 2, 2) == 3:
            z = -z                      # log(-z) = log(z) at 2 (Proposition 11), then v(z-1) >= 2
        count = cutoffs(p, int(vp(z-1, p)), n)["log"]
        return F(residue(partial("log", z-1, count), p, n))
    assert fn == "exp"
    if z == 0:
        return F(1)
    return F(residue(partial("exp", z, cutoffs(p, int(vp(z, p)), n)["exp"]), p, n))


def image_check(fn, x, y, extra=2):
    """y must contain the image of the grid of x, and the images must meet every class of y one digit down."""
    p = x.p
    assert lb_is_canonical(*y.fields()), (fn, x, y)
    if y.exact:
        for z in x.grid(2):
            if fn == "inv":
                assert z*y.cen == 1, (x, y, z)
            else:
                assert oracle_value(fn, z, p, 6) == F(residue(y.cen, p, 6)), (fn, x, y, z)
        assert x.exact, (fn, x, y)
        return 1
    n = y.expo+extra
    depth = 3 if p == 2 else 2
    classes = set()
    for z in x.grid(depth):
        if n >= 1:
            value = oracle_value(fn, z, p, n)
            assert vp(value-y.cen, p) >= y.expo, (fn, x, y, z, value)
            classes.add(residue((value-y.cen)/F(p)**y.expo, p, 1) if value != y.cen else 0)
    if not x.exact and n >= 1:
        assert classes == set(range(p)), (fn, x, y, classes)
    return len(x.grid(depth))


def sample_balls(p, exact_too=True):
    """Balls and exact values at p: centres of valuation -2 to 3 and zero, exponents -2 to 4."""
    centres = [F(0), F(1), F(-1), F(3), F(5), F(p), F(2*p), F(p+1), F(1-p), F(p*p), F(3*p*p), F(p)**3,
               F(1, p), F(3, p*p), F(1, p+1), F(7, 3 if p != 3 else 5), F(p*p, p+2), F(4), F(-4), F(12)]
    balls = []
    for a in centres:
        if exact_too:
            balls.append(LBall(p, a))
        for expo in range(-2, 5):
            balls.append(LBall(p, a, expo))
    return list(dict.fromkeys(balls))


# ---- checks ----

def check_1f_canonical():
    forms = equal = different = 0
    for p in P1F:
        for x in sample_balls(p):
            assert lb_is_canonical(*x.fields()), x
            _, u, v, n, exact = x.fields()
            assert F(p)**v*u == x.cen and (exact or 0 <= x.cen < F(p)**n)
            forms += 1
            if x.exact:
                continue
            for t in (F(1), F(-1), F(1, p+1), F(7), F(-p*p)):
                assert LBall(p, x.cen+F(p)**x.expo*t, x.expo) == x, (x, t)
                equal += 1
            assert LBall(p, x.cen+F(p)**(x.expo-1), x.expo) != x
            assert LBall(p, x.cen, x.expo+1) != x and LBall(p, x.cen) != x
            different += 3
    return f"canonical_forms={forms} same_set_same_form={equal} different_sets_different_form={different}"


def check_1f_projection():
    balls = points = classes = 0
    for p in (2, 3, 5):
        for big_h in (0, 1, 2, 3, 4, 6, 9, 12, 45, 50):
            for d in (1, 2, 3, 4, 6, 9, 10):
                for big_a in (0, 1, 3, 5, 7, 10):
                    y = ref_project_fball(big_a, big_h, d, p)
                    assert lb_is_canonical(*y.fields())
                    balls += 1
                    if big_h == 0:
                        assert y.exact and y.cen == F(big_a, d)
                        continue
                    seen = set()
                    for t in range(-3, p*p):
                        z = F(big_a+big_h*t, d)
                        assert y.has(z), (p, big_a, big_h, d, t)
                        points += 1
                        if 0 <= t < p*p:
                            seen.add(residue((z-y.cen)/F(p)**y.expo, p, 2))
                    assert seen == set(range(p*p)), (p, big_a, big_h, d)
                    classes += len(seen)
    return f"balls={balls} rational_points_inside={points} classes_two_digits_down_met={classes}"


def check_1f_arith():
    sums = products = inverses = statuses = 0
    for p in (2, 3, 5):
        balls = [x for x in sample_balls(p) if x.exact or -1 <= x.expo <= 3]
        balls = balls[::3]
        for x in balls:
            status, y = ref_inv(x)
            assert status == oracle_status("inv", x), (x, status)
            statuses += 1
            if status == "OK":
                inverses += image_check("inv", x, y)
            assert ref_neg(ref_neg(x)) == x and all(ref_neg(x).has(-z) for z in x.grid(1))
            for y in balls:
                for sign, (status, z) in ((1, ref_add(x, y)), (-1, ref_add(x, y, -1)), (0, ref_mul(x, y))):
                    assert status == "OK" and lb_is_canonical(*z.fields())
                    if z.exact:
                        assert x.exact and y.exact or sign == 0 and 0 in (x.cen, y.cen)
                    seen = set()
                    for s in x.grid(1):
                        for t in y.grid(1):
                            value = s*t if sign == 0 else s+sign*t
                            assert z.has(value), (x, y, sign, z)
                            if not z.exact:
                                quotient = (value-z.cen)/F(p)**z.expo
                                seen.add(residue(quotient, p, 1) if quotient else 0)
                    assert z.exact or seen == set(range(p)), (x, y, sign, z, seen)
                    if sign:
                        sums += 1
                    else:
                        products += 1
        assert ref_add(LBall(2, 1, 3), LBall(3, 1, 3))[0] == "DOMAIN"
        assert ref_mul(LBall(2, 1, 3), LBall(3, 1, 3))[0] == "DOMAIN"
    return (f"sums_and_differences={sums} products={products} inverse_grid_points={inverses} "
            f"inverse_statuses={statuses}")


def check_1f_predicates():
    pairs = 0
    for p in (2, 3):
        balls = [x for x in sample_balls(p) if x.exact or -1 <= x.expo <= 2]
        for x in balls:
            for y in balls:
                inside = [y.has(z) for z in x.grid(2)]
                assert ref_contains(x, y) == all(inside), (x, y)
                common = any(inside) or any(x.has(z) for z in y.grid(2))
                assert ref_overlaps(x, y) == common, (x, y)
                assert ref_equal_set(x, y) == (ref_contains(x, y) and ref_contains(y, x)), (x, y)
                pairs += 1
    assert not ref_contains(LBall(2, 1, 3), LBall(3, 1, 3))
    return f"pairs={pairs}"


def check_1f_typed():
    values = fractions = parts = undetermined = 0
    for p in P1F:
        for x in sample_balls(p):
            points = x.grid(2)
            vals = {vp(z, p) for z in points}
            status, m = ref_valuation(x)
            assert (status == "OK") == (len(vals) == 1), (x, vals)
            if status == "OK":
                assert vals == {m}
                assert ref_abs(x) == ("OK", F(0) if m == inf else F(p)**(-m))
            else:
                assert ref_abs(x)[0] == "NOT_DETERMINED"
                undetermined += 1
            values += 1
            fracs = {fractional_part(z, p) for z in x.grid(3)}
            status, r = ref_frac(x)
            assert (status == "OK") == (len(fracs) == 1), (x, fracs)
            if status == "OK":
                assert fracs == {r} and 0 <= r < 1
                rest = r.denominator
                while rest % p == 0:
                    rest //= p
                assert rest == 1
            fractions += 1
            for prec_p in (-1, 1, 4):
                status, parts_of_x = ref_decompose(x, prec_p)
                nonzero = [z for z in points if z]
                signs = {components(z, p, 4)[1] % (4 if p == 2 else p) for z in nonzero}
                if len(nonzero) < len(points):
                    assert status == ("DOMAIN" if x.exact else "NOT_DETERMINED"), (x, status)
                    continue
                assert (status == "OK") == (len(signs) == 1), (x, signs)
                if status != "OK":
                    undetermined += 1
                    continue
                m, wres, w, u = parts_of_x
                assert lb_is_canonical(*w.fields()) and lb_is_canonical(*u.fields())
                assert {wres} == signs
                assert w.exact == (wres in (1, 3 if p == 2 else p-1))
                assert w.exact or w.expo == prec_p
                assert u.exact == (x.exact and w.exact)
                assert u.exact or u.expo == (prec_p if x.exact else x.expo-m)
                k = max(5, prec_p+1, 0 if u.exact else u.expo+2)
                seen = set()
                for z in nonzero:
                    mz, wz, uz = components(z, p, k)
                    assert mz == m
                    if w.exact:
                        assert (wz-w.cen) % p**k == 0, (x, z, w)
                    else:
                        assert vp(wz-w.cen, p) >= w.expo, (x, z, w)
                    if u.exact:
                        assert residue(u.cen, p, k) == uz % p**k, (x, z, u)
                    else:
                        assert vp(uz-u.cen, p) >= u.expo, (x, z, u)
                        seen.add(residue((uz-u.cen)/F(p)**u.expo, p, 1))
                if not u.exact and not x.exact:
                    assert seen == set(range(p)), (x, u, seen)
                parts += 1
    return f"valuations={values} fractional_parts={fractions} decompositions={parts} undetermined={undetermined}"


def check_1f_teichmuller_sources():
    """Values quoted from sources on disk and from the probe of FLINT (lanes/d-functions/probe)."""
    conrad = 2+5+2*5**2+5**3+3*5**4+4*5**5+2*5**6+3*5**7        # conrad-hensel:hensel.txt:183
    pari = 2+83*101+18*101**2+69*101**3+62*101**4                 # pari-doc:usersch3.tex:16497
    assert teich_power(2, 5, 8) == conrad == 280182               # 280182: padic_teichmuller, probe
    assert teich_power(2, 101, 5) == pari == 6523027634           # 6523027634: padic_teichmuller, probe
    assert teich(2, 5, 8) == conrad and teich(2, 101, 5) == pari
    assert pow(conrad, 2, 5**8) == 5**8-1
    status, (m, wres, w, u) = ref_decompose(LBall(5, 50), 8)
    assert (status, m, wres, w) == ("OK", 2, 2, LBall(5, conrad, 8))
    assert ref_decompose(LBall(2, 3, 8), 8)[1][2] == LBall(2, -1)
    return "quoted_values=2 probe_values=2 square_is_minus_one=1"


def check_1f_exp_log():
    statuses = images = exact_inputs = 0
    by_status = {}
    for p in P1F:
        for x in sample_balls(p):
            if not x.exact and x.expo > 3 and p == 13:
                continue
            for fn in ("exp", "log", "ilog"):
                for prec_p in ((3,) if not x.exact else (-1, 0, 1, 3, 5)):
                    status, y = LOCAL_RULE[fn](x, prec_p)
                    assert status == oracle_status(fn, x), (fn, x, status, oracle_status(fn, x))
                    by_status[status] = by_status.get(status, 0)+1
                    statuses += 1
                    if status != "OK":
                        assert y is None
                        continue
                    images += image_check(fn, x, y)
                    if x.exact:
                        assert y.exact or y.expo == prec_p, (fn, x, prec_p, y)
                        exact_inputs += 1
    assert ref_log(LBall(2, 3, 1), 9)[1] == LBall(2, 0, 2) == ref_log(LBall(2, 1, 1), 9)[1]
    assert ref_ilog(LBall(2, 6, 2), 9)[1] == LBall(2, 0, 2)
    assert ref_log(LBall(2, -1), 9) == ("OK", LBall(2, 0)) == ref_ilog(LBall(2, -1), 9)
    assert ref_ilog(LBall(3, 3), 9) == ("OK", LBall(3, 0)) and ref_log(LBall(3, 3), 9)[0] == "DOMAIN"
    summary = " ".join(f"{k}={by_status[k]}" for k in sorted(by_status, key=ST.get))
    return f"statuses={statuses} ({summary}) image_grid_points={images} exact_inputs={exact_inputs}"


def check_1f_regression():
    """Review N4 and the precision cases of SPEC 9.3.2."""
    e3, e12 = oracle_value("exp", 3, 3, 8), oracle_value("exp", 12, 3, 8)
    assert (e3, e12) == (958, 5125) and vp(e3-e12, 3) == 2         # 958, 5125: padic_exp, probe
    status, y = ref_exp(LBall(3, 3, 2), 8)
    assert status == "OK" and y == LBall(3, 4, 2) and y.has(e3) and y.has(e12)
    assert ref_exp(LBall(3, 12, 2), 8)[1] == y
    wrong = LBall(3, e3, 8)                                         # what a centre evaluation returns
    assert not wrong.has(e12)
    cases = 3
    for p, a, b, loss in ((3, 3, 12, 1), (2, 2, 10, 1)):
        expo = int(vp(F(a-b), p))
        ball = LBall(p, a, expo)
        assert ball.has(b)
        status, y = ref_ilog(ball, 8)
        la, lb = oracle_value("ilog", a, p, 8), oracle_value("ilog", b, p, 8)
        assert status == "OK" and y.has(la) and y.has(lb)
        assert vp(la-lb, p) == expo-loss == y.expo, (p, a, b, y)
        cases += 1
    assert oracle_value("log", -1, 2, 16) == 0 and ref_log(LBall(2, -1, 5), 0)[1] == LBall(2, 0, 5)
    assert vp(oracle_value("log", 3, 2, 16), 2) == 2
    assert residue(partial("cos", 4, 30), 2, 4) == 9
    edge = {(2, 2, 2): "DOMAIN", (2, 0, 1): "NOT_DETERMINED", (2, 4, 3): "OK", (2, 0, 2): "OK",
            (3, 0, 0): "NOT_DETERMINED", (3, 0, 1): "OK", (3, 1, 1): "DOMAIN", (3, 3, 2): "OK"}
    for (p, a, expo), status in edge.items():
        assert ref_exp(LBall(p, a, expo), 8)[0] == status, (p, a, expo)
        cases += 1
    assert ref_exp(LBall(3, 0), 8) == ("OK", LBall(3, 1)) and ref_exp(LBall(3, 0, 4), 8)[1] == LBall(3, 1, 4)
    assert ref_ilog(LBall(3, 0, 4), 8)[0] == "NOT_DETERMINED" and ref_ilog(LBall(3, 0), 8)[0] == "DOMAIN"
    assert ref_log(LBall(3, 0, 0), 8)[0] == "NOT_DETERMINED" and ref_log(LBall(3, 0, 1), 8)[0] == "DOMAIN"
    return f"cases={cases+6}"


def check_1f_f_at():
    calls = failures = named = 0
    adeles = (((F(1), F(2)), (3, 45, 1)), ((F(-1), F(1)), (4, 36, 1)), ((F(0), F(0)), (1, 0, 1)),
              ((F(1, 2), F(1, 2)), (3, 0, 1)), ((F(2), F(3)), (12, 1350, 7)), ((F(-2), F(-1)), (1, 60, 3)))
    universe = (PLACE_INF, 2, 3, 5, 7)
    subsets = [[v for i, v in enumerate(universe) if mask >> i & 1] for mask in range(32)]
    for real, fin in adeles:
        for places in subsets:
            for fn in ("exp", "log", "ilog", "inv"):
                status, where, value, statuses, part = ref_f_at(fn, real, fin, reversed(places), 4)
                calls += 1
                own = {}
                for v in places:
                    if v == PLACE_INF:
                        kind = "log" if fn == "ilog" else fn
                        inside = {real_in_domain(kind, t) for t in (real[0], real[1], sum(real)/2)}
                        own[v] = ("OK" if inside == {True} else
                                  ("NOT_UNIT" if fn == "inv" else "DOMAIN") if inside == {False} else
                                  "UNIT_NOT_CERTIFIED" if fn == "inv" else "NOT_DETERMINED")
                    else:
                        own[v] = oracle_status(fn, ref_project_fball(*fin, v))
                assert statuses == own, (fn, real, fin, places, statuses, own)
                ranked = sorted(places, key=lambda v: (-ST[own[v]], place_key(v)))
                if not places or own[ranked[0]] == "OK":
                    assert status == "OK" and where is None and sorted(value, key=place_key) == places
                    named += 1
                else:
                    assert (status, where) == (own[ranked[0]], ranked[0]) and value is None
                    assert sorted(part, key=place_key) == [v for v in places if own[v] == "OK"]
                    failures += 1
    status, where, value, statuses, _ = ref_f_at("exp", (F(1), F(2)), (3, 45, 1), [2, 3, 5], 4)
    assert (status, where) == ("DOMAIN", 5) and statuses == {2: "NOT_DETERMINED", 3: "OK", 5: "DOMAIN"}
    return f"calls={calls} ok_with_all_places_named={named} failures_with_place={failures}"


def check_1f_real():
    intervals = roots = 0
    ends = (F(-3), F(-2), F(-3, 2), F(-1), F(-1, 2), F(0), F(1, 3), F(1), F(3, 2), F(2), F(7))
    for fn in REAL_ENTIRE+("log", "lgamma", "log_abs", "inv", "gamma", "zeta", "root"):
        for degree in ((2, 3, 4, 5) if fn == "root" else (2,)):
            for lo in ends:
                for hi in ends:
                    if lo > hi:
                        continue
                    grid = [lo+(hi-lo)*F(j, 24) for j in range(25)]
                    grid += [F(k) for k in range(-3, 8) if lo <= k <= hi]
                    inside = {real_in_domain(fn, t, degree) for t in grid}
                    expected = ("OK" if inside == {True} else
                                ("NOT_UNIT" if fn == "inv" else "DOMAIN") if inside == {False} else
                                "UNIT_NOT_CERTIFIED" if fn == "inv" else "NOT_DETERMINED")
                    assert ref_real_status(fn, lo, hi, degree) == expected, (fn, degree, lo, hi)
                    intervals += 1
                    if fn != "root" or expected != "OK":
                        continue
                    status, (below, above) = ref_real_root(lo, hi, degree, 40)
                    eps = F(1, 2**39)
                    for t in grid:
                        assert below**degree <= t <= above**degree, (degree, lo, hi, t)
                    if degree % 2 == 0:
                        assert below >= 0
                    assert (below+eps)**degree >= lo, (degree, lo, below)
                    assert above == 0 or (above > eps and (above-eps)**degree <= hi) or above < 0, (degree, hi)
                    assert above >= 0 or (above-eps)**degree <= hi
                    roots += 1
    assert ref_real_root(F(-8), F(-8), 3, 20)[1] == (F(-2), F(-2))
    assert ref_real_root(F(0), F(0), 3, 20)[1] == (F(0), F(0))
    below, above = ref_real_root(F(-1, 1024), F(1, 1024), 3, 40)[1]
    assert below < 0 < above and below**3 <= F(-1, 1024) and above**3 >= F(1, 1024)
    return f"status_intervals={intervals} root_enclosures={roots} probe_cases=3"


def check_1f_rootlist_lball():
    """The accessor that S-D14 promises: the root certificate (a, K) at p as the local ball a + p^K Z_p."""
    roots = cylinders = 0

    def value(coeffs, t, q):
        return sum(c*t**i for i, c in enumerate(coeffs)) % q

    # X^2 - 2 at 7, X^2 + X - 2 at 2, X^3 - X at 3, X^2 + 1 at 5: every root is simple modulo p.
    for p, coeffs, big_k in ((7, (-2, 0, 1), 3), (2, (-2, 1, 1), 5), (3, (0, -1, 0, 1), 2), (5, (1, 0, 1), 4)):
        q = p**big_k
        found = [a for a in range(q) if value(coeffs, a, q) == 0]
        deep = [a for a in range(q*p*p) if value(coeffs, a, q*p*p) == 0]
        assert len(found) == len(deep) == len(coeffs)-1
        for a in found:
            x = LBall(p, a, big_k)
            assert lb_is_canonical(*x.fields()) and x.cen == a
            _, u, v, n, exact = x.fields()
            assert (n, exact) == (big_k, 0) and p**v*u == a
            assert ref_project_fball(a, q, 1, p) == x
            for other in (2, 3, 5, 7, 11):
                if other != p:
                    assert ref_project_fball(a, q, 1, other) == LBall(other, 0, 0)
                    cylinders += 1
            assert sum(1 for b in deep if x.has(b)) == 1
            roots += 1
    assert LBall(3, 0, 2).fields() == (3, 0, 0, 2, 0)
    return f"root_balls={roots} cylinder_is_whole_Zq_elsewhere={cylinders}"


def check_1f_examples():
    """Every example of docs/api-1f.md, printed so that the document quotes computed values."""
    lines = [
        ("projection of (3 + 45 Zhat)/1 at 2, 3, 5, 7", [ref_project_fball(3, 45, 1, p) for p in (2, 3, 5, 7)]),
        ("projection of (12 + 1350 Zhat)/7 at 2, 3, 5, 7",
         [ref_project_fball(12, 1350, 7, p) for p in (2, 3, 5, 7)]),
        ("projection of (1 + 60 Zhat)/3 at 2, 3, 5", [ref_project_fball(1, 60, 3, p) for p in (2, 3, 5)]),
        ("projection of the exact 3/4 at 2", ref_project_fball(3, 0, 4, 2)),
        ("exp of [p=3: 3 + O(3^2)]", ref_exp(LBall(3, 3, 2), 8)),
        ("exp of [p=3: 12 + O(3^2)]", ref_exp(LBall(3, 12, 2), 8)),
        ("exp of the exact 3 at p = 3, prec_p = 8", ref_exp(LBall(3, 3), 8)),
        ("exp of the exact 12 at p = 3, prec_p = 8", ref_exp(LBall(3, 12), 8)),
        ("exp of [p=2: 2 + O(2^2)]", ref_exp(LBall(2, 2, 2), 8)),
        ("exp of [p=2: 0 + O(2^1)]", ref_exp(LBall(2, 0, 1), 8)),
        ("exp of [p=2: 4 + O(2^3)]", ref_exp(LBall(2, 4, 3), 8)),
        ("exp of [p=3: 0 + O(3^4)]", ref_exp(LBall(3, 0, 4), 8)),
        ("exp of the exact 3/2 at p = 3, prec_p = 4", ref_exp(LBall(3, F(3, 2)), 4)),
        ("exp of the exact 3 at p = 3, prec_p = 0", ref_exp(LBall(3, 3), 0)),
        ("log of [p=2: 3 + O(2^1)]", ref_log(LBall(2, 3, 1), 8)),
        ("log of [p=2: 3 + O(2^4)]", ref_log(LBall(2, 3, 4), 8)),
        ("log of the exact -1 at p = 2", ref_log(LBall(2, -1), 8)),
        ("log of the exact 3 at p = 2, prec_p = 8", ref_log(LBall(2, 3), 8)),
        ("log of [p=3: 4 + O(3^3)]", ref_log(LBall(3, 4, 3), 8)),
        ("log of [p=3: 2 + O(3^3)]", ref_log(LBall(3, 2, 3), 8)),
        ("log of [p=3: 0 + O(3^0)]", ref_log(LBall(3, 0, 0), 8)),
        ("Log of [p=3: 3 + O(3^2)]", ref_ilog(LBall(3, 3, 2), 8)),
        ("Log of [p=3: 12 + O(3^4)]", ref_ilog(LBall(3, 12, 4), 8)),
        ("Log of [p=2: 2 + O(2^3)]", ref_ilog(LBall(2, 2, 3), 8)),
        ("Log of [p=2: 10 + O(2^5)]", ref_ilog(LBall(2, 10, 5), 8)),
        ("Log of [p=2: 6 + O(2^2)]", ref_ilog(LBall(2, 6, 2), 8)),
        ("Log of [p=5: 1/25 + O(5^0)]", ref_ilog(LBall(5, F(1, 25), 0), 8)),
        ("Log of the exact 3 at p = 3", ref_ilog(LBall(3, 3), 8)),
        ("Log of the exact 12 at p = 3, prec_p = 8", ref_ilog(LBall(3, 12), 8)),
        ("Log of [p=3: 0 + O(3^4)]", ref_ilog(LBall(3, 0, 4), 8)),
        ("[p=3: 1 + O(3^2)] * [p=3: 3 + O(3^3)]", ref_mul(LBall(3, 1, 2), LBall(3, 3, 3))),
        ("[p=3: 0 + O(3^2)] * [p=3: 0 + O(3^3)]", ref_mul(LBall(3, 0, 2), LBall(3, 0, 3))),
        ("exact 1/3 * [p=3: 5 + O(3^2)]", ref_mul(LBall(3, F(1, 3)), LBall(3, 5, 2))),
        ("inverse of [p=3: 6 + O(3^3)]", ref_inv(LBall(3, 6, 3))),
        ("inverse of [p=3: 0 + O(3^3)]", ref_inv(LBall(3, 0, 3))),
        ("fractional part of [p=5: 7/25 + O(5^0)]", ref_frac(LBall(5, F(7, 25), 0))),
        ("fractional part of [p=5: 7/25 + O(5^-1)]", ref_frac(LBall(5, F(7, 25), -1))),
        ("fractional part of the exact 1/3 at 2", ref_frac(LBall(2, F(1, 3)))),
        ("decomposition of [p=5: 50 + O(5^6)], prec_p = 3", ref_decompose(LBall(5, 50, 6), 3)),
        ("decomposition of [p=2: 6 + O(2^2)]", ref_decompose(LBall(2, 6, 2), 3)),
        ("decomposition of [p=2: 6 + O(2^4)]", ref_decompose(LBall(2, 6, 4), 3)),
        ("decomposition of the exact -12 at 3", ref_decompose(LBall(3, -12), 3)),
        ("exp_at of (1 +/- 0 ; 3 mod 45) over 2, 3, 5",
         ref_f_at("exp", (F(1), F(1)), (3, 45, 1), [2, 3, 5], 4)[:2]),
        ("exp_at of (1 +/- 0 ; 3 mod 45) over inf, 3", ref_f_at("exp", (F(1), F(1)), (3, 45, 1), [PLACE_INF, 3], 4)[:3]),
        ("statuses of exp_at of (1 +/- 0 ; 3 mod 45) over inf, 2, 3, 5, 7",
         ref_f_at("exp", (F(1), F(1)), (3, 45, 1), [PLACE_INF, 2, 3, 5, 7], 4)[3]),
        ("Log_at of ([-1, 1] ; 3 mod 45) over inf, 2, 3", ref_f_at("ilog", (F(-1), F(1)), (3, 45, 1), [PLACE_INF, 2, 3], 4)[:2]),
        ("cube root enclosure of [-1/1024, 1/1024], 10 bits", ref_real_root(F(-1, 1024), F(1, 1024), 3, 10)),
    ]
    for name, value in lines:
        print(f"     example: {name}: {value}", flush=True)
    return f"examples={len(lines)}"


def wrong_exp_centre_evaluation(x, prec_p):
    """The defect of review N4: the centre is evaluated to the requested precision, the radius is lost."""
    status, y = ref_exp_true(x, prec_p)
    if status == "OK" and not x.exact and x.cen != 0:
        return "OK", LBall(x.p, series_centre("exp", x.cen, x.p, max(prec_p, x.expo)), max(prec_p, x.expo))
    return status, y


def wrong_exp_uncertain_zero(x, prec_p):
    if not x.exact and x.cen == 0:
        return "OK", LBall(x.p, 1, x.expo)
    return ref_exp_true(x, prec_p)


def wrong_log_flint_domain(x, prec_p):
    if x.p == 2 and not vp(x.cen-1, 2) >= 2:
        return "DOMAIN", None
    return ref_log_true(x, prec_p)


def wrong_log_safe_radius_at_2(x, prec_p):
    status, y = ref_log_true(x, prec_p)
    if status == "OK" and not x.exact and x.expo == 1:
        return "OK", LBall(2, y.cen, 1)
    return status, y


def wrong_ilog_absolute_exponent(x, prec_p):
    status, y = ref_ilog_true(x, prec_p)
    if status == "OK" and not x.exact:
        return "OK", LBall(x.p, y.cen, x.expo)
    return status, y


def wrong_ilog_of_exact_p(x, prec_p):
    if x.exact and x.cen != 0 and x.cen/F(x.p)**int(x.val()) in (1, -1):
        return "OK", LBall(x.p, 0, prec_p)
    return ref_ilog_true(x, prec_p)


def wrong_mul_without_cross_term(x, y):
    status, z = ref_mul_true(x, y)
    if status == "OK" and not x.exact and not y.exact:
        expo = min(x.val()+y.expo, y.val()+x.expo)
        return "OK", LBall(x.p, z.cen, None if expo == inf else int(expo))
    return status, z


def wrong_inv_exponent(x):
    status, y = ref_inv_true(x)
    if status == "OK" and not x.exact:
        return "OK", LBall(x.p, y.cen, x.expo-int(x.val()))
    return status, y


def wrong_inv_status(x):
    if x.cen == 0:
        return "NOT_UNIT", None
    return ref_inv_true(x)


def wrong_projection_without_d(big_a, big_h, d, p):
    if big_h == 0:
        return LBall(p, F(big_a, d))
    return LBall(p, F(big_a, d), int(vp(big_h, p)))


def wrong_combine_first_failure(statuses):
    bad = [v for v in sorted(statuses, key=place_key) if statuses[v] != "OK"]
    return (statuses[bad[0]], bad[0]) if bad else ("OK", None)


def wrong_combine_last_place(statuses):
    status, where = combine_true(statuses)
    if where is None:
        return status, where
    return status, max((v for v in statuses if statuses[v] == status), key=place_key)


def wrong_frac_one_digit_early(x):
    if not x.exact and x.expo == -1:
        return "OK", lb_centre(x.cen, x.p, 0)
    return ref_frac_true(x)


def wrong_valuation_of_uncertain_zero(x):
    if x.cen == 0 and not x.exact:
        return "OK", x.expo
    return ref_valuation_true(x)


def wrong_decompose_sign_at_2(x, prec_p):
    if x.p == 2 and not x.exact and x.cen != 0 and x.expo-int(x.val()) == 1:
        return ref_decompose_true(LBall(2, x.cen, x.expo+1), prec_p)
    return ref_decompose_true(x, prec_p)


def wrong_centre_not_reduced(a, p, expo):
    a = F(a)
    return a if a.denominator == 1 and 0 <= a < F(p)**expo*p else lb_centre_true(a, p, expo)


def wrong_real_log_touching_zero(fn, lo, hi, degree=2):
    if fn == "log" and lo == 0 < hi:
        return "DOMAIN"
    return ref_real_status_true(fn, lo, hi, degree)


def wrong_real_even_root_touching_zero(fn, lo, hi, degree=2):
    if fn == "root" and degree % 2 == 0 and lo == 0:
        return "NOT_DETERMINED"
    return ref_real_status_true(fn, lo, hi, degree)


def wrong_odd_root_of_negative(x, k, bits):
    if F(x) < 0:
        return root_bounds_true(-F(x), k, bits)
    return root_bounds_true(x, k, bits)


ref_exp_true, ref_log_true, ref_ilog_true, ref_mul_true, ref_inv_true = ref_exp, ref_log, ref_ilog, ref_mul, ref_inv
ref_frac_true, ref_valuation_true, ref_decompose_true, combine_true = ref_frac, ref_valuation, ref_decompose, combine
lb_centre_true, ref_real_status_true, root_bounds_true = lb_centre, ref_real_status, root_bounds

MUTANTS_1F = (
    ("exp of a ball by centre evaluation at prec_p (review N4)", "ref_exp", wrong_exp_centre_evaluation,
     "check_1f_regression"),
    ("exp of a ball by centre evaluation at prec_p, enumeration", "ref_exp", wrong_exp_centre_evaluation,
     "check_1f_exp_log"),
    ("exp takes the uncertain zero for the exact zero", "ref_exp", wrong_exp_uncertain_zero, "check_1f_exp_log"),
    ("log at 2 with the domain of padic_log", "ref_log", wrong_log_flint_domain, "check_1f_exp_log"),
    ("log of 1 + 2 Z_2 returned with exponent 1", "ref_log", wrong_log_safe_radius_at_2, "check_1f_exp_log"),
    ("Log keeps the absolute exponent N", "ref_ilog", wrong_ilog_absolute_exponent, "check_1f_regression"),
    ("Log keeps the absolute exponent N, enumeration", "ref_ilog", wrong_ilog_absolute_exponent,
     "check_1f_exp_log"),
    ("Log of the exact p is a ball", "ref_ilog", wrong_ilog_of_exact_p, "check_1f_exp_log"),
    ("product without the term N + M", "ref_mul", wrong_mul_without_cross_term, "check_1f_arith"),
    ("inverse with exponent N - m", "ref_inv", wrong_inv_exponent, "check_1f_arith"),
    ("inverse of an uncertain zero is NOT_UNIT", "ref_inv", wrong_inv_status, "check_1f_arith"),
    ("projection ignores the denominator", "ref_project_fball", wrong_projection_without_d,
     "check_1f_projection"),
    ("combined status is the first failure", "combine", wrong_combine_first_failure, "check_1f_f_at"),
    ("reported place is the last one", "combine", wrong_combine_last_place, "check_1f_f_at"),
    ("fractional part determined at exponent -1", "ref_frac", wrong_frac_one_digit_early, "check_1f_typed"),
    ("valuation of O(p^N) is N", "ref_valuation", wrong_valuation_of_uncertain_zero, "check_1f_typed"),
    ("sign at 2 determined for N - m = 1", "ref_decompose", wrong_decompose_sign_at_2, "check_1f_typed"),
    ("centre of a ball not reduced", "lb_centre", wrong_centre_not_reduced, "check_1f_canonical"),
    ("real log of [0, b] is DOMAIN", "ref_real_status", wrong_real_log_touching_zero, "check_1f_real"),
    ("even root of [0, b] is NOT_DETERMINED", "ref_real_status", wrong_real_even_root_touching_zero,
     "check_1f_real"),
    ("odd root of a negative number has the wrong sign", "root_bounds", wrong_odd_root_of_negative,
     "check_1f_real"),
)


def check_1f_mutants():
    """Each wrong rule replaces the reference function; the named check must then fail."""
    space = globals()
    killed, survivors = 0, []
    for name, target, wrong, check in MUTANTS_1F:
        saved = space[target]
        space[target] = wrong
        try:
            space[check]()
            survivors.append(name)
            outcome = "SURVIVED"
        except AssertionError as err:
            killed += 1
            frames = traceback.extract_tb(err.__traceback__)
            outcome = f"killed (assert in {frames[-1].name}, line {frames[-1].lineno})"
        except Exception as ex:  # noqa: BLE001  a crash is detection too, and is reported as such
            killed += 1
            outcome = f"killed (crash: {type(ex).__name__})"
        finally:
            space[target] = saved
        print(f"     mutant {name!r} -> {check}: {outcome}", flush=True)
    assert not survivors, ("wrong rules not rejected", survivors)
    return f"planted_wrong_rules={len(MUTANTS_1F)} rejected={killed} survivors=0"


CHECKS_1F = (check_1f_canonical, check_1f_projection, check_1f_arith, check_1f_predicates, check_1f_typed,
             check_1f_teichmuller_sources, check_1f_exp_log, check_1f_regression, check_1f_f_at,
             check_1f_real, check_1f_rootlist_lball, check_1f_examples, check_1f_mutants)


def main():
    checks = (check_legendre, check_domains, check_lifting_and_groups, check_decomposition,
              check_truncation, check_working_precision, check_series_identities, check_series_radii,
              check_log_radii, check_global, check_root_criteria, check_root_precision, check_global_roots,
              check_powers, check_power_precision, check_fractional_parts, check_real_and_character,
              check_no_order, check_typed_and_projection, check_mutation_testing)
    for check in checks+CHECKS_1F:
        print(f"{check.__name__}: {check()}", flush=True)
    print(f"checks run: {len(checks)+len(CHECKS_1F)} ({len(checks)} of docs/proofs/functions.md, "
          f"{len(CHECKS_1F)} of docs/api-1f.md), all passed", flush=True)


if __name__ == "__main__":
    main()
