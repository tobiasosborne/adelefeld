#!/usr/bin/env python3
"""Mutation test of the new reference functions and checks of proto/solvers_checks.py (lane s-design-repair).

Each mutant changes one line of the source text of the file (the change is asserted to apply exactly once), runs
the named check in a child process, and must make that check FAIL (the child exits nonzero because a failure was
recorded). A mutant whose check still passes SURVIVES and is reported; the script exits 1 if any survives.

Usage: python3 lanes/s-design-repair/mutants.py [name ...]      (about 2 minutes; one core)
"""
import os
import subprocess
import sys
import time
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "proto/solvers_checks.py").read_text().rsplit('if __name__ == "__main__":', 1)[0]

# (name, old text, new text, check to run)
MUTANTS = [
    ("nd_off_by_one", "    if X > limit:\n        return NOT_DETERMINED, found, cert",
     "    if X >= limit:\n        return NOT_DETERMINED, found, cert", "check_s3_limit"),
    ("nd_ignores_found_count", "            if len(found) == 2:\n                return NOT_UNIQUE, found, cert",
     "            if len(found) == 3:\n                return NOT_UNIQUE, found, cert", "check_s3_limit"),
    ("first_count_wrong", "        return OK, sols[0], 2, cert", "        return OK, sols[0], 1, cert", "check_s3_verify"),
    ("verify_not_unique_unchecked",
     "        return len(set(sols)) >= 2 and all(is_solution(m, c, A, B, n, d) for n, d in sols)",
     "        return len(set(sols)) >= 2", "check_s3_verify"),
    ("verify_ok_without_uniqueness",
     "        return sols[0] == ((1 if T > 0 else -1) * R, abs(T)) if 2 * A * B < m else set(reduced) == set(sols)",
     "        return True", "check_s3_verify"),
    ("brute_fast_start", "        n = r - m * ((r + A) // m)", "        n = r - m * ((r + A) // m) + m", "check_s3_verify"),
    ("real_complete_without_count", "    return len(balls) == n == (0 if len(g) == 1 else sturm_count(g))",
     "    return len(balls) == n", "check_s2_lists"),
    ("real_entries_without_disjointness",
     "        if not real_entry_ok(g, lo, hi) or (prev is not None and not prev < F(lo)):",
     "        if not real_entry_ok(g, lo, hi):", "check_s2_real_completeness"),
    ("accuracy_bits_off", "    return -(top_bit(rad) - top_bit(abs(mid)) + 1)", "    return -(top_bit(rad) - top_bit(abs(mid)) - 1)",
     "check_s2_real_planted"),
    ("refine_ignores_floor", "and (floor is None or lo > floor) \\", "\\", "check_s2_real_planted"),
    ("cauchy_bound_too_small", "    while B * lead <= lead + height:", "    while B * lead <= height:", "check_s2_real_planted"),
    ("seed_uses_f_not_g", "    g = normalise_g(f)\n    ga, da = peval(g, a), peval(pderiv(g), a)",
     "    g = ptrim(f)\n    ga, da = peval(g, a), peval(pderiv(g), a)", "check_s2_seed"),
    ("seed_weak_condition", "    if vg is not None and not vg > 2 * s:", "    if vg is not None and not vg >= 2 * s:",
     "check_s2_seed"),
    ("complete_verifier_no_rerun", "    st, certs, unres = padic_roots(L[\"g\"], p, 1, depth)\n    if unres:\n        return False",
     "    st, certs, unres = padic_roots(L[\"g\"], p, 1, depth)\n    return True", "check_s2_lists"),
    ("entries_no_disjointness",
     "    return all(not balls_meet(p, items[i], items[j]) for i in range(len(items)) for j in range(i))",
     "    return True", "check_s2_lists"),
    ("entries_no_cert_test", "    if not all(root_cert_ok(L[\"g\"], p, a, k, s) for a, k, s in L[\"certs\"]):\n        return False",
     "    pass", "check_s2_lists"),
    ("rootlist_not_squarefree", "    g = normalise_g(f)\n    st, certs, unres = padic_roots(g, p, kreq, depth)",
     "    g = ptrim(f)\n    st, certs, unres = padic_roots(g, p, kreq, depth)", "check_s2_lists"),
    ("assoc_zero_is_N", "    return 0 if a % N == 0 else gcd(a, N)", "    return gcd(a, N)", "check_s1_assoc"),
    ("checker_drops_K4", "        if prod != N ** c:\n            return False", "        if prod > N ** c:\n            return False",
     "check_s1_checker_mutants"),
    ("trace_counts_wrong", "                stack.append((a + p ** e * b, e + 1, w))\n                nchild += 1",
     "                stack.append((a + p ** e * b, e + 1, w))\n                nchild += 2", "check_s2_bezout_depth"),
]

EQUIVALENT = [
    ("real_entry_weak_sign", "g(lo) g(hi) <= 0 instead of < 0 for lo < hi",
     "an end point is then a root, so every accepted ball still holds a root; with disjoint balls and n equal to the "
     "count the pigeonhole of Proposition 3.8(2) still gives exactly one root in each: the change does not alter "
     "what a verified list claims (the design keeps the strict test because it is the one stated and proved)"),
]
only = set(sys.argv[1:])
env = dict(os.environ, OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1")
survivors, ran = [], 0
for name, old, new, check in MUTANTS:
    if only and name not in only:
        continue
    assert source.count(old) == 1, f"mutant {name}: the text to change occurs {source.count(old)} times"
    t = time.time()
    r = subprocess.run([sys.executable, "-c", source.replace(old, new) + f"\n{check}()\nassert not FAILURES\n"],
                       text=True, capture_output=True, timeout=170, env=env)
    ran += 1
    killed = r.returncode != 0
    if not killed:
        survivors.append(name)
    out = [ln for ln in r.stdout.splitlines() if ln.startswith(("PASS", "FAIL"))]
    print(f"{'KILLED  ' if killed else 'SURVIVED'} {name} by {check} ({time.time() - t:.0f} s): "
          f"{(out[0][:110] if out else (r.stderr.strip().splitlines() or ['?'])[-1][:110])}")
for name, what, why in EQUIVALENT:
    print(f"EQUIVALENT {name} ({what}): {why}")
print(f"mutants: {ran}, killed: {ran - len(survivors)}, survived: {len(survivors)} {survivors}")
sys.exit(1 if survivors else 0)
