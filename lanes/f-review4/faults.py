#!/usr/bin/env python3
# faults.py (lane f-review4): plants faults in a SCRATCH copy of src/lfunc.c, builds a library archive with the
# faulty lfunc.o and tests/test_lfunc.c against it, and reports what the stored comparison
# (stored_before_optimisation) and the rest of test_lfunc catch. Optionally also builds the lane's probe
# against the faulty archive and runs oracle.py's `small` mode on it.
# Usage: python3 lanes/f-review4/faults.py SCRATCHDIR [fault names...]
import os, sys, subprocess, shutil, re

ROOT = os.getcwd()
SCRATCH = sys.argv[1]
SRC = open('src/lfunc.c').read()
CFLAGS = "-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -Iinclude -Itests".split()

FAULTS = {
    # brief item 5 (a): the digit count of F8 one too small (the unit power is taken modulo p^(H_k - 1))
    'f8_digit': (
        "        fmpz_divexact(Qt, Q, tmp);\n        fmpz_mod(term, zk, Qt);\n",
        "        fmpz_divexact(Qt, Q, tmp);\n        if (H > 1) fmpz_divexact_ui(Qt, Qt, p);\n        fmpz_mod(term, zk, Qt);\n"),
    # brief item 5 (b): a factor of F9 with the wrong valuation (one too high: the count T is one for a
    # valuation the factor does not have, so a term of the tail is dropped)
    'f9_valuation': (
        "            log_short(term, p, z, w, K, PK);\n",
        "            log_short(term, p, z, w + 1, K, PK);\n"),
    # brief item 5 (c): the tree combines two nodes with the wrong denominator (A = A D + R C, not A D + R C B)
    'f9_tree': (
        "        fmpz_mul(tmp, R, C); fmpz_mul(tmp, tmp, B);\n",
        "        fmpz_mul(tmp, R, C);\n"),
    # own: the residual inverse of F9 with K - m - 1 digits instead of K - m
    'f9_inverse': (
        "            fmpz_divexact(z, PK, Q);\n            fmpz_invmod(inv, low, z);",
        "            fmpz_divexact(z, PK, Q); fmpz_divexact_ui(z, z, p);\n            fmpz_invmod(inv, low, z);"),
    # own: the same residual-inverse fault, but only at a prime above 2^32 (the word prime)
    'f9_inverse_bigp': (
        "            fmpz_divexact(z, PK, Q);\n            fmpz_invmod(inv, low, z);",
        "            fmpz_divexact(z, PK, Q); if (p > 4294967296UL) fmpz_divexact_ui(z, z, p);\n            fmpz_invmod(inv, low, z);"),
    # own: the F8 digit fault only for K > 40 (the stored fixture has no F8 row with K > 38)
    'f8_digit_K40': (
        "        fmpz_divexact(Qt, Q, tmp);\n        fmpz_mod(term, zk, Qt);\n",
        "        fmpz_divexact(Qt, Q, tmp);\n        if (H > 1 && K > 40) fmpz_divexact_ui(Qt, Qt, p);\n        fmpz_mod(term, zk, Qt);\n"),
    # own: F9 with the last factor's count taken for the LOWER bound v instead of the true valuation (still
    # valid: a longer sum). Expected to be caught by nothing: it is not a fault. A control.
    'control_lower_bound': (
        "            log_short(term, p, z, w, K, PK);\n",
        "            log_short(term, p, z, v, K, PK); (void) w;\n"),
}

def run(cmd, **kw):
    return subprocess.run(cmd, capture_output=True, text=True, **kw)

def main():
    names = sys.argv[2:] or list(FAULTS)
    os.makedirs(SCRATCH, exist_ok=True)
    for name in names:
        old, new = FAULTS[name]
        assert SRC.count(old) == 1, (name, SRC.count(old))
        d = os.path.join(SCRATCH, name)
        os.makedirs(d, exist_ok=True)
        src = os.path.join(d, 'lfunc.c')
        open(src, 'w').write(SRC.replace(old, new))
        lib = os.path.join(d, 'libadelefeld.a')
        shutil.copy('lanes/f-review4/build/libadelefeld.a', lib)
        r = run(['cc'] + CFLAGS + ['-Isrc', '-c', src, '-o', os.path.join(d, 'lfunc.o')])
        if r.returncode:
            print(name, 'COMPILE FAILED', r.stderr[:800]); continue
        r = run(['ar', 'r', lib, os.path.join(d, 'lfunc.o')])
        assert r.returncode == 0, r.stderr
        test = os.path.join(d, 'test_lfunc')
        r = run(['cc'] + CFLAGS + ['tests/test_lfunc.c', 'lanes/f-review4/build/support/golden.o',
                 'lanes/f-review4/build/support/jsonl.o', lib, '-lflint', '-lgmp', '-lm', '-o', test])
        assert r.returncode == 0, r.stderr[:800]
        r = run(['timeout', '600', test])
        fails = {m.group(1): int(m.group(2)) for m in re.finditer(r"FAIL (\w+) \(.*?, (\d+) failed checks\)", r.stdout)}
        summary = re.search(r"(\d+) tests, (\d+) checks, (\d+) failed checks, (\d+) failed tests", r.stdout)
        print(f"{name}: exit {r.returncode}; stored_before_optimisation failed checks: "
              f"{fails.get('stored_before_optimisation', 0)}; all failing tests: {fails}; "
              f"summary: {summary.group(0) if summary else r.stdout[-300:]}")
        if '--probe' in os.environ.get('FAULT_OPTS', '') or True:
            pd = os.path.join(d, 'probe')
            r = run(['sh', 'lanes/f-review4/build.sh', pd, lib])
            assert r.returncode == 0, r.stderr[:800]
            r = run(['timeout', '900', 'python3', '-B', 'lanes/f-review4/oracle.py', os.path.join(pd, 'probe'),
                     os.environ.get('FAULT_MODE', 'small')])
            tail = [l for l in r.stdout.split('\n') if re.match(r"\s+(diff_|oracle_fail|point_fail|cases|alias_fail)", l)]
            print(f"   probe ({os.environ.get('FAULT_MODE', 'small')}): exit {r.returncode}; " + "; ".join(t.strip() for t in tail))

if __name__ == '__main__':
    main()
