"""Independent finite checks for docs/proofs/functions.md. Python 3, standard library only.

Run: python3 -B proto/functions_checks.py
All arithmetic is exact. Quotient checks are finite evidence, not proofs of infinite statements.
The cutoffs are proved in the companion file; a longer exact series is the numerical oracle.
"""

from fractions import Fraction as F
from functools import lru_cache
from math import factorial, gcd, inf

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


def check_truncation():
    scenarios = comparisons = 0
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
    return f"scenarios={scenarios} omitted_terms={comparisons} max_degree=4096 max_n=40"


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


def check_working_precision():
    cases = 0
    for p in PRIMES:
        for fn in FAMILY + ("log",):
            for v in range(1 if fn == "log" else cdisc(p), cdisc(p)+3):
                for n in (-2, 0, 1, 7, 20, 40):
                    count = cutoffs(p, v, n)[fn]
                    degrees = list(term_degrees(fn, count))
                    denoms = [k if fn == "log" else factorial(k) for k in degrees]
                    d = max([0] + [int(vp(den, p)) for den in denoms])
                    w = max(v, n+d)
                    x = F(-p**v, p+1)
                    # A representative modulo p^W and a second allowed lift.
                    y = residue(x, p, w)
                    for point in (y, y+p**w):
                        assert vp(partial(fn, x, count)-partial(fn, point, count), p) >= n
                        modular = F(0)
                        for k, den in zip(degrees, denoms):
                            sign = (-1)**(k-1) if fn == "log" else (-1)**(k//2) if fn in ("sin", "cos") else 1
                            modular += sign*F(pow(point, k, p**w), den)
                        assert vp(modular-partial(fn, x, count), p) >= n
                        cases += 1
                    if n > 0:
                        arg = x+1 if fn == "log" else x
                        assert residue(partial(fn, x, count), p, n) == series(fn, arg, p, n)
    return f"rounded_partial_sums={cases} target_exponents=-2,0,1,7,20,40"


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


def check_root_precision():
    images = preimages = scaled = 0
    for p in PRIMES:
        c = cdisc(p)
        for n in (1, 2, 3, 4, 5, 6):
            e = int(vp(n, p))
            for extra in (0, 1):
                relative_out = c+extra
                relative_in = relative_out+e
                k = relative_in+2
                q = p**k
                for b in (1, 3 if p == 2 else 2):
                    a = b**n
                    candidates = range(b % p**relative_out, q, p**relative_out)
                    image = {pow(y, n, q) for y in candidates}
                    expected = {(a+t*p**relative_in) % q for t in range(p**2)}
                    assert image == expected, (p, n, relative_in, b)
                    images += 1
                    preimage = {y for y in range(b % p**c, q, p**c)
                                if (pow(y, n, q)-a) % p**relative_in == 0}
                    assert preimage == set(candidates)
                    preimages += 1
                    for j in (-2, 0, 1):
                        root = F(p)**j*b
                        nin = n*j+relative_in
                        nout = nin-e-(n-1)*j
                        # Check the formula by actual rational powers, including negative exponents N.
                        actual = {residue(((root+F(p)**nout*t)**n-root**n)/F(p)**nin, p, 2)
                                  for t in range(p**2)}
                        assert actual == set(range(p**2))
                        scaled += 1
    assert {x*x % 8 for x in range(1, 8, 2)} == {1}
    assert 5 not in {x*x % 8 for x in range(8)}
    assert 4 not in {x**3 % 9 for x in range(9)}
    return f"branch_images={images} branch_preimages={preimages} rational_scaled_images={scaled} guard_failures=2"


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
    uncertain = exact = zero_centres = 0
    for p in PRIMES:
        c = cdisc(p)
        k = 5 if p == 2 else 2 if p == 13 else 3
        q = p**k
        for u0 in (1, 1+p**c, 1+2*p**c):
            alpha = vp(series("log", u0, p, k), p)
            for a in range(c, min(c+2, k)+1):
                bases = quotient_ball(u0, p, a, k)
                for s0 in (0, 1, -1, p):
                    center = pow(u0, s0, q)
                    beta = vp(s0, p)
                    for b in range(0, min(2, k-c)+1):
                        exponent_modulus = p**max(b, k-c)
                        exponents = range(s0 % p**b, exponent_modulus, p**b)
                        actual = {pow(u, s, q) for u in bases for s in exponents}
                        r = min(a+beta, b+alpha, a+b, k)
                        assert actual == quotient_ball(center, p, r, k), (p, a, b, u0, s0, r)
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
            f"zero_product_centres={zero_centres}")


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
                    # Both points are in the ball, even when the finite positive grid omits cancellation.
                    assert vp(-a, p) >= n
                    assert vp(0, p) != vp(F(p)**n, p)
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


def check_regression_mutations():
    """Concrete independent witnesses against tempting wrong precision/series rules."""
    rejected = []
    rejected.append(series("sin", 3, 3, 2) != 0)  # Constant fake sine.
    rejected.append(series("cos", 4, 2, 4) != 1)  # Constant fake cosine.
    for p, v, n in ((2, 2, 4), (3, 1, 4)):
        first = ceildiv(n, v)  # Incorrect cutoff omitting factorial valuations.
        rejected.append(vp(F(p)**(v*first)/factorial(first), p) < n)
    rejected.append(vp(F(2)**2/2, 2) < 2)  # Incorrect log cutoff omitting v_p(k).
    count = cutoffs(3, 1, 3)["exp"]
    rounded_without_guard = sum((F(pow(3, k, 27), factorial(k)) for k in range(count)), F(0))
    rejected.append(vp(rounded_without_guard-partial("exp", 3, count), 3) < 3)
    rejected.append(vp(iwasawa(12, 3, 8)-iwasawa(3, 3, 8), 3) < vp(12-3, 3))
    rejected.append(vp(series("cos", 4, 2, 6)-1, 2) < 4)  # Missing the division by 2.
    rejected.append({series("log", a, 2, 5) for a in range(1, 32, 2)} != set(range(0, 32, 2)))
    rejected.append(5 not in {x*x % 8 for x in range(8)})  # Removing the root guard.
    # A square-root ball of exponent 3 requires exponent 2, not 3, on its selected branch.
    rejected.append({b for b in range(1, 16, 4) if b*b % 8 == 1} != {1, 9})
    # Exponent uncertainty alone: fixed base 4, s in Z_3 produces 1 and 4, not one ball mod 9.
    rejected.append(vp(pow(4, 1, 27)-pow(4, 0, 27), 3) < 2)
    # Cross uncertainty with zero log and exponent centres: (1+9)^3 differs from 1 mod 3^4.
    rejected.append(vp(10**3-1, 3) == 3)
    assert all(rejected)
    return f"incorrect_rules_rejected={len(rejected)} witnesses={len(rejected)}"


def main():
    checks = (check_legendre, check_domains, check_lifting_and_groups, check_decomposition,
              check_truncation, check_working_precision, check_series_identities, check_series_radii,
              check_log_radii, check_global, check_root_criteria, check_root_precision, check_global_roots,
              check_powers, check_power_precision, check_fractional_parts, check_real_and_character,
              check_no_order, check_typed_and_projection, check_regression_mutations)
    for check in checks:
        print(f"{check.__name__}: {check()}", flush=True)


if __name__ == "__main__":
    main()
