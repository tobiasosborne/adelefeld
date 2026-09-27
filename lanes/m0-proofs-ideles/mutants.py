#!/usr/bin/env python3
"""Mutation pass over the three check files of lane m0-proofs-ideles.

Each mutant replaces one formula in the source of a check file, executes the mutated source as a fresh module
and runs the named check. A mutant is killed when that check reports FAIL (or raises). Run from the repository
root: python3 lanes/m0-proofs-ideles/mutants.py
"""
import io
import contextlib
import sys
import types

MUTANTS = [
    ("proto/policies_checks.py", "s = qgcd(c, R / K)\n", "s = R / K\n", "check_best_scaled"),
    ("proto/policies_checks.py", "g = qgcd(s, q)\n        return ('sc', g,", "g = s\n        return ('sc', g,",
     "check_scalar_add"),
    ("proto/policies_checks.py", "s2 = s * qgcd(u, F(K, K2))", "s2 = s", "check_conversion"),
    ("proto/policies_checks.py", "return (a * b, qgcd(a * M, b * N, N * M))", "return (a * b, qgcd(a * M, b * N))",
     "check_hull"),
    ("proto/policies_checks.py", "h = gcd(gcd(u, v), K)\n        return", "h = gcd(u, K)\n        return",
     "check_scaled_product"),
    ("proto/policies_checks.py", "return (c, qgcd(R, C))", "return (c, R)", "check_cap"),
    ("proto/policies_checks.py", "sm = local_set(L, [(x * (L // d) + y * (L // e)) % q",
     "sm = local_set(L, [(x * (L // e) + y * (L // d)) % q", "check_local_ops"),
    ("proto/policies_checks.py", "ok &= need(stays == ((d * e) % h == 0), 10)",
     "ok &= need(stays == (h == 1), 10)", "check_local_ops"),
    ("proto/ideles_checks.py", "    if N % 2 == 0 and (N // 2) % 2 == 1:\n        N //= 2\n", "",
     "check_canonical"),
    ("proto/ideles_checks.py", "b = a + e if a >= 2 else (2 + e if k % 2 == 0 else 0)",
     "b = a + e if a >= 2 else (1 + e if k % 2 == 0 else 0)", "check_power"),
    ("proto/ideles_checks.py", "b = 1 + e if k % (p - 1) == 0 else 0", "b = e if k % (p - 1) == 0 else 0",
     "check_power"),
    ("proto/ideles_checks.py", "ok &= prod == img(c * c2, g, M)", "ok &= prod == img(c * c2, lcm(N, N2), M)",
     "check_products"),
    ("proto/ideles_checks.py",
     "            L = lcm(N, 2)\n            M = L", "            L = N\n            M = 2 * L",
     "check_idele_to_adele"),
    ("proto/ideles_checks.py", "pred = qgcd(abs(a) * L, M)", "pred = qgcd(abs(a) * N, M)", "check_division"),
    ("proto/ideles_checks.py", "return (abs(xi) / r, (u if xi > 0 else -u) % M)", "return (abs(xi) / r, u % M)",
     "check_class_map"),
    ("proto/quotient_checks.py", "half = list(range(floor(l2), floor(h2) + 1))",
     "half = list(range(floor(l2), floor(h2)))", "check_reduction"),
    ("proto/quotient_checks.py",
     "        if s == 0 and hi_n == 1 and (w + 1 + n) % N == 0:\n            return True\n",
     "", "check_reduction"),
    ("proto/quotient_checks.py", "ok &= full == (hi - lo >= N)", "ok &= full == (hi - lo > N)", "check_full_image"),
    ("proto/quotient_checks.py", "pieces = [(a + k * N, F(A)) for k in range(B)]",
     "pieces = [(a + k * N, F(A)) for k in range(B - 1)]", "check_split"),
    ("proto/quotient_checks.py", "                T[(m - 1) % Np].append(('pt', F(0), F(0)))",
     "                T[m].append(('pt', F(0), F(0)))", "check_translation"),
    ("proto/quotient_checks.py", "k0, k1 = ceil((lo - a) / N), floor((hi - a) / N)",
     "k0, k1 = ceil((lo - a) / N), floor((hi - a) / N) - 1", "check_reconstruct_full"),
    ("proto/quotient_checks.py", "                if 2 * A * B >= m:\n",
     "                if 2 * A * B > m:\n", "check_partial"),
    # added after review (docs/reviews/m0-proofs/ideles-review.md): the two refuted claims as they were stated,
    # and the reviewer's surviving mutant
    ("proto/policies_checks.py",
     "    value of the original context (it is the same set as the original value).\"\"\"\n    return True\n",
     "    value of the original context (it is the same set as the original value).\"\"\"\n"
     "    return gcd(gcd(A, H), d) == 1\n", "check_backend_repairs"),
    ("proto/policies_checks.py", "    g = gcd(d, q)\n    return g if A % g == 0 else 0\n",
     "    return 1 if gcd(d, q) == 1 else 0\n", "check_backend_repairs"),
    ("proto/quotient_checks.py", "        for j in range(Np // Ni):", "        for j in range(1):",
     "check_mixed_families"),
    ("proto/ideles_checks.py", "    return 1 + e if k % (p - 1) == 0 else 0\n\n\ndef check_power_local",
     "    return 1 + e if k % p == 0 else 0\n\n\ndef check_power_local", "check_power_local"),
    ("proto/policies_checks.py", "== H * h // gcd(gcd(A * B, H * h), d * e)", "== H * h", "check_backend_repairs"),
]


def run(path, old, new, check):
    src = open(path).read()
    assert src.count(old) >= 1, (path, old)
    mod = types.ModuleType("mutant")
    buf = io.StringIO()
    try:
        with contextlib.redirect_stdout(buf):
            exec(compile(src.replace(old, new), path, "exec"), mod.__dict__)
            getattr(mod, check)()
        return bool(mod.FAILURES), buf.getvalue().strip().splitlines()[-1]
    except Exception as exc:  # a crash also kills the mutant
        return True, f"raised {type(exc).__name__}: {exc}"


if __name__ == "__main__":
    killed = 0
    for path, old, new, check in MUTANTS:
        k, line = run(path, old, new, check)
        killed += k
        print(f"{'KILLED ' if k else 'SURVIVED'} {path.split('/')[-1]} {check}: {line[:90]}")
    print(f"{killed} of {len(MUTANTS)} mutants killed")
    sys.exit(0 if killed == len(MUTANTS) else 1)
