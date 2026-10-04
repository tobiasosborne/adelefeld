"""New restricted centres, checked against the independent exp-inversion oracle."""
import sys
import json
from math import gcd
from pathlib import Path

sys.path.insert(0, 'proto')
sys.path.insert(0, 'lanes/f-review13')
import idlog_checks as series
import il_oracle as exponential

rows = []
images = units = nontrivial = 0
checked_images = {}
for p in (2, 3, 5, 7):
    for k in (1, 2, 3):
        q = p**k
        # No unit other than +-1 exists mod 2, 3 or 4. Keep those boundary cases too.
        cs = [c for c in range(2, q - 1) if gcd(c, q) == 1][:2]
        if not cs:
            cs = [3 if p == 2 else 2]
        for cofactor in (1, 11):
            M = q * cofactor
            for c in cs:
                if c >= M or gcd(c, M) != 1:
                    continue
                for m in (-2, 0, 2):
                    for a, b in ((1, 1), (p + 1, 1), (1, p + 1)):
                        r = series.rational((a * p**max(m, 0), b * p**max(-m, 0)))
                        E = series.image_exponent(p, M)
                        H = 5
                        key = (p, *r, c, M, H)
                        im, count = series.enumerated_image(p, r, c, M, H)
                        other = exponential.image(p, *r, c, M, H)
                        assert im == other == exponential.predicted(p, *r, c, M, H)
                        checked_images[key] = other
                        for N in sorted({-2, 0, 1, 2, 3, E, E + 1, 6}):
                            want = series.expected_ball(p, r, c, M, N)
                            _, an, bn = exponential.unit_part(*r, p)
                            L = want['exponent']
                            other_E = max(exponential.v(M, p), exponential.dd(p))
                            assert not want['exact'] and want['image_exponent'] == other_E
                            assert L == min(N, other_E)
                            centre = 0 if L <= (2 if p == 2 else 1) else (
                                exponential.Log(an * c, bn, p, H) % p**L)
                            assert want['center'] == centre, (p, r, c, M, N, want, centre)
                            assert {z % p**max(0, L) for z in other} == {centre}
                            row = dict(p=p, r=list(r), c=c, M=M, requested=N, expected=want)
                            complete = cofactor == 1 or (p == 2 and k == 1)
                            if m == 0 and complete and N == 6:
                                row.update(H=H, image=sorted(im), enumerated_units=count)
                                images += 1
                                units += count
                            nontrivial += c % q not in (1, q - 1)
                            rows.append(row)

path = Path('tests/ref/vectors/f-repair8/local.jsonl')
temporary = path.with_suffix('.jsonl.tmp')
with temporary.open('w') as f:
    for row in rows:
        f.write(json.dumps(row, sort_keys=True, separators=(',', ':')) + '\n')
temporary.replace(path)
print(json.dumps(dict(rows=len(rows), non_pm1_rows=nontrivial, complete_images=images,
                      enumerated_units=units, image_cases_cross_checked=len(checked_images),
                      oracle_disagreements=0)))
