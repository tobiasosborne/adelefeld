from pathlib import Path
import json
import subprocess

lane = Path(__file__).resolve().parent
valid = [
    ('((1) + (0)*i ; 0)', '2'),
    ('((1 +/- 1) + (0)*i ; 0)', '2'),
    ('((-199.5 +/- 0.5) + (0)*i ; 0)', 'real'),
    ('((-0.0625 +/- 0.00006103515625) + (0)*i ; 0)', 'real'),
    ('((2) + (0)*i ; 5 mod 7)', '2'),
    ('((2) + (0 +/- 0.125)*i ; 0)', 'real'),
    ('((2 +/- 0.125) + (0)*i ; 0)', 'real'),
    ('((1) + (1 +/- 0.125)*i ; 0)', '18446744073709551557'),
    ('((0) + (9.064720283654387619255365891433333620343722935447591168372033 +/- 1e-60)*i ; 0)', '2'),
    ('((-130 +/- 1) + (0.25 +/- 0.1)*i ; 0)', 'real'),
]
hostile = [
    ('1', '2', 'UNSUPPORTED'),
    ('(1 ; 0)', '2', 'UNSUPPORTED'),
    ('1 mod 4', '2', 'UNSUPPORTED'),
    ('[1]', 'real', 'UNSUPPORTED'),
    ('((1) + (0)*i ; 0)', '4', 'DOMAIN'),
    ('((1) + (0)*i ; 0)', '0', 'DOMAIN'),
    ('((1) + (0)*i ; 0)', '1', 'DOMAIN'),
    ('((1) + (0)*i ; 0)', '-2', 'DOMAIN'),
    ('((1) + (0)*i ; 0)', '18446744073709551616', 'DOMAIN'),
    ('((1) + (0)*i ; 0)', '18446744073709551615', 'DOMAIN'),
    ('((1) + (0)*i ; 0)', 'two', 'PARSE'),
    ('((1) + (0)*i ; 0)', 'infinity', 'PARSE'),
    ('((1) + (0)*i ; 0)', '2,3', 'PARSE'),
    ('((1) + (0)*i ; 0)', '2.0', 'PARSE'),
    ('((1) + (0)*i ; 0', '4', 'PARSE'),
    ('((1) + (0)*i ; 0', 'two', 'PARSE'),
    ('(1 ; 0)', 'two', 'PARSE'),
    ('1', '4', 'UNSUPPORTED'),
    ('((1 +/- -1) + (0)*i ; 0)', '2', 'PARSE'),
    ('((1) + (0)*i ; 0)', None, 'PARSE'),
]
cmds = [f'local_zeta_factor_at {s} with {v}' for s, v in valid]
cmds += [f'local_zeta_factor_at {s}' + (f' with {v}' if v else '') for s, v, _ in hostile]
(lane / 'driver.cmd').write_text('prec 128\ndigits 40\n' + '\n'.join(cmds) + '\n')
r = subprocess.run(['timeout', '30', str(lane / 'build/adf'), str(lane / 'driver.cmd')],
                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
(lane / 'driver.out').write_text(r.stdout)
(lane / 'driver.stderr').write_text(r.stderr)
lib = subprocess.run(['timeout', '30', str(lane / 'build/driver-library')],
                     input=''.join(v + '\t' + s + '\n' for s, v in valid),
                     stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
assert lib.returncode == 0, lib.stderr
expected = lib.stdout.splitlines() + ['error: ' + st for _, _, st in hostile]
(lane / 'driver.expected').write_text('\n'.join(expected) + '\n')
actual = r.stdout.splitlines()
bad = [{'index': i, 'command': c, 'expected': e, 'actual': actual[i] if i < len(actual) else None}
       for i, (c, e) in enumerate(zip(cmds, expected)) if i >= len(actual) or actual[i] != e]
result = {'commands': len(cmds), 'printed_against_library': len(valid), 'driver_exit': r.returncode,
          'library_exit': lib.returncode, 'lines': len(actual), 'failures': bad}
(lane / 'driver.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result))
raise SystemExit(bool(bad) or len(actual) != len(expected) or r.returncode != 1)
