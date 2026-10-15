from fractions import Fraction as F
from math import gcd
from pathlib import Path
import json
import re
import subprocess

lane = Path(__file__).resolve().parent
record_path = lane/'work/ux/notebook/out/b_which_rational.records.jsonl'
records = [json.loads(x) for x in record_path.read_text().splitlines()]
checks = 0
def check(ok, label):
    global checks
    assert ok, label
    checks += 1
    print('PASS', label)

def digits(s, p):
    return [int(x) for x in s.split()] if p > 10 else [int(x) for x in s]

def integer(ds, p):
    ans = 0
    for d in ds:
        ans = p*ans + d
    return ans

def power(p, n):
    return F(p)**n

def expansion(s, p):
    if s.startswith('~'):
        s = s[1:]; fraction = s.split('.')[1] if '.' in s else ''
        return integer(digits(s.replace('.', ''), p), p) * power(p, -len(digits(fraction, p)))
    m = re.fullmatch(r'\(([^)]+)\)(.*)', s)
    period, rest = m.groups(); rest = rest.strip()
    frac = rest.split('.')[1] if '.' in rest else ''
    all_digits = digits(rest.replace('.', ''), p)
    per = digits(period, p); shift = len(digits(frac, p))
    return (F(integer(all_digits, p)) +
            F(integer(per, p)*p**len(all_digits), 1-p**len(per))) * power(p, -shift)

def valuation(q, p):
    v = 0; n = q.numerator; d = q.denominator
    while n and n % p == 0: n //= p; v += 1
    while d % p == 0: d //= p; v -= 1
    return v

for r in records:
    result = subprocess.run(['timeout', '10', str(lane/'work/build/adf')],
                            input='\n'.join(r['driver'])+'\n', text=True, capture_output=True)
    check(result.stdout.strip() == r['value'], f"cell {r['cell']} driver value {result.stdout.strip()}")
    check(result.returncode == (r['status'] != 'OK'), f"cell {r['cell']} exit {result.returncode}")
    if r.get('dump'):
        loaded = subprocess.run(['timeout', '10', str(lane/'work/build/adf')],
            input='load '+r['dump']+'\n', text=True, capture_output=True)
        check(loaded.stdout.strip() == r['value'], f"cell {r['cell']} dump loads to same text")
    q = F(146) if r['cell'] == 1 else F(1,7) if r['cell'] == 3 else F(22,7)
    for pl in r.get('places', []):
        p = pl['p']; e = expansion(pl['digits'], p)
        if pl['k'] is None:
            check(e == q, f"cell {r['cell']} p={p}: exact expansion {pl['digits']} = {e}")
        else:
            difference = q-e
            check(not difference or valuation(difference,p) >= pl['k'],
                  f"cell {r['cell']} p={p}: finite expansion {pl['digits']} = {e}")
        check(pl['v'] == valuation(q,p), f"cell {r['cell']} p={p}: valuation {pl['v']}")
    if r['kind'] == 'rat':
        midpoint, radius = [F(x) for x in r['real'].split(' +/- ')]
        check(abs(midpoint-q) <= radius, f"cell {r['cell']}: real printed interval contains 22/7")

for A,B,m,c in [(30,15,1000,146),(40,30,1000,146),(400,300,1000,146),(3,1001,2003,1)]:
    candidates = [F(n,d) for d in range(1,B+1) for n in range(-A,A+1)
                  if gcd(n,d)==1 and (n-c*d)%m == 0]
    print('Enumeration', (A,B,m,c), 'count', len(candidates), 'values', ', '.join(map(str,candidates)))
    check(len(candidates) > 1 if A == 400 else candidates == [F(1) if m == 2003 else F(22,7)],
          f'complete enumeration A={A}, B={B}, m={m}')

check(7*146 == 1022 and 1022%1000 == 22, '7*146 = 1022 = 22 mod 1000')
check(2*30*15 == 900 < 1000 and 2*40*30 == 2400 > 1000, 'both uniqueness-bound products')
lo, hi, a = F(313,100), F(315,100), F(1,7)
check(lo <= a+3 <= hi and a+2 < lo and a+4 > hi, 'only k=3 meets [3.13,3.15]')
# The displayed 0.011 radius is an outward print. It still admits exactly the same progression point.
check(F(314,100)-F(11,1000) <= a+3 <= F(314,100)+F(11,1000), 'printed real ball contains 22/7')
check(not any('cert' in k for r in records for k in r), 'no certificate field in any of 8 records')
check(not any('author' in r or 'accepted' in r for r in records), 'no author or acceptance state in 8 records')
print('PASS', checks, 'checks')
