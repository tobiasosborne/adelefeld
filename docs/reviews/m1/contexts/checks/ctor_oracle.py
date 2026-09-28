#!/usr/bin/env python3
"""ctor_oracle.py: checks the output of ctor_cases.c. Own computations: primes by trial
division / python-flint-free sieve, v_p(n!) by Legendre's formula, statuses from modctx.h:
blocks: DOMAIN if k < 0, q NULL with k > 0, a q_i < 2, or two q_i not coprime.
prime powers: DOMAIN if k < 0, p_i not prime, a repeated prime, e_i = 0 (checked before
UNSUPPORTED); UNSUPPORTED if some p^e >= 2^64.
fmpz: DOMAIN if K < 1; one block for 2 <= K < 2^64; none otherwise.
factorial n: K = n!, blocks p^{v_p(n!)} in increasing p; UNSUPPORTED if one >= 2^64.
primorial (n, e): K = prod_{p <= n} p^e; n < 2 or e = 0: K = 1; UNSUPPORTED if some p^e >= 2^64.
Usage: python3 ctor_oracle.py out.txt"""
import re
import sys
from math import gcd, prod

sys.set_int_max_str_digits(0)
W = 1 << 64
OK, DOMAIN, UNSUP = 0, 7, 8


def is_prime(n):
    if n < 2:
        return False
    for p in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        if n % p == 0:
            return n == p
    d, s = n - 1, 0
    while d % 2 == 0:
        d //= 2
        s += 1
    for a in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):  # deterministic below 3.3e24
        x = pow(a, d, n)
        if x in (1, n - 1):
            continue
        for _ in range(s - 1):
            x = x * x % n
            if x == n - 1:
                break
        else:
            return False
    return True


def primes_upto(n):
    return [p for p in range(2, n + 1) if is_prime(p)]


def expect_blocks(qs):
    if any(q < 2 for q in qs):
        return (DOMAIN,)
    for i in range(len(qs)):
        for j in range(i):
            if gcd(qs[i], qs[j]) != 1:
                return (DOMAIN,)
    return (OK, prod(qs), qs)


def vp_fact(n, p):
    e, pk = 0, p
    while pk <= n:
        e += n // pk
        pk *= p
    return e


bad = 0
n = 0
for line in open(sys.argv[1]):
    line = line.strip()
    if line.startswith("SUMMARY") or line.startswith("nullout"):
        print(line)
        continue
    n += 1
    m = re.match(r"(\S+) st=(\d+)(?: K=(\d+) q=(\S*))?( OUT-TOUCHED)?$", line)
    call, st = m.group(1), int(m.group(2))
    got = (st,) if st != OK else (OK, int(m.group(3)), [int(x) for x in m.group(4).split(",") if x])
    a = re.match(r"(\w+)\((.*)\)$", call)
    kind, args = a.group(1), a.group(2)
    MAX = W - 1
    if kind == "blocks":
        if args == "-1" or args == "NULL,3":
            want = (DOMAIN,)
        elif args == "":
            want = (OK, 1, [])
        else:
            qs = [MAX if x == "max" else MAX - 1 if x == "max-1" else MAX - 2 if x == "max-2" else int(x)
                  for x in args.split(",")]
            want = expect_blocks(qs)
    elif kind == "pp":
        if args == "-1":
            want = (DOMAIN,)
        elif args == "":
            want = (OK, 1, [])
        else:
            pe = []
            for t in args.split(","):
                p, e = t.split("^")
                p = MAX if p == "maxodd" else int(p)
                e = MAX if e == "maxe" else int(e)
                pe.append((p, e))
            ps = [p for p, _ in pe]
            if any(e == 0 or not is_prime(p) for p, e in pe) or len(set(ps)) != len(ps):
                want = (DOMAIN,)
            elif any(e >= 64 or p ** e >= W for p, e in pe):
                want = (UNSUP,)
            else:
                qs = [p ** e for p, e in pe]
                want = (OK, prod(qs), qs)
    elif kind == "fmpz":
        K = int(args)
        want = (DOMAIN,) if K < 1 else (OK, K, [K] if 2 <= K < W else [])
    elif kind == "fact":
        N = int(args)
        if N >= 66:   # v_2(N!) >= 64
            want = (UNSUP,)
        else:
            qs = [p ** vp_fact(N, p) for p in primes_upto(N)]
            want = (UNSUP,) if any(q >= W for q in qs) else (OK, prod(qs), qs)
            if want[0] == OK:
                f = 1
                for i in range(2, N + 1):
                    f *= i
                assert f == want[1]
    elif kind == "prim":
        if args in ("max,max", "max,64"):
            want = (UNSUP,)
        else:
            N, e = (int(x) for x in args.split(","))
            if N < 2 or e == 0:
                want = (OK, 1, [])
            else:
                ps = primes_upto(N)
                if e >= 64 or any(p ** e >= W for p in ps):
                    want = (UNSUP,)
                else:
                    qs = [p ** e for p in ps]
                    want = (OK, prod(qs), qs)
    else:
        raise SystemExit("unknown " + line)
    if tuple(got) != tuple(want) or m.group(5):
        bad += 1
        print("MISMATCH", call, "got", str(got)[:120], "want", str(want)[:120], m.group(5) or "")
print("calls checked:", n, "mismatches:", bad)
sys.exit(1 if bad else 0)
