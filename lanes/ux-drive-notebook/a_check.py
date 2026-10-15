from pathlib import Path
import json
import subprocess

lane = Path(__file__).resolve().parent
records = [json.loads(x) for x in (lane/'work/ux/notebook/out/a_roots.records.jsonl').read_text().splitlines()]
checks = 0
for record in records:
    script = '\n'.join(record['driver']) + '\n'
    result = subprocess.run(['timeout', '10', str(lane/'work/build/adf')], input=script,
                            text=True, capture_output=True)
    assert result.stdout.strip() == record['value'], (record, result.stdout)
    assert result.returncode == (record['status'] != 'OK')
    checks += 2
    print('cell', record['cell'], 'driver:', repr(result.stdout.strip()), 'exit', result.returncode)
    for branch in record.get('branches', []):
        if 'O(' in branch['value']:
            n = record['prec']; value = int(branch['value'].split(' + ')[0]); m = 5**n
            assert (value*value+1) % m == 0
            assert value % 5 == int(branch['seed'])
            digits = ''.join(str((value//5**i)%5) for i in reversed(range(n)))
            assert branch['digits'] == '~' + digits
            checks += 3
            print(' root', value, 'modulus', m, 'square+1 quotient', (value*value+1)//m, 'digits', digits)
        else:
            q = int(branch['value'])
            assert q*q == 9 and q % 5 == int(branch['seed'])
            # In modulo 5^k, sum(4*5^i, i=1..k-1)+2 = 5^k-3; positive 3 stays 3.
            expected = '(4)2' if q == -3 else '(0)3'
            assert branch['digits'] == expected
            checks += 2
            print(' exact', q, 'digits', expected)
assert {x*x % 5 for x in range(5)} == {0, 1, 4}
checks += 1
print('PASS', checks, 'checks. Square residues mod 5: 0,1,4; 2 is absent.')
