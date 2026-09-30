"""Store old-code outputs. Seed and distribution are part of the fixture contract."""
import json
import random
import subprocess
import sys
from pathlib import Path
sys.set_int_max_str_digits(0)
rng = random.Random(930505)
primes = [2, 3, 5, 7, 11, 18446744073709551557]
cases = []
for i in range(2000):
    p = primes[i % 6]
    f = ['exp', 'log', 'Log'][rng.randrange(3)]
    N = rng.randint(2, 3000) if i < 50 else rng.randint(-2, 36)
    # Only two full-word-prime large fixtures keep the entire stored file below 1 MB.
    if p == primes[-1] and i >= 12:
        N = rng.randint(-2, 36)
    v = rng.randint(-7, 5)
    ex = rng.randrange(2)
    M = 0 if ex else v + rng.randint(1, 40)
    ud = rng.choice([d for d in range(1, 20) if d % p]) if ex else 1
    un = rng.randrange(1, 40)
    while un % p == 0 or __import__('math').gcd(un, ud) != 1:
        un += 1
    if f == 'exp':
        v = rng.randint(2 if p == 2 else 1, 5)
        M = 0 if ex else v + rng.randint(1, 40)
    elif f == 'log':
        v = 0
        un = ud + p**rng.randint(1, 4) * un
        while __import__('math').gcd(un, ud) != 1:
            un += p
        M = 0 if ex else rng.randint(1, 40)
    if not ex:
        un %= p**(M-v)
        if un == 0:
            v = 0
    if ex and rng.randrange(5) == 0:
        un = -un
    cases.append((f, p, un, ud, v, M, ex, N))
text = ''.join(' '.join(map(str, c)) + '\n' for c in cases)
Path('lanes/f-slice5/stored.in').write_text(text)
r = subprocess.run(['timeout', '180', 'lanes/f-slice5/build/probe-before'], input=text,
                   text=True, capture_output=True, check=True)
rows = r.stdout.splitlines()
assert len(rows) == 2000
out = []
for c, row in zip(cases, rows):
    f, p, un, ud, v, M, ex, N = c
    st, ye, yv, yn, yu, yd, alias, unchanged, canonical, elapsed = row.split()
    assert (alias, unchanged, canonical) == ('1', '1', '1')
    out.append(json.dumps(dict(f=f, N=N, st=int(st),
        x=dict(p=p, un=un, ud=ud, v=v, N=M, exact=ex),
        y=dict(p=p if int(st)==0 else 7, un=int(yu), ud=int(yd), v=int(yv), N=int(yn), exact=int(ye))),
        separators=(',', ':')))
path = Path('tests/ref/vectors/f-slice5/stored.jsonl')
path.write_text('\n'.join(out) + '\n')
assert path.stat().st_size < 1000000
print('calls=2000 bytes=' + str(path.stat().st_size))
