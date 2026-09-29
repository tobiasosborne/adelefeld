from fractions import Fraction as Q
from collections import Counter
import random
import subprocess
import sys
from mpmath import iv, mp

iv.prec = mp.prec = 6000
counts = Counter()
binary = sys.argv[1] if len(sys.argv) > 1 else 'lanes/f-review2/real_bridge'
p = subprocess.Popen(['timeout', '160', binary], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                     text=True, bufsize=1)


def interval(q):
    return iv.mpf(q.numerator) / iv.mpf(q.denominator)


def rational(t):
    sign, m, e, _ = t
    return (-1 if sign else 1) * Q(m) * Q(2)**e


def oracle(name, n, q):
    x = interval(q)
    if name == 'root':
        if not q:
            return iv.mpf(0)
        y = iv.exp(iv.log(abs(x)) / n) if n > 1 else abs(x)
        return -y if q < 0 else y
    if name == 'logabs':
        return iv.log(abs(x))
    return getattr(iv, name)(x)


def run_case(name, m, e, rm, re, n, prec):
    c, r = Q(m)*Q(2)**e, Q(rm)*Q(2)**re
    lo, hi = c-r, c+r
    want = 'OK'
    if name == 'log':
        want = 'DOMAIN' if hi <= 0 else 'NOT_DETERMINED' if lo <= 0 else 'OK'
    if name == 'logabs':
        want = 'DOMAIN' if lo == hi == 0 else 'NOT_DETERMINED' if lo <= 0 <= hi else 'OK'
    if name == 'sqrt' or name == 'root' and n > 0 and n % 2 == 0:
        want = 'DOMAIN' if hi < 0 else 'NOT_DETERMINED' if lo < 0 else 'OK'
    if name == 'root' and n == 0:
        want = 'DOMAIN'
    line = f'{name} {m} {e} {rm} {re} {n} {prec}\n'
    p.stdin.write(line); p.stdin.flush()
    reply = p.stdout.readline().split()
    assert len(reply) == 8, (line, reply)
    st, finite, alias, at, where, untouched, mid, rad = reply
    assert st == want, (line, reply, want)
    assert (finite, alias, at, where, untouched) == ('1',)*5, (line, reply)
    counts['cases'] += 1; counts['status_'+st] += 1
    counts['alias_checks'] += 1; counts['at_checks'] += 1; counts['where_checks'] += 1
    if st != 'OK':
        return
    a, b = Q(mid)-Q(rad), Q(mid)+Q(rad)
    for q in (lo, (lo+c)/2, c, (c+hi)/2, hi):
        ref = oracle(name, n, q)
        lower, upper = map(rational, ref._mpi_)
        # An outward oracle interval strictly inside the returned ball certifies the sampled point.
        assert a <= lower and upper <= b, (line, a, b, q, lower, upper)
        counts['enclosed_points'] += 1


try:
    rng = random.Random(3921)
    degrees = (0, 1, 2, 3, 2**20, 2**63, 2**63+1)
    rows = [(0, 0, 0, 0), (0, 0, 1, 0), (1, 0, 1, 0), (-1, 0, 1, 0),
            (-3, 0, 1, 0), (3, 0, 1, 0), (0, 0, 1, 20)]
    rows += [(rng.randint(-200, 200), rng.randint(-12, 0), rng.randint(0, 20),
              rng.randint(-20, 1)) for _ in range(70)]
    # Exact dyadic centres very close to multiples of pi/2, with intervals straddling the critical point.
    for k in range(-8, 9):
        m = int(mp.floor(k * mp.pi / 2 * 2**200))
        rows.append((m, -200, 1, -190))
    for row in rows:
        for prec in (2, 53, 4096):
            for name in ('exp', 'log', 'logabs', 'sin', 'cos', 'sqrt'):
                # Avoid the huge exp range solely in this bounded exact-endpoint adapter.
                if name == 'exp' and abs(Q(row[0])*Q(2)**row[1]) + Q(row[2])*Q(2)**row[3] > 200:
                    continue
                run_case(name, *row, 0, prec)
            for n in degrees:
                run_case('root', *row, n, prec)
    print(dict(counts), flush=True)
finally:
    p.stdin.close()
    assert p.wait(timeout=5) == 0
