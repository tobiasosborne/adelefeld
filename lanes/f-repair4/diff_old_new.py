"""f-repair4: differential run of the reviewer's harness h over the old library (build-before) and the new one
(build), on the cases of attack1 (seed argv[1]). Lines must be identical except: (a) a ball input, n >= 2, a value
request (S, Q, A) with Nreq < E (N-D14: exponent min(N, E)); (b) old LIMIT, new OK (F1: a power no longer tested
where it is not formed). Every other difference is printed."""
import json, subprocess, sys
sys.path.insert(0, 'lanes/f-review6')
from oracle import vp
f = sys.argv[2]
cases = json.load(open(f + '.json'))
inp = open(f).read()
run = lambda h: subprocess.run(['timeout', '200', h], input=inp, capture_output=True, text=True).stdout.splitlines()
old, new = run('lanes/f-repair4/build-before/h'), run('lanes/f-repair4/build/h')
assert len(old) == len(new) == len(cases), (len(old), len(new), len(cases))
same = nd14 = f1 = other = 0
for (mode, x, n, sd, Nreq, cap), a, b in zip(cases, old, new):
    if a == b: same += 1; continue
    if x['kind'] == 'ball' and n >= 2 and mode in 'SQA':
        s = vp(n, x['p']); j = x['m'] // n; E = x['M'] - s - (n - 1) * j
        if Nreq < E: nd14 += 1; continue
    if a.startswith('LIMIT') and b.startswith('OK'): f1 += 1; continue
    other += 1; print('UNEXPECTED', mode, x, n, sd, Nreq, cap, '|', a[:150], '|', b[:150])
print('cases', len(cases), 'identical', same, 'differ by N-D14 (ball, Nreq < E)', nd14, 'old LIMIT -> new OK', f1,
      'other', other)
