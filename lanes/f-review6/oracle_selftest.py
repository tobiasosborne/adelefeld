"""Cross-check of oracle.py against brute-force enumeration modulo p^H (small p)."""
import sys; sys.path.insert(0, 'lanes/f-review6')
from oracle import *
bad = 0; cnt = 0
for p in [2, 3, 5, 7]:
    for n in range(1, 13):
        s = vp(n, p); c = cfor(p); L = 3 if p > 3 else 5
        H = L + s + c
        P = p**H
        pw = {}
        for b in range(P):
            if b % p: pw.setdefault(pow(b, n, P), []).append(b)
        for U in range(1, p**(s + c + 1)):
            if U % p == 0: continue
            # brute force: branch t has a root iff some b = t mod p^c with b^n = U mod p^H whose
            # residues mod p^L agree (stability) -- compare with lift()
            for t in ([1, 3] if p == 2 else range(1, p)):
                cnt += 1
                y = lift(p, n, U, 1, t, L)
                bs = [b for b in pw.get(U % P, []) if b % (4 if p == 2 else p) == t]
                if y is None:
                    if bs: bad += 1; print("lift None but brute", p, n, U, t)
                else:
                    if not bs or any(b % p**L != y for b in bs):
                        bad += 1; print("lift mismatch", p, n, U, t, y, bs[:3])
print("selftest cases", cnt, "bad", bad)
