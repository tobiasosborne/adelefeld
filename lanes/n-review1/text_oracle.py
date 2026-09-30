from fractions import Fraction as Q
from pathlib import Path
from collections import Counter
import importlib.util
import random
import subprocess
import sys
from math import gcd

sys.set_int_max_str_digits(0)
spec = importlib.util.spec_from_file_location('ref', 'proto/text_grammar.py')
ref = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ref)
proc = subprocess.Popen(['timeout', '170', sys.argv[1] if len(sys.argv)>1 else 'lanes/n-review1/text_bridge'],
                        stdin=subprocess.PIPE,
                        stdout=subprocess.PIPE, text=True)
counts = Counter()
fails = []
rng = random.Random(20260930)

def chk(ok, detail):
    counts['checks'] += 1
    if not ok:
        fails.append(detail)
        if len(fails) <= 15:
            print('FAIL', detail, flush=True)

def e(q):
    q = abs(q)
    k = q.numerator.bit_length()-q.denominator.bit_length()
    power = Q(2**k) if k >= 0 else Q(1, 2**(-k))
    return k+1 if q >= power else k

def dyadic_fits(q, bits):
    n, d = abs(q.numerator), q.denominator
    if d & (d-1):
        return False
    while n and n % 2 == 0:
        n //= 2
    return n.bit_length() <= bits

def interval(text):
    parts = text.split('+/-')
    m = Q(parts[0].strip())
    r = Q(parts[1].strip()) if len(parts) == 2 else Q(0)
    return m-r, m+r

def case(form, text, prec=64, digits=20, expected=None, real=None):
    name = {'U':'ucoset', 'I':'idele', 'C':'idclass'}[form]
    data = text if isinstance(text, bytes) else text.encode()
    proc.stdin.write(f'{form} {prec} {digits} {data.hex()}\n')
    proc.stdin.flush()
    output = proc.stdout.readline().rstrip('\n')
    if not output:
        raise RuntimeError('bridge stopped: '+repr(data[:80]))
    counts['calls'] += 1
    counts[form] += 1
    fields, _, printed = output.partition(' TEXT ')
    f = fields.split()
    st, ks, kind = map(int, f[:3])
    if st == 0 and isinstance(text, bytes):
        text = text.decode('ascii')
    cls = ref.classify(data)
    chk((ks == ref.STATUS[cls[1:]] if cls.startswith('!') else ks == 0 and kind == ref.TYPES.index(cls)),
        ('classify', data[:120], output[:160], cls))
    want = ref.canonical(name, data) if expected is None else expected
    wantst = ref.STATUS[want[1:]] if want.startswith('!') else 0
    if st == 1 and wantst == 0 and form != 'U':
        if real is None:
            head = text.split(';')[0][1:].strip()
            lo, hi = interval(head)
        else:
            m, r = real
            lo, hi = m-r, m+r
        a, b = (lo, hi) if lo > 0 else (-hi, -lo)
        chk(e(b)-e(a) >= max(prec, 2), ('unexpected_ND', data[:120], prec, lo, hi, output))
        counts['allowed_ND'] += 1
    else:
        chk(st == wantst, ('status', data[:120], prec, st, wantst, output[:160]))
    if st:
        chk(f[3:] == ['unchanged=1'], ('output_changed', data[:120], output[:160]))
        counts['nonOK'] += 1
        return
    counts['OK'] += 1
    c, N = int(f[3]), int(f[4])
    chk((N == 0 and c in (-1, 1)) or (N > 0 and 1 <= c <= N and gcd(c, N) == 1),
        ('unit_canonical', data[:120], c, N))
    if form == 'U':
        chk(printed == want and int(f[5]) == 0, ('unit_text', data[:120], output[:160], want[:160]))
        return
    lo, hi = map(Q, f[5:7])
    if real is None:
        head = text.split(';')[0][1:].strip()
        a, b = interval(head)
        m, r = (a+b)/2, (b-a)/2
    else:
        m, r = real
        a, b = m-r, m+r
    chk(lo <= a <= b <= hi, ('read_enclosure', data[:120], prec, lo, hi, a, b))
    chk(lo > 0 if form == 'C' else lo > 0 or hi < 0, ('stored_sign', data[:120], lo, hi))
    if dyadic_fits(m, max(prec, 2)) and dyadic_fits(r, 30):
        chk((lo, hi) == (a, b), ('dyadic_tight', data[:120], prec, lo, hi, a, b))
        counts['tight'] += 1
    idx = 8 if form == 'I' else 7
    if form == 'I':
        content = Q(text.split(';')[1].split('*')[0].strip())
        chk(Q(f[7]) == content, ('content', data[:120], f[7], content))
    if printed:
        rt = int(f[idx])
        chk(rt == 0, ('roundtrip_status', data[:120], output[:160]))
        if rt == 0:
            l2, h2 = map(Q, f[idx+1:idx+3])
            chk(l2 <= lo and hi <= h2, ('roundtrip_enclosure', data[:120], lo, hi, l2, h2))
            counts['roundtrips'] += 1
        ph = printed.split(';')[0][1:].strip()
        pl, pu = ref.read_real(ph)
        chk(pl <= lo <= hi <= pu and (pl > 0 if form == 'C' else pl > 0 or pu < 0),
            ('print_enclosure_sign', data[:120], lo, hi, printed[:160]))
        exact_print = ref.print_real((lo+hi)/2, (hi-lo)/2, digits,
                                    'positive' if form == 'C' else 'nonzero')
        chk(ph == exact_print, ('print_Q', data[:120], digits, ph[:160], exact_print[:160]))
        counts['printer_reference'] += 1
    else:
        counts['NULL'] += 1

for form, name in (('U','ucoset'), ('I','idele'), ('C','idclass')):
    for row in Path(f'tests/golden/{name}.tsv').read_text().splitlines():
        if not row or row.startswith('#'):
            continue
        text, expected = row.split('\t')
        for prec in (2, 7, 30, 128):
            case(form, text, prec, expected=expected)
            counts['golden'] += 1

for _ in range(1200):
    shift = rng.randrange(-15, 16)
    scale = Q(2)**shift
    m = Q(rng.randrange(1, 20000), 2**rng.randrange(0, 12))*scale
    r = m * Q(rng.randrange(0, 1025), 1024)
    if rng.randrange(2):
        m = -m
    N = rng.randrange(0, 60)
    c = rng.randrange(-80, 81)
    unit = f'[{c} mod {N}]'
    realtext = ref.fmt_decimal(m)+' +/- '+ref.fmt_decimal(r)
    form = rng.choice('IC')
    text = f'({realtext} ; 3/7 * {unit})' if form == 'I' else f'<{realtext} ; {unit}>'
    for prec in (2, 5, 30, 128):
        case(form, text, prec, rng.choice((1, 2, 6, 20)), real=(m, r))

for sign in (-1, 1):
    for n in (1, 2, 8, 11, 19, 31, 55, 90):
        m, r = Q(sign), 1-Q(1, 10**n)
        realtext = ref.fmt_decimal(m)+' +/- '+ref.fmt_decimal(r)
        for prec in (2, 7, 30, 64, 128, 512):
            case('I', f'({realtext} ; 1 * [1])', prec, 1, real=(m, r))
            if sign > 0:
                case('C', f'<{realtext} ; [1]>', prec, 1, real=(m, r))

seeds = ['[5 mod 6]', '[1]', '(1 +/- 0.25 ; 3/2 * [5 mod 36])', '<1 ; [1]>']
for seed in seeds:
    form = 'U' if seed[0] == '[' else 'I' if seed[0] == '(' else 'C'
    for i in range(len(seed)):
        for bad in (b'\x00', b'\xff', b'(', b']', b'e', b'-', b' ', b'/'):
            b = seed.encode()
            case(form, b[:i]+bad+b[i+1:])
        case(form, seed[:i])
        case(form, seed[:i]+seed[i+1:])
for form, text, expected in (
    ('I', '(0e'+'0'*40+'5 ; 1 * [1])', '!DOMAIN'),
    ('I', '(1e'+'0'*40+'5 ; 1 * [1])', '(100000 ; 1 * [1])'),
    ('I', '(1e'+'9'*40+' ; 0 * [2 mod 4])', '!LIMIT'),
    ('I', '(1e100001 ; 0 * [2 mod 4])', '!LIMIT'),
    ('I', '(1e100001 ; -1 * [2 mod 4])', '!PARSE'),
    ('U', '[1 mod '+'9'*100000+']', None),
    ('U', '[1 mod '+'0'*100000+']', '[1]'),
    ('U', '['+'9'*100000+' mod 1]', '[1 mod 1]'),
    ('C', '<1 ; [1]>'+' '*1048576, '!LIMIT'),
    ('I', '(1 ; 1 * [1])'+' '*1048576, '!LIMIT'),
):
    case(form, text, expected=expected)
    counts['hostile_extra'] += 1

proc.stdin.close()
chk(proc.wait(timeout=5) == 0, 'bridge_exit')
print('counts', dict(counts))
print('failures', len(fails))
raise SystemExit(bool(fails))
