"""Print the repaired F1 diameters using the unchanged review models."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path('lanes/m5-review1').resolve()))
from checks import corners, local, mp, results, row, run, width
s = mp.mpc('-.75')
pairs = [(1, 1), (5, 4)]
lines = run([row(i, 0, q, n, 0, s, -12, prec=160) for i, (q, n) in enumerate(pairs)])
for (q, n), line in zip(pairs, lines):
    values = [local(q, n, 0, 1, t)[0] for t in corners(s, -12)]
    sep = max(max(v.real for v in values) - min(v.real for v in values),
              max(v.imag for v in values) - min(v.imag for v in values))
    status, _, _, ball = results(line)[0]
    print(q, n, 'status', status, 'diameter', mp.nstr(width(ball), 30),
          'sampled diameter', mp.nstr(sep, 30), 'ratio', mp.nstr(width(ball) / sep, 20))
