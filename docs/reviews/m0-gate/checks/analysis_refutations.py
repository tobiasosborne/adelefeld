#!/usr/bin/env python3
"""Independent numerical attempts on analysis L6, P12, L14, P15 and catalogue P9.

mpmath values are numerical evidence, not interval certificates. The repaired proofs are also
reviewed step by step in review.md. Only the final diagnostic imports the author's splitting test.
"""
import inspect
import math
import sys
from pathlib import Path
import mpmath as m

m.mp.dps = 50
counts = {}
ratios = {}


def bounded(name, actual, bound):
    counts[name] = counts.get(name, 0) + 1
    ratio = abs(actual) / bound if bound else m.mpf(0)
    ratios[name] = max(ratios.get(name, 0), ratio)
    assert ratio <= 1 + m.mpf("1e-40"), (name, actual, bound)


def S(j, alpha, beta, T):
    K = int(m.floor(T)) + 1
    prefix = 0
    while m.exp(m.mpf(j) / K - alpha * (2 * K + 1) + beta) > m.mpf(".5"):
        prefix += K ** j * m.exp(-alpha * K * K + beta * K)
        K += 1
    rho = m.exp(m.mpf(j) / K - alpha * (2 * K + 1) + beta)
    return prefix + K ** j * m.exp(-alpha * K * K + beta * K) / (1 - rho)


def J(r, b, R):
    rp = max(r, 0)
    if b >= 2 * rp / R:
        return 2 * R ** r * m.exp(-b * R) / b
    R0 = 2 * rp / b
    t = min(max(r / b, R), R0)
    return (R0 - R) * t ** r * m.exp(-b * t) + 2 * R0 ** r * m.exp(-b * R0) / b


for alpha in map(m.mpf, [".03", ".5", "3"]):
    for beta in map(m.mpf, ["0", ".7", "4"]):
        for j in [0, 1, 4, 12]:
            for T in map(m.mpf, ["0", "2.3", "15"]):
                actual = sum(n ** j * m.exp(-alpha * n * n + beta * n)
                             for n in range(int(m.floor(T)) + 1, 260))
                bounded("analysis_L6", actual, S(j, alpha, beta, T))

for r in map(m.mpf, ["-5", "-.2", "0", ".5", "4", "30"]):
    for b in map(m.mpf, [".1", "1", "4", "30"]):
        for R in map(m.mpf, ["1", "2", "11"]):
            actual = b ** (-r - 1) * m.gammainc(r + 1, b * R, m.inf)
            bounded("analysis_L14", actual, J(r, b, R))

# Unimodal grid bound used in P12. Analytic integral via substitution y=alpha*x^2/2.
for j in [0, 1, 2, 8]:
    for alpha in map(m.mpf, [".1", "1", "6"]):
        maximum = 1 if j == 0 else (m.mpf(j) / alpha) ** (m.mpf(j) / 2) * m.exp(-m.mpf(j) / 2)
        integral = m.mpf(".5") * (2 / alpha) ** (m.mpf(j + 1) / 2) * m.gamma(m.mpf(j + 1) / 2)
        for spacing in map(m.mpf, [".03", ".1", ".7", "1", "3", "20"]):
            actual = sum((spacing * n) ** j * m.exp(-alpha * (spacing * n) ** 2 / 2)
                         for n in range(1, 1600))
            bounded("analysis_P12_grid", actual, maximum + integral / spacing)

# Positive majorants; incomplete Gamma integrates each term independently of the bound code.
for C in [1, 3, 9, 25]:
    a0 = m.pi / C
    for e in [0, 1]:
        for r in map(m.mpf, ["-3", ".2", "8"]):
            for N in [0, 2, 5]:
                actual = sum(n ** e * (a0 * n * n) ** (-r - 1)
                             * m.gammainc(r + 1, a0 * n * n, m.inf) for n in range(N + 1, 31))
                bounded("analysis_P15_sum", actual, m.exp(a0) * S(e, a0, 0, N) * J(r, a0, m.mpf(1)))
            for R in map(m.mpf, ["1", "2", "8"]):
                actual = sum(n ** e * (a0 * n * n) ** (-r - 1)
                             * m.gammainc(r + 1, a0 * n * n * R, m.inf) for n in range(1, 31))
                bounded("analysis_P15_integral", actual, m.exp(a0) * S(e, a0, 0, 0) * J(r, a0, R))

for C in [1, 5, 15]:
    a0, R, N = m.pi / C, m.mpf(3), 5
    for z in [m.mpc(".3", "2"), m.mpc("7", "-.5")]:
        e = 1
        exact = sum(n ** e * (a0 * n * n) ** (-z)
                    * m.gammainc(z, a0 * n * n, a0 * n * n * R) for n in range(1, N + 1))
        U = [max(1, R ** (z.real - 1 - ell)) for ell in range(3)]
        M2 = sum(n ** e * m.exp(-a0 * n * n) * (a0 * a0 * n ** 4 * U[0]
                 + 2 * a0 * n * n * abs(z - 1) * U[1] + abs((z - 1) * (z - 2)) * U[2])
                 for n in range(1, N + 1))
        for K in [2, 10, 40]:
            h = (R - 1) / K
            approx = h * sum(sum(n ** e * m.exp(-a0 * n * n * (1 + (k + m.mpf(".5")) * h))
                                      for n in range(1, N + 1))
                             * (1 + (k + m.mpf(".5")) * h) ** (z - 1) for k in range(K))
            bounded("analysis_P15_midpoint", exact - approx, (R - 1) * h * h * M2 / 24)

# Local-factor pole residues, independently derived by first-order denominator expansion.
pole_errors = []
eps = m.mpf("1e-20")
for p in [2, 3, 7, 19]:
    for k in [-2, 0, 3]:
        s0 = 2 * m.pi * 1j * k / m.log(p)
        residual = eps / (1 - m.exp(-(s0 + eps) * m.log(p)))
        err = abs(residual - 1 / m.log(p))
        assert err < m.mpf("1e-18")
        pole_errors.append(err)
for k in range(5):
    s = -2 * k + eps
    residue = 2 * (-m.pi) ** k / math.factorial(k)
    err = abs(eps * m.pi ** (-s / 2) * m.gamma(s / 2) - residue)
    assert err < m.mpf("1e-17")
    pole_errors.append(err)
print("catalogue_P9: checks=%d max_residue_error=%s" % (len(pole_errors), m.nstr(max(pole_errors), 10)))
for name in counts:
    print(f"{name}: checks={counts[name]} max_sample_to_bound={m.nstr(ratios[name], 10)} failures=0")

if "--splitting" in sys.argv:
    sys.path.insert(0, str(Path(__file__).resolve().parents[4] / "proto"))
    import analysis_checks as author
    recorded = []
    original = author.close

    def record(name, actual, expected, tol=author.TOL):
        error = abs(actual - expected) / max(1, abs(expected))
        recorded.append(error)
        print("general_splitting comparison=%d relative_error=%s tolerance=%s" %
              (len(recorded), m.nstr(error, 12), m.nstr(tol, 6)), flush=True)
        return original(name, actual, expected, tol)

    author.close = record
    author.check_general_splitting()
    # Deliberately alter a copied function in this process only. Keep the original oracle unchanged.
    source = inspect.getsource(author.check_general_splitting)
    assert "+b/(s-1)-a/s" in source
    source = source.replace("+b/(s-1)-a/s", "+a/(s-1)-b/s")
    namespace = dict(author.__dict__)
    exec(compile(source, "<gate-swapped-residues-mutant>", "exec"), namespace)
    try:
        namespace["check_general_splitting"]()
    except AssertionError:
        print("general_splitting swapped_pole_residues: killed=1 survived=0")
    else:
        raise AssertionError("swapped pole residues survived")
