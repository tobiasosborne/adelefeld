import hashlib
import json
import pathlib
import re
import subprocess
import sys
from fractions import Fraction

lane = pathlib.Path(__file__).resolve().parent
root = lane.parent.parent
scenario = sys.argv[1].lower()
checks = 0
results = {}

def check(value, label):
    global checks
    checks += 1
    assert value, label

for path in sorted((root / 'ux/ledger/claims').glob(scenario + '*.json')):
    claim = json.loads(path.read_text())
    print('\n' + claim['id'] + ': ' + claim['statement'])
    if not claim['script']:
        print('No script: cannot reproduce FLINT equality through this driver. Proposed, not evidence.')
        continue
    script = '\n'.join(claim['script']) + '\n'
    (lane / (claim['id'] + '.cmd')).write_text(script)
    command = ['timeout', '20', str(root / 'build/adf'), str(lane / (claim['id'] + '.cmd'))]
    run = subprocess.run(command, text=True, capture_output=True)
    print('Command: ' + ' '.join(command))
    print('Input:\n' + script + 'Output:\n' + run.stdout + 'exit=' + str(run.returncode))
    seal = json.loads((root / 'ux/ledger/seals' / path.name).read_text())
    check(run.stdout.splitlines() == seal['actual'], claim['id'] + ' actual lines')
    check(run.returncode == seal['exit'], claim['id'] + ' exit')
    check(hashlib.sha256(path.read_bytes()).hexdigest() == seal['claim_sha256'], claim['id'] + ' hash')
    results[claim['id']] = run.stdout.splitlines()

if scenario == 'a':
    check(format(1, '04b') == '0001' and format(9, '04b') == '1001', 'cos digits')
    check((9 - 1) == 2**3, 'first unequal cos digit')
    check(results['a2-v2-log-3'] == ['2: 4 + O(2^3)'], 'log ball')
    print('Every x in 4 + 8 Z_2 is 4(1 + 2k); its unit factor is odd, hence v_2(x) = 2.')
    values = [int(re.match(r'3: (\d+)', s)[1]) for s in results['a3-exp-3-exp-12-mod-3-8']]
    delta = abs(values[1] - values[0])
    print('Exponential centres:', values, 'difference:', delta)
    check(delta % 9 == 0 and delta % 27 != 0, 'exponential distance')
    for value in values:
        digits = ''.join(str(value // 3**i % 3) for i in reversed(range(8)))
        print('Base-3 digits:', value, digits)
    check(results['a4-sin-3-at-3'] == ['3: 3 + O(3^2)'], 'sin ball')
    check(results['a5-exp-3-exp-12-distance'] ==
          ['3: 4 + O(3^2)', '3: 4 + O(3^2)', '3: 13 + O(3^3)', '3: 22 + O(3^3)'], 'distance child')
    print('13 % 9 = 22 % 9 = 4; 22 - 13 = 9. The 3^2 digit is the first difference.')
else:
    check(results['b1-exp-log-5-is-1'] == ['5: 0', '5: 1'], 'Log then exp')
    primes = [p for p in range(3, 100, 2) if all(p % d for d in range(2, int(p**0.5) + 1))]
    signs = []
    for p in primes:
        roots = [x for x in range(p) if (x*x - 2) % p == 0]
        symbol = 1 if roots else -1
        signs.append(str(symbol))
        check((symbol == 1) == (p % 8 in (1, 7)), 'enumeration at ' + str(p))
        print('p=' + str(p) + ', roots modulo p=' + str(roots) + ', sign=' + str(symbol))
    check(results['b6-two-square-family'] == signs, '24 driver signs against enumeration')
    squares = sorted({x*x % 8 for x in range(8)})
    check(squares == [0, 1, 4], 'square residues modulo 8')
    print('Squares modulo 8:', squares, '; 5 is absent. Odd squares are 1 mod 8.')
    check(all((x*x+1) % 4 != 0 for x in range(4)), 'no root x^2+1 modulo 4')
    check(results['b4-x2-plus-1-root-in-z2'] == ['none'], 'empty complete list')
    check(results['b5-sqrt-1-plus-4z2'] == ['error: NOT_DETERMINED'], 'mixed root domain')
    check(results['b5-2-sqrt-5-plus-8z2'] == ['error: DOMAIN'], 'all outside root domain')
    check(results['b5-1-sqrt-1-plus-8z2'] ==
          ['2 [+1]: 1 + O(2^2); 2 [-1]: 3 + O(2^2)'], 'two root branches')
    factors = [Fraction(6,35), Fraction(1,2), Fraction(1,3), Fraction(5), Fraction(7)]
    product = Fraction(1)
    for f in factors:
        product *= f
    check(product == 1, 'product formula exact multiplication')
    centre, radius = Fraction('0.171428571428571428571428571'), Fraction('1e-27')
    check(abs(centre - Fraction(6,35)) <= radius, 'input ball contains 6/35')
    check(abs(centre * Fraction(35,6) - 1) + radius * Fraction(35,6) <= Fraction('8.4e-27'),
          'printed norm ball encloses exact input norm interval')
    print('6/35 = 2*3/(5*7); the only prime factors are 2, 3, 5, 7.')
    print('Exact product:', product, '; input-centre error:', centre - Fraction(6,35))
    print('Norm interval fits in 1 +/- 8.4e-27. This is an enclosure, not a computed exact equality.')
print('\nIndependent assertions:', checks)
