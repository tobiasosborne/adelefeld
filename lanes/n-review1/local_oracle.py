from fractions import Fraction as Q
from collections import Counter
import subprocess
import sys

pbig = 2**64 - 59
proc = subprocess.Popen(['timeout', '175', sys.argv[1] if len(sys.argv)>1 else 'lanes/n-review1/local_bridge'],
                        stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)
counts = Counter()
failures = []

def call(op, x, arg=0, alias=0):
    p, exact, v, N, u = x
    proc.stdin.write(f'{op} {p} {exact} {v} {N} {u} {arg} {alias}\n')
    proc.stdin.flush()
    line = proc.stdout.readline()
    if not line:
        raise RuntimeError(f'bridge stopped: {op} {x} {arg}')
    counts[op] += 1
    t = line.split()
    return int(t[0]), t[1:]

def ball(t):
    return tuple(map(int, t[:4])) + (Q(t[4]),)

def chk(ok, detail):
    counts['checks'] += 1
    if not ok:
        failures.append(detail)
        if len(failures) <= 20:
            print('FAIL', detail, flush=True)

def pp(p, v):
    return Q(p**v) if v >= 0 else Q(1, p**(-v))

def val(q, p):
    if not q:
        return 10**100
    a, b, v = abs(q.numerator), q.denominator, 0
    while a % p == 0:
        a //= p
        v += 1
    while b % p == 0:
        b //= p
        v -= 1
    return v

def mod(q, P):
    return q.numerator * pow(q.denominator, -1, P) % P

def inside(q, b):
    p, exact, v, N, u = b
    d = q - pp(p, v)*u
    return d == 0 if exact else val(d, p) >= N

def omega(p, r, n):
    P = p**n
    a = r % p
    for _ in range(n):
        a = pow(a, p, P)
    return a

def split_expected(x, n):
    p, exact, v, N, u = x
    r = mod(u, 4 if p == 2 else p)
    sign = -1 if (r == 3 if p == 2 else r == p-1) else 1
    rational = p == 2 or r in (1, p-1)
    n = max(n, 1)
    k = n if exact else N-v
    om = sign if rational else omega(p, r, max(n, k))
    W = (p, 1, 0, 0, Q(sign)) if rational else (p, 0, 0, n, Q(om % p**n))
    U = (p, 1, 0, 0, u/sign) if exact and rational else (p, 0, 0, k, Q(mod(u, p**k)*pow(om, -1, p**k) % p**k))
    return r, W, U

for p in (2, 3, 5, 7, 11, pbig):
    residues = range(1, min(p, 30)) if p != pbig else (1, 2, 3, 17, p-1)
    for r in residues:
        for n in (-3, 0, 1, 2, 3, 7, 17, 33):
            st, t = call('T', (p, 1, 0, 0, Q(r)), n)
            rational = p == 2 or r in (1, p-1)
            want = (p, 1, 0, 0, Q(-1 if p != 2 and r == p-1 else 1)) if rational else (
                p, 0, 0, max(n, 1), Q(omega(p, r, max(n, 1))))
            chk(st == 0 and ball(t) == want, ('teich', p, r, n, st, t, want))

for p, maxrel in ((2, 5), (3, 3), (5, 2), (7, 2), (11, 1), (pbig, 2)):
    for rel in range(1, maxrel+1):
        units = [a for a in range(1, p**rel) if a % p] if p != pbig else [1, 2, 3, p-1, p+1, p**rel-1]
        units = sorted(set(a for a in units if a < p**rel))
        for a in units:
            for v in (-3, -1, 0, 2):
                x = (p, 0, v, v+rel, Q(a))
                samples = range(p*p) if p < 12 else (0, 1, 2, 3, p-1, p)
                points = [pp(p, v)*Q(a+p**rel*t) for t in samples]
                for n in (0, 1, rel, rel+3):
                    st, t = call('S', x, n)
                    if p == 2 and rel == 1:
                        chk(st == 1 and t[:2] == ['777', '777'], ('splitstatus', x, n, st, t))
                        continue
                    r, W, U = split_expected(x, n)
                    chk(st == 0 and int(t[0]) == v and int(t[1]) == r and ball(t[2:]) == W
                        and ball(t[7:]) == U, ('split', x, n, st, t, W, U))
                    for alias in (1, 2):
                        ast, at = call('S', x, n, alias)
                        chk((ast, at) == (st, t), ('split_alias', x, n, alias, ast, at, st, t))
                    depth = rel+2
                    om = (-1 if r == 3 else 1) if p == 2 else omega(p, r, depth)
                    target = p**rel
                    image = [mod(q/pp(p, v), p**depth)*pow(om, -1, p**depth) % p**depth for q in points]
                    chk(all((z-int(U[4])) % target == 0 for z in image), ('split_points', x, image, U))
                    counts['split_points'] += len(image)
                    if p < 12:
                        chk(len(set((z-int(U[4]))//target % p**2 for z in image)) == p**2,
                            ('split_surjectivity', x, U))
                st, t = call('F', x)
                want = Q(mod(Q(a), p**(-v)), p**(-v)) if v < 0 else Q(0)
                if x[3] < 0:
                    chk(st == 1 and Q(t[0]) == 17, ('fracstatus', x, st, t))
                else:
                    chk(st == 0 and Q(t[0]) == want, ('frac', x, st, t, want))
                    chk(all(val(q-want, p) >= 0 for q in points), ('frac_points', x, want))
                    counts['frac_points'] += len(points)
                for k in (-1, 0, 1, rel, rel+1):
                    st, t = call('U', x, k)
                    wantst = 7 if k < 0 else 1 if k > rel else 0
                    chk(st == wantst and int(t[0]) == (a % p**k if st == 0 else 17),
                        ('unit', x, k, st, t))
                exponents = (-12, -9, -4, -2, -1, 0, 1, 2, 3, 4, 6, 8, 9, 12)
                for k in exponents:
                    st, t = call('P', x, k)
                    B = ball(t)
                    vals = [q**k for q in points]
                    chk(st == 0 and all(inside(q, B) for q in vals), ('power_enclosure', x, k, st, B))
                    counts['power_points'] += len(vals)
                    if k != 0:
                        tight = min(val(q-vals[0], p) for q in vals[1:])
                        chk(B[3] == tight, ('power_tight', x, k, B, tight))
                    else:
                        chk(B == (p, 1, 0, 0, Q(1)), ('powerzero', x, B))
                    ast, at = call('P', x, k, 1)
                    chk((ast, at) == (st, t), ('power_alias', x, k, ast, at, st, t))
                    if v == 0 and a <= 3 and abs(k) <= 4:
                        rst, rt = call('R', x, k)
                        RB = ball(rt)
                        chk(rst == 0 and inside(pp(p, B[2])*B[4], RB) and (B[1] or B[3] >= RB[3]),
                            ('repeated_mul', x, k, st, B, rst, RB))

for p in (2, 3, 5, 7, pbig):
    for num in (-19, -3, -1, 1, 3, 19):
        for den in (1, 3, 7):
            if num % p == 0 or den % p == 0:
                continue
            for v in (-4, -1, 0, 3):
                x = (p, 1, v, 0, Q(num, den))
                for n in (0, 1, 4, 9):
                    r, W, U = split_expected(x, n)
                    st, t = call('S', x, n)
                    chk(st == 0 and int(t[0]) == v and int(t[1]) == r and ball(t[2:]) == W
                        and ball(t[7:]) == U, ('split_exact', x, n, st, t, W, U))
                st, t = call('F', x)
                want = Q(mod(x[4], p**(-v)), p**(-v)) if v < 0 else Q(0)
                chk(st == 0 and Q(t[0]) == want, ('frac_exact', x, st, t, want))
                for k in (-9, -2, -1, 0, 1, 2, 9):
                    st, t = call('P', x, k)
                    chk(st == 0 and ball(t)[1] == 1 and pp(p, ball(t)[2])*ball(t)[4]
                        == (pp(p, v)*x[4])**k, ('power_exact', x, k, st, t))

for p in (2, 3, pbig):
    for exact, N in ((1, 0), (0, -3), (0, 0), (0, 3)):
        x = (p, exact, 0, N, Q(0))
        st, t = call('S', x, 10)
        chk(st == (7 if exact else 1) and t[:2] == ['777', '777'], ('zero_split', x, st, t))
        st, t = call('U', x, 0)
        chk(st == (7 if exact else 1) and int(t[0]) == 17, ('zero_unit', x, st, t))
        st, t = call('F', x)
        chk(st == (1 if not exact and N < 0 else 0), ('zero_frac', x, st, t))
        for k in (-3, 0, 1, 2, 3):
            st, t = call('P', x, k)
            wantst = (6 if exact else 2) if k < 0 else 0
            chk(st == wantst, ('zero_powstatus', x, k, st, t))
            if st == 0:
                want = (p, 1, 0, 0, Q(1)) if k == 0 else (p, exact, 0, N*k, Q(0))
                chk(ball(t) == want, ('zero_pow', x, k, t, want))

E, B, L = 2**60, 2**26, 2**63-1
limit_cases = [
    ('T', (5, 1, 0, 0, Q(2)), B//3+1, 10),
    ('T', (5, 1, 0, 0, Q(1)), L, 0),
    ('T', (2, 1, 0, 0, Q(1)), L, 0),
    ('T', (5, 1, 0, 0, Q(0)), L, 7),
    ('S', (5, 0, 0, E, Q(2)), 1, 10),
    ('S', (5, 0, 0, E, Q(1)), L, 0),
    ('S', (5, 0, 0, E, Q(4)), 1, 10),
    ('S', (2, 0, E+1, E+2, Q(1)), 1, 1),
    ('S', (3, 0, E+1, E+2, Q(1)), 1, 10),
    ('F', (2, 0, 0, -E-1, Q(0)), 0, 10),
    ('F', (2, 1, -E, 0, Q(1)), 0, 10),
    ('U', (2, 1, E+1, 0, Q(1)), 1, 0),
    ('U', (2, 0, -L-1, L, Q(1)), 1, 0),
    ('U', (2, 0, 0, 1, Q(1)), L, 10),
    ('P', (2, 0, 0, E+1, Q(0)), 0, 10),
    ('P', (2, 1, E, 0, Q(1)), 2, 10),
    ('P', (2, 0, 0, E, Q(1)), 2, 10),
    ('P', (3, 0, 0, E, Q(1)), 2, 0),
    ('P', (2, 0, 0, 0, Q(0)), -L-1, 2),
    ('P', (2, 0, 0, 0, Q(0)), L, 0),
    ('P', (2, 0, 0, 0, Q(0)), 1, 0),
    ('P', (2, 1, 0, 0, Q(-1)), -L-1, 0),
    ('P', (3, 1, 0, 0, Q(-1)), -L, 0),
    ('P', (5, 1, 0, 0, Q(2)), -L-1, 10),
]
for op, x, arg, wantst in limit_cases:
    st, t = call(op, x, arg)
    chk(st == wantst, ('limit_status', op, x, arg, st, wantst, t))
    counts['limit_cases'] += 1
    if st and op == 'S':
        chk(t[:2] == ['777', '777'], ('limit_scalar_outputs', op, x, arg, t))
    if op == 'P':
        ast, at = call(op, x, arg, 1)
        if st:
            chk(ast == st and ball(at) == x, ('limit_alias_output', x, arg, ast, at))
        else:
            chk((ast, at) == (st, t), ('limit_alias_OK', x, arg, ast, at, st, t))

proc.stdin.close()
chk(proc.wait(timeout=5) == 0, 'bridge_exit')
print('counts', dict(counts))
print('failures', len(failures))
raise SystemExit(bool(failures))
