#!/usr/bin/env python3
"""Independent edge probes and three isolated mutations of the S reference."""
from contextlib import redirect_stdout
from fractions import Fraction
from io import StringIO
from pathlib import Path
import runpy


SOURCE = Path(__file__).resolve().parents[4] / "proto/solvers_checks.py"
base = runpy.run_path(str(SOURCE), run_name="closure_probe")


def check_edges():
    huge = 10 ** 400
    cases = [
        ([-huge, 1], [huge]),
        ([huge, -1], [huge]),
        ([-1, huge], [Fraction(1, huge)]),
        ([1, huge], [Fraction(-1, huge)]),
        ([-huge, 1 - huge, 1], [-1, huge]),
        ([huge, huge - 1, -1], [-1, huge]),
    ]
    for f, roots in cases:
        status, count, balls = base["real_roots_ref"](f, 6)
        assert status == "OK" and count == len(roots)
        assert all(sum(lo <= root <= hi for lo, hi in balls) == 1 for root in roots)
    print(f"huge_real: {len(cases)} cases, 400-digit coefficients, 0 failures")

    seeds = [([0, 27], 3, 0, [0, 1]), ([0, 8], 2, 0, [0, 1]),
             ([8, -8], 2, 1, [-1, 1]), ([0, -9], 3, 0, [0, 1])]
    for f, p, a, expected_g in seeds:
        status, out = base["seed_root"](f, p, a, 1)
        assert status == "OK" and out["g"] == expected_g
        assert out["certs"][0][2] == 0
        assert base["padic_verify_entries"](f, out)
        assert not base["padic_verify_complete"](f, out, 3)
    assert base["seed_root"]([1, 2], 2, 0, 1)[0] == "NOT_DETERMINED"
    print("seed_neighbours: 5 cases including content at 2 and 3, 0 failures")

    certs = 0
    for p in (2, 3):
        for c0 in range(-2, 3):
            for c1 in range(-2, 3):
                for c2 in range(-2, 3):
                    f = [c0, c1, c2]
                    for k in (1, 2, 3):
                        for s in range(k):
                            for a in range(p ** k):
                                if not base["root_cert_ok"](f, p, a, k, s):
                                    continue
                                certs += 1
                                for kp in range(s + 1, k + 1):
                                    assert base["root_cert_ok"](f, p, a % p ** kp, kp, s)
    assert not base["root_cert_ok"]([3, 0, 1], 2, 1, 1, 1)
    print(f"certificate_truncation: {certs} certificates, 0 failures")

    f = [0, -2, 1]  # X(X-2) at 2
    status, out = base["rootlist_padic"](f, 2, 1, 3)
    assert status == "OK" and base["padic_verify_complete"](f, out, 3)
    bad = dict(out)
    bad["certs"] = []
    bad["n"] = 0
    assert base["padic_verify_entries"](f, bad)
    assert not base["padic_verify_complete"](f, bad, 3)
    f = [-6, 11, -6, 1]  # (X-1)(X-2)(X-3)
    assert base["real_verify_entries"](f, [(0, 4)])
    assert not base["real_verify_complete"](f, [(0, 4)], 1)
    print("list_verifiers: 3 changed claims, 0 false complete acceptances")

    f = [0, 2, 2, 1]  # X(X^2+2X+2), sole root 0 in Z_2
    derivative = base["pderiv"](f)
    s = base["val"](base["peval"](derivative, 0), 2)
    assert s == 1
    assert all((x * x + 2 * x + 2) % 4 != 0 for x in range(4))
    print("isolation_text: f=X(X^2+2X+2), p=2, root 0, v_p(f'(0))=1, sum over other integer roots=0")

    f = [0, 2]
    assert base["normalise_g"](f) == [0, 1]
    print("reduced_field: f=2X, g*=X; same up to a constant, content 2")

    a = 2 ** 5000
    g = [0, 1]
    s = base["val"](base["peval"](base["pderiv"](g), a), 2)
    k0 = base["val"](base["peval"](g, a), 2) - s
    K = max(1, s + 1)
    assert (k0, 2 * K) == (5000, 2)
    print("seed_resource: g=X, p=2, a=2^5000, k0=5000, 2K=2")


def mutated_namespace(name, start, end, replacement):
    text = SOURCE.read_text()
    left = text.index(start)
    right = text.index(end, left)
    old = text[left:right]
    assert name in old
    text = text[:left] + replacement(old) + text[right:]
    ns = {"__name__": "closure_mutant"}
    exec(compile(text, str(SOURCE), "exec"), ns)
    return ns


def run_check(ns, check_name):
    ns["FAILURES"].clear()
    out = StringIO()
    with redirect_stdout(out):
        ns[check_name]()
    return len(ns["FAILURES"]), out.getvalue().strip().splitlines()[-1]


def check_mutants():
    def seed_change(old):
        assert old.count("    g = normalise_g(f)\n") == 1
        return old.replace("    g = normalise_g(f)\n", "    g = f\n", 1)

    seed = mutated_namespace("seed_root", "def seed_root(", "def balls_meet(", seed_change)
    failures, line = run_check(seed, "check_s2_seed")
    assert failures > 0
    print("mutant_seed_uses_f: KILLED; " + line)

    def complete_change(old):
        cut = old.index("    p = L[\"place\"]")
        return old[:cut] + "    return True\n\n\n"

    complete = mutated_namespace("padic_verify_complete", "def padic_verify_complete(",
                                 "def approx_roots(", complete_change)
    failures, line = run_check(complete, "check_s2_lists")
    assert failures > 0
    print("mutant_complete_skips_rerun: KILLED; " + line)

    def accuracy_change(old):
        assert old.count(" and real_accuracy_ok(out, prec)") == 1
        return old.replace(" and real_accuracy_ok(out, prec)", "", 1)

    accuracy = mutated_namespace("rr_finish", "def rr_finish(", "REAL_CASES =", accuracy_change)
    failures, line = run_check(accuracy, "probe_s2_flint_real")
    assert failures == 0
    lo, hi = Fraction(1), Fraction(1) + Fraction(6, 1024)
    baseline = base["rr_finish"]([-1, 1], [(lo, hi)], 8)[0]
    mutant = accuracy["rr_finish"]([-1, 1], [(lo, hi)], 8)[0]
    assert baseline == "NOT_DETERMINED" and mutant == "OK"
    print("mutant_rr_skips_final_accuracy: SURVIVED current FLINT probe; witness baseline=" +
          baseline + ", mutant=" + mutant + "; " + line)


if __name__ == "__main__":
    check_edges()
    check_mutants()
