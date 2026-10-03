"""Attack B4: the status of adf_lball_roots against the statuses of adf_lball_root_seed over all valid seeds
(LIMIT iff some branch is LIMIT), for inputs whose Log is exact (unit centre +-1) and N beyond the power bound,
and exact inputs with rational branches; outputs untouched on LIMIT."""
import sys, random
from oracle import *
from attack_roots import parse_roots

def main():
    lines, meta = [], []
    BIG = [2**40, 2**60, 2**63 - 1, 2**27]
    for p in (2, 3, 5, 7, 13, 65537):
        for n in (2, 3, 4, 6, 12, 16):
            for cen in (1, -1, 4, 8):
                for kind in ('exact', 'ball'):
                    for N in BIG + [5]:
                        if kind == 'exact': xf = fields_exact(p, F(cen))
                        else:
                            M = rng_M = 2**40
                            if cen < 0: continue   # a canonical ball centre is p^M - 1: too big to write
                            xf = '%d 1 0 %d 0' % (cen, M) if cen % p else None
                            if xf is None: continue
                        lines.append('A %d %s %d %d %d' % (p, xf, n, N, 100)); meta.append((p, xf, n, N))
    H = Harness()
    if os.environ.get('DUMP'): open(os.environ['DUMP'],'w').write('\n'.join(lines)+'\n'); return
    out = H.run(lines)
    l2, i2 = [], []
    for k, o in enumerate(out):
        p, xf, n, N = meta[k]
        cen = F(int(xf.split()[0]), int(xf.split()[1]))
        valid = [t for t in ((1, 3) if p == 2 else range(1, p)) if branch_exists(p, n, cen, t)]
        for t in valid:
            l2.append('S %d %s %d %d %d' % (p, xf, n, t, N)); i2.append((k, t))
    out2 = H.run(l2)
    per = {}
    for (k, t), o in zip(i2, out2):
        per.setdefault(k, []).append(o.split()[0])
    bad = 0; stats = {}
    for k, o in enumerate(out):
        st = o.split()[0]
        if 'TOUCHED' in o.replace('UNTOUCHED', ''): bad += 1; print('TOUCHED', meta[k], o[:100]); continue
        sts = [x for x in per.get(k, []) if x != 'DOMAIN']
        want = 'DOMAIN' if not sts else ('LIMIT' if 'LIMIT' in sts else ('OK' if all(x == 'OK' for x in sts) else sts[0]))
        stats[(st, want)] = stats.get((st, want), 0) + 1
        if st != want:
            bad += 1
            if bad < 15: print('DIFF', meta[k], 'roots', o[:80], 'branches', per.get(k))
    print('lists', len(out), 'seeded calls', len(l2), 'differences', bad, stats)

main()
