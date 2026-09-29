#!/usr/bin/env python3
"""Independent finite oracles for milestone S. No author functions are imported.

The target is run as a separate Python program with a JSON transport appended to its
unchanged function definitions. All expected answers are computed here, by enumeration.
Sources of the contracts: docs/proofs/solvers.md and docs/api-s.md. This is review code.
Run one suite at a time; each has a 165 second wall-clock budget.
"""
import argparse
import hashlib
import itertools as it
import json
import math
import os
from pathlib import Path
import random
import signal
import subprocess
import sys
import time
from fractions import Fraction as Q

ROOT = Path(__file__).resolve().parents[3]
AUTHOR = ROOT / 'proto/solvers_checks.py'
MODULI = (1, 4, 6, 8, 9, 12, 16, 36)
RNG = random.Random(294710)
FAIL = []
FINDINGS = []

BRIDGE = '''
import json
for line in sys.stdin:
    results = []
    for name, args in json.loads(line):
        try:
            out = globals()[name](*args)
            if name == 'real_roots_ref':
                out = [out[0], out[1], [[str(a), str(b)] for a,b in out[2]]]
            results.append({'out': out})
        except Exception as err:
            results.append({'error': type(err).__name__, 'message': str(err)})
    print(json.dumps(results), flush=True)
'''


class Target:
    def __init__(self):
        source = AUTHOR.read_text().rsplit('if __name__ == "__main__":', 1)[0]
        self.p = subprocess.Popen([sys.executable, '-u', '-c', source + BRIDGE],
                                  stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True,
                                  env=dict(os.environ, OMP_NUM_THREADS='1', OPENBLAS_NUM_THREADS='1'))

    def call(self, requests):
        self.p.stdin.write(json.dumps(requests) + '\n')
        self.p.stdin.flush()
        result = json.loads(self.p.stdout.readline())
        assert len(result) == len(requests)
        return result

    def close(self):
        self.p.stdin.close()
        assert self.p.wait(timeout=5) == 0


def batches(items, n=512):
    items = iter(items)
    while part := list(it.islice(items, n)):
        yield part


def check(ok, context):
    if not ok:
        if len(FAIL) < 10:
            print('FAIL', context, flush=True)
        FAIL.append(str(context))


def show(name, **counts):
    print(name + ': ' + ', '.join(f'{k}={v}' for k, v in counts.items()), flush=True)


def finding(name, text):
    FINDINGS.append(name)
    print('FINDING ' + name + ': ' + text, flush=True)


def fractions(m, c, A, B, reduced=True):
    return {(n, d) for n in range(-A, A + 1) for d in range(1, B + 1)
            if (n - c*d) % m == 0 and (not reduced or math.gcd(n, d) == 1)}


def classify(s):
    return 'NO_SOLUTION' if not s else 'OK' if len(s) == 1 else 'NOT_UNIQUE'


def recon(t):
    # Every tuple in these rectangular ranges, including c outside its canonical range.
    cases = [(m, c, A, B) for m in range(1, 19) for c in range(-m, 2*m+1)
             for A in range(m+2) for B in range(1, 9)]
    ncall = boundary = points = none = 0
    for chunk in batches(cases, 256):
        req = [('recon_partial', [*x, lim]) for x in chunk for lim in (-1, 0, 1, 3, 100)]
        outs = t.call(req)
        for j, args in enumerate(chunk):
            m, c, A, B = args
            want = fractions(*args)
            points += len(want)
            none += not want
            boundary += abs(2*A*B-m) <= 1
            for lim, ans in zip((-1, 0, 1, 3, 100), outs[j*5:j*5+5]):
                ncall += 1
                check('out' in ans, (args, lim, ans))
                if 'out' not in ans:
                    continue
                status, pairs, cert = ans['out']
                got = set(map(tuple, pairs))
                check(got <= want, (args, 'extraneous', pairs))
                if status != 'NOT_DETERMINED':
                    check(status == classify(want), (args, lim, status, want))
                    if status == 'OK':
                        check(got == want, (args, 'unique', got, want))
                else:
                    check(A < m <= 2*A*B and cert is not None, (args, 'ND preconditions'))
                    Rp, Tp, R, T = cert
                    # Invert phi for independently enumerated points, rather than using lattice_points.
                    sigma = 1 if T > 0 else -1
                    searched = []
                    for nn, dd in want:
                        y = (R*dd - sigma*nn*abs(T)) // m
                        x = (dd - y*abs(Tp)) // abs(T)
                        if x <= max(lim, 0):
                            searched.append((nn, dd))
                    check(abs(T) <= B and B//abs(T) > max(lim, 0) and len(searched) < 2,
                          (args, 'ND iff', lim, searched))
                if cert is not None:
                    Rp, Tp, R, T = cert
                    check((R-c*T) % m == (Rp-c*Tp) % m == 0 and
                          abs(T*Rp-Tp*R) == m and 0 <= R <= A < Rp and T != 0 and T*Tp <= 0,
                          (args, cert))
                    check(math.gcd(R, T) == math.gcd(T, m), (args, 'gcd'))
    show('reconstruction', boxes=len(cases), calls=ncall, boundary_boxes=boundary,
         solutions=points, empty_boxes=none, failures=len(FAIL))
    # Arbitrary certificates, not only the author's Euclidean rows.
    accepted = tried = 0
    for m in range(1, 10):
        for c in range(m):
            for A in range(m):
                want = fractions(m, c, A, 12, False)
                for Rp in range(A+1, m+1):
                    for R in range(A+1):
                        for T in range(-m, m+1):
                            for Tp in range(-m, m+1):
                                tried += 1
                                if (not T or T*Tp > 0 or abs(T*Rp-Tp*R) != m or
                                        (R-c*T) % m or (Rp-c*Tp) % m):
                                    continue
                                accepted += 1
                                sg = 1 if T > 0 else -1
                                got = {(sg*(x*R-y*Rp), x*abs(T)+y*abs(Tp))
                                       for x in range(1, 13) for y in range(13)
                                       if abs(x*R-y*Rp) <= A and x*abs(T)+y*abs(Tp) <= 12}
                                check(got == want, ('arbitrary certificate', m, c, A, Rp, Tp, R, T))
    show('arbitrary_reconstruction_certificates', tried=tried, accepted=accepted, false_accepted=len(FAIL))


def span(rows, n, N):
    # Group closure by adding generators, not coefficient reduction or Howell computation.
    zero = (0,)*n
    seen, todo = {zero}, [zero]
    while todo:
        v = todo.pop()
        for row in rows:
            w = tuple((v[j]+row[j]) % N for j in range(n))
            if w not in seen:
                seen.add(w)
                todo.append(w)
    return seen


def canonical_from_set(S, n):
    # Lexicographic minima in the actual finite module determine its reduced rows.
    return [list(min(candidates)) for j in range(n)
            if (candidates := [v for v in S if not any(v[:j]) and v[j]])]


def image_of(A, x, N):
    return tuple(sum(row[j]*x[j] for j in range(len(x))) % N for row in A)


def matrix_cases():
    for N in MODULI:
        for r, c in ((0, 0), (0, 1), (0, 2), (1, 0), (2, 0), (1, 1), (1, 2), (2, 1)):
            for flat in it.product(range(N), repeat=r*c):
                A = [list(flat[i*c:(i+1)*c]) for i in range(r)]
                yield N, r, c, A, True
        if N <= 9:
            for flat in it.product(range(N), repeat=4):
                yield N, 2, 2, [list(flat[:2]), list(flat[2:])], False
        for r, c in ((2, 2), (2, 3), (3, 2), (3, 1), (1, 3)):
            for _ in range(30):
                A = [[RNG.randrange(N) for _ in range(c)] for _ in range(r)]
                yield N, r, c, A, False


def matrix(t):
    mats = systems = vectors = certificates = 0
    for chunk in batches(matrix_cases(), 64):
        req, expected = [], []
        for N, r, c, A, exhaustive_rhs in chunk:
            mats += 1
            points = list(it.product(range(N), repeat=c))
            by_b = {}
            for x in points:
                by_b.setdefault(image_of(A, x, N), set()).add(x)
            kernel = by_b[(0,)*r]
            all_b = list(it.product(range(N), repeat=r)) if exhaustive_rhs else list(
                dict.fromkeys([(0,)*r, tuple(RNG.randrange(N) for _ in range(r)),
                               image_of(A, points[-1], N)]))
            for b in all_b:
                req.append(('linsolve_mod', [A, list(b), r, c, N]))
                expected.append((N, r, c, A, b, kernel, by_b.get(b, set())))
            vectors += len(points)
        answers = t.call(req)
        certreq = []
        for data, answer in zip(expected, answers):
            N, r, c, A, b, kernel, want = data
            systems += 1
            if 'out' not in answer:
                check(False, ('solver raised', data, answer))
                continue
            sol = answer['out']
            check(sol['status'] == ('OK' if want else 'NO_SOLUTION'), ('status', data, sol))
            gotker = span(sol['G'], c, N)
            check(gotker == kernel and sol['G'] == canonical_from_set(kernel, c), ('kernel', data, sol))
            if want:
                got = {tuple((x[j]+sol['x0'][j]) % N for j in range(c)) for x in gotker}
                check(got == want, ('coset', data, sol))
            else:
                y = sol['y']
                check(all(sum(y[i]*A[i][j] for i in range(r)) % N == 0 for j in range(c))
                      and sum(y[i]*b[i] for i in range(r)) % N != 0, ('dual', data, sol))
            certreq.append(('linsol_check', [sol, A, list(b), r, c, N]))
        for ans in t.call(certreq):
            certificates += 1
            check(ans == {'out': True}, ('generated certificate', ans))
    show('matrix', matrices=mats, systems=systems, vectors=vectors,
         certificates=certificates, failures=len(FAIL))
    # Exhaust every scalar certificate, including invented E, V, G and opposite statuses.
    trial = accepted = false = 0
    jobs, truths = [], []
    for N in MODULI:
        divisors = [d for d in range(1, N) if N % d == 0]
        forms = [[]] + [[[d]] for d in divisors]
        for a in range(N):
            kernel = {(x,) for x in range(N) if a*x % N == 0}
            for E, G in it.product(forms, repeat=2):
                for v in range(N) if E else (None,):
                    sol = {'status': 'OK', 'x0': [0], 'E': E, 'V': [[v]] if E else [], 'G': G}
                    jobs.append(('linsol_check', [sol, [[a]], [0], 1, 1, N]))
                    truths.append(span(G, 1, N) == kernel)
    for off in range(0, len(jobs), 512):
        for truth, ans in zip(truths[off:off+512], t.call(jobs[off:off+512])):
            trial += 1
            if ans.get('out'):
                accepted += 1
                false += not truth
    check(false == 0, ('false scalar certificate', false))
    show('invented_scalar_certificates', tried=trial, accepted=accepted, false_accepted=false)


def evalp(f, x, mod=None):
    value = sum(c*x**i for i, c in enumerate(f))
    return value if mod is None else value % mod


def derivative(f):
    return [i*f[i] for i in range(1, len(f))]


def val(x, p):
    if not x:
        return math.inf
    v = 0
    while x % p == 0:
        x //= p
        v += 1
    return v


def primitive_p(f, p):
    v = min(val(c, p) for c in f if c)
    return [c//p**v for c in f]


def residue_roots(f, p, M):
    # Enumerate surviving residues, digit by digit; no variable transform or Hensel formula.
    residues, modulus = [0], 1
    for _ in range(M):
        residues = [x+t*modulus for x in residues for t in range(p)
                    if evalp(f, x+t*modulus, modulus*p) == 0]
        modulus *= p
        assert len(residues) < 300000
    return residues


def roots(t):
    cases = [(list(f), p, 2, D) for p in (2, 3) for d in range(4)
             for f in it.product(range(-2, 3), repeat=d+1) if f[-1]
             for D in (0, 1, 2)]
    cases += [([0, p**h, 0, 1], p, 1, h+1) for p in (2, 3) for h in range(1, 5)]
    cases += [([0], 2, 2, 0), ([], 3, 2, 0), ([0, 27], 3, 2, 0), ([1, 2], 2, 3, 1)]
    counts = {2: [0, 0, 0], 3: [0, 0, 0]}
    newton_requests = []
    for chunk in batches(cases, 128):
        answers = t.call([('padic_roots', x) for x in chunk])
        for (f, p, kreq, D), ans in zip(chunk, answers):
            check('out' in ans, ('padic exception', f, p, ans))
            if 'out' not in ans:
                continue
            status, certs, unresolved = ans['out']
            counts[p][0] += 1
            if not any(f):
                check(status == 'DOMAIN', ('zero polynomial', ans))
                continue
            f = primitive_p(f, p)
            items = [(a, k) for a, k, _ in certs] + list(map(tuple, unresolved))
            check(all((a-b) % p**min(k, l) for i, (a, k) in enumerate(items)
                      for b, l in items[:i]), ('overlap', f, p, ans))
            check((status == 'OK') == (len(unresolved) == 0), ('complete', f, p, ans))
            # Enough depth for these finite tests; this is not asserted as a general depth theorem.
            M = max([k+s+2 for a, k, s in certs] + [len(f)*(D+1)+2, 6])
            rs = residue_roots(f, p, M)
            counts[p][2] += len(rs)
            check(all(sum((x-a) % p**k == 0 for a, k in items) == 1 for x in rs),
                  ('uncovered finite roots', f, p, D, M, ans))
            for a, k, s in certs:
                counts[p][1] += 1
                check(k == max(kreq, s+1) and val(evalp(derivative(f), a), p) == s
                      and evalp(f, a, p**(k+s)) == 0, ('bad certificate', f, p, a, k, s))
                check(sum((x-a) % p**k == 0 for x in rs) == p**s,
                      ('certificate residue count', f, p, a, k, s))
                newton_requests.append((f, p, a, k, s))
    show('padic_p2', calls=counts[2][0], certificates=counts[2][1], residues=counts[2][2])
    show('padic_p3', calls=counts[3][0], certificates=counts[3][1], residues=counts[3][2])
    for chunk in batches(newton_requests):
        for data, ans in zip(chunk, t.call([('newton_step', x) for x in chunk])):
            f, p, a, k, s = data
            a2, k2 = ans['out']
            # Brute force the permitted next digit block, not the Newton quotient.
            good = [x for x in range(a, p**(2*k-s), p**k)
                    if evalp(f, x, p**(2*k)) == 0]
            check(k2 == 2*k-s and good == [a2], ('newton differs from exhaustive extension', data, ans))
    show('newton', calls=len(newton_requests), failures=len(FAIL))
    # Direct P3.4 w, derivative identity and (W), using the binomial theorem.
    nodes = 0
    for p in (2, 3):
        for f in ([1, 0, 1], [-9, 0, 1], [0, 4, 0, 1], [0, 0, 1], [3, 6, 9]):
            f = primitive_p(f, p)
            for e in range(4):
                for a in range(p**e):
                    coeff = [sum(f[i]*math.comb(i, j)*a**(i-j)*p**(e*j)
                                 for i in range(j, len(f))) for j in range(len(f))]
                    w = min(val(z, p) for z in coeff)
                    g = [z//p**w for z in coeff]
                    for b in range(p):
                        if evalp(g, b, p) or evalp(derivative(g), b, p):
                            continue
                        a1 = a+p**e*b
                        child = [sum(f[i]*math.comb(i, j)*a1**(i-j)*p**((e+1)*j)
                                     for i in range(j, len(f))) for j in range(len(f))]
                        w1 = min(val(z, p) for z in child)
                        cg = [z//p**w1 for z in child]
                        nodes += 1
                        check(w1 >= w+1 and (not any(evalp(cg, x, p) == 0 for x in range(p))
                                              or w1 >= w+2), ('P3.4', f, a, e, b))
    show('singular_children', nodes=nodes, failures=len(FAIL))
    # x^3+2x = x(x^2+2); x^2+2 has no root even modulo 4.
    check(not residue_roots([2, 0, 1], 2, 2), 'nonzero factor has no 2-adic root')
    finding('3.11-minimum', 'f=X^3+2X, p=2: only root 0, s=1; Z_2 already isolates it (k=0 < 2)')
    finding('seed-normalization', 'f=27X, p=3, a=0: seed s=3; stored g=X has derivative valuation 0')


def multiply(f, g):
    out = [0]*(len(f)+len(g)-1)
    for i, a in enumerate(f):
        for j, b in enumerate(g):
            out[i+j] += a*b
    return out


def real(t):
    # All distinct roots are known exactly by construction, including repeated and close ones.
    cases = []
    pool = [Q(-3), Q(-1, 2), Q(0), Q(1, 3), Q(1), Q(1)+Q(1, 2**40)]
    for roots_ in it.combinations_with_replacement(pool, 3):
        f = [1]
        for q in roots_:
            f = multiply(f, [-q.numerator, q.denominator])
        cases.append((f, sorted(set(roots_))))
    cases += [([1], []), ([0, 1], [Q(0)]), ([1, 0, 1], [])]
    exact = 0
    for (f, want), ans in zip(cases, t.call([('real_roots_ref', [f, 6]) for f, _ in cases])):
        check('out' in ans, ('real exception', f, ans))
        if 'out' not in ans:
            continue
        st, n, balls = ans['out']
        intervals = [(Q(a), Q(b)) for a, b in balls]
        exact += sum(a == b for a, b in intervals)
        check(st == 'OK' and n == len(want), ('real number', f, ans))
        check(all(sum(a <= q <= b for q in want) == 1 for a, b in intervals)
              and all(sum(a <= q <= b for a, b in intervals) == 1 for q in want), ('real cover', f, ans))
    show('real_planted', polynomials=len(cases), exact_intervals=exact, failures=len(FAIL))
    probes = t.call([('real_roots_ref', [[-10**400, 1], 6]),
                     ('real_cert_ok', [[-6, 11, -6, 1], 1, [[0, 4]]]),
                     ('real_cert_ok', [[-1, 1], 1, [[1, 2]]]),
                     ('real_cert_ok', [[-1, 1], 1, [[1, 1]]])])
    check(probes[0].get('error') == 'OverflowError', ('overflow reproduction', probes[0]))
    check([x.get('out') for x in probes[1:]] == [True, False, True], ('real controls', probes))
    finding('reference-overflow', 'real_roots_ref([-10**400,1],6) raises OverflowError; exact answer is {10**400}')
    finding('untrusted-real-count', 'real_cert_ok((X-1)(X-2)(X-3),1,[[0,4]]) accepts a 3-root interval')
    show('real_endpoints', positive_width_endpoint_rejected=1, exact_root_point_accepted=1)


def extras(t):
    rowcases = [(N, [list(v[:2]), list(v[2:])]) for N in (1, 4, 6, 8, 9, 12)
                for v in it.product(range(N), repeat=4)]
    howell_n = 0
    for chunk in batches(rowcases):
        answers = t.call([('howell', [rows, 2, N]) for N, rows in chunk])
        for (N, rows), ans in zip(chunk, answers):
            S = span(rows, 2, N)
            H = ans['out']
            howell_n += 1
            check(H == canonical_from_set(S, 2), ('arbitrary H', N, rows, ans))
            order = 1
            for i, row in enumerate(H):
                j = next(j for j, x in enumerate(row) if x)
                order *= N//row[j]
                check(span(H[i+1:], 2, N) == {v for v in S if not any(v[:j+1])},
                      ('E4 by definition', N, rows, H))
            check(order == len(S), ('Howell count', N, rows, H))
    show('arbitrary_howell', matrices=howell_n, failures=len(FAIL))
    ballcases = []
    for _ in range(250):
        r, c = RNG.randrange(4), RNG.randrange(3)
        A = [[RNG.randrange(-3, 4) for _ in range(c)] for _ in range(r)]
        balls = [(RNG.randrange(-8, 9), RNG.choice((1, 2, 3, 4, 6)), RNG.randrange(1, 5))
                 for _ in range(r)]
        ballcases.append((A, balls, r, c))
    systems = projections = tested = 0
    transforms = t.call([('adelic_system', x) for x in ballcases])
    solreq = []
    for (A, balls, r, c), ans in zip(ballcases, transforms):
        A2, b2, N = ans['out']
        solreq.append(('linsolve_mod', [A2, b2, r, c, N]))
    for case, tr, ans in zip(ballcases, transforms, t.call(solreq)):
        A, balls, r, c = case
        A2, b2, N = tr['out']
        sol = ans['out']
        expected = set()
        for x in it.product(range(N), repeat=c):
            tested += 1
            good = all(Q(d*sum(A[i][j]*x[j] for j in range(c))-B, H).denominator == 1
                       for i, (B, H, d) in enumerate(balls))
            check(good == (image_of(A2, x, N) == tuple(b2)), ('ball transform', case, x))
            if good:
                expected.add(x)
        systems += 1
        check((sol['status'] == 'OK') == bool(expected), ('ball status', case, ans))
        if expected:
            for j in range(c):
                radius = math.gcd(N, *(v[j] for v in sol['G']))
                predicted = {x for x in range(N) if (x-sol['x0'][j]) % radius == 0}
                check(predicted == {x[j] for x in expected}, ('coordinate projection', case, j))
                projections += 1
    show('adelic', systems=systems, integer_points=tested, projections=projections, failures=len(FAIL))
    polynomials = [(p, list(f)) for p in (2, 3) for d in range(6)
                   for f in it.product(range(p), repeat=d+1) if f[-1]]
    for chunk in batches(polynomials):
        req = []
        for p, f in chunk:
            xp = [0]*(p+1)
            xp[p], xp[1] = 1, -1
            req.append(('pgcd_p', [f, xp, p]))
        for (p, f), ans in zip(chunk, t.call(req)):
            count = sum(evalp(f, x, p) == 0 for x in range(p))
            check(len(ans['out'])-1 == count, ('finite-field count', p, f, ans))
    show('mod_p_count', polynomials=len(polynomials), failures=len(FAIL))


def certificates(t):
    requests = []
    for p in (2, 3):
        for f in it.product(range(-2, 3), repeat=3):
            if not f[-1]:
                continue
            for k in (1, 2, 3):
                for a in range(-1, p**k+1):
                    for s in range(-1, k+1):
                        requests.append((list(f), p, a, k, s))
    accepted = false = 0
    cache = {}
    for chunk in batches(requests):
        for data, ans in zip(chunk, t.call([('root_cert_ok', x) for x in chunk])):
            f, p, a, k, s = data
            want = (0 <= s < k and 0 <= a < p**k and val(evalp(derivative(f), a), p) == s
                    and val(evalp(f, a), p) >= k+s)
            check(ans.get('out') == want, ('R1-R3', data, ans))
            if ans.get('out'):
                accepted += 1
                key = (tuple(f), p, k+s+2)
                if key not in cache:
                    cache[key] = residue_roots(f, p, k+s+2)
                rs = [x for x in cache[key] if (x-a) % p**(s+1) == 0]
                false += len(rs) != p**s or any((x-a) % p**k for x in rs)
    check(false == 0, ('false root certificate', false))
    show('invented_root_certificates', tried=len(requests), accepted=accepted, false_accepted=false)
    # Equality in strong Hensel is not enough: x^2+3 has no root modulo 8.
    ans = t.call([('root_cert_ok', [[3, 0, 1], 2, 1, 1, 1])])[0]
    check(ans == {'out': False} and not residue_roots([3, 0, 1], 2, 3), 'strong boundary')
    show('strong_hensel_boundary', equality_counterexamples=1, failures=len(FAIL))
    bounds = [([-c, 0, 1], p, 2, int(val(4*c, p))+1)
              for p in (2, 3) for c in range(-12, 13) if c]
    for args, ans in zip(bounds, t.call([('padic_roots', x) for x in bounds])):
        # -4(X^2-c) + 2X(2X) = 4c is an explicit integer Bezout identity.
        check(ans.get('out', [None])[0] == 'OK', ('Bezout depth', args, ans))
    show('bezout_depth', polynomials=len(bounds), failures=len(FAIL))
    local = forgotten = witnesses = 0
    for m in range(1, 19):
        primes = [p for p in range(2, m+1) if m % p == 0 and
                  all(p % d for d in range(2, math.isqrt(p)+1))]
        for c in range(-m, 2*m):
            for n in range(-6, 7):
                for d in range(1, 7):
                    if math.gcd(n, d) != 1:
                        continue
                    congruent = (n-c*d) % m == 0
                    valuations = all(val(n-c*d, p)-val(d, p) >= val(m, p) for p in primes)
                    check(congruent == valuations and (not congruent or math.gcd(d, m) == 1),
                          ('local reconstruction meaning', m, c, n, d))
                    local += 1
    for H in range(1, 13):
        for d in range(1, 8):
            if math.gcd(H, d) != 1:
                continue
            for A in range(-H, H+1):
                c = next(c for c in range(H) if (c*d-A) % H == 0)
                for k in range(-3, 4):
                    q = Q(A+H*k, d)
                    check((q.numerator-c*q.denominator) % H == 0, ('forget', H, A, d, k))
                    forgotten += 1
                e = next(e for e in range(2, H*d+2) if math.gcd(e, H*d) == 1)
                q = c+Q(H, e)
                check((q*d-A)/H != int((q*d-A)/H), ('proper inclusion', H, A, d))
                witnesses += 1
    show('local_and_forget', reduced_pairs=local, ball_rationals=forgotten,
         proper_witnesses=witnesses, failures=len(FAIL))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('suite', choices=('recon', 'matrix', 'roots', 'real', 'extras', 'certificates'))
    args = parser.parse_args()
    signal.alarm(165)
    start = time.monotonic()
    print('target_sha256=' + hashlib.sha256(AUTHOR.read_bytes()).hexdigest(), flush=True)
    t = Target()
    try:
        globals()[args.suite](t)
    finally:
        t.close()
    show('total', failures=len(FAIL), findings=len(FINDINGS), seconds=round(time.monotonic()-start, 2))
    return bool(FAIL)


if __name__ == '__main__':
    sys.exit(main())
