#!/usr/bin/env python3
"""Lane q-review3, hunt 4: an own printer of conventions 9.4 (adf_qclass row, finite part row) and 9.5
(real balls, unconstrained), compared with adf_qclass_get_str on reduced values.
Usage: python3 printer.py --cases N --seed S"""
import argparse, os, random, sys, time
from fractions import Fraction as F

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.set_int_max_str_digits(0)
from check import gen_lift, gen_pieces, case_line, run, parse, stored_of  # noqa: E402


def X(y):
    """floor(log10 |y|), y != 0, exact."""
    y = abs(F(y))
    n, d = y.numerator, y.denominator
    e = len(str(n)) - len(str(d))
    while F(10) ** e > y:
        e -= 1
    while F(10) ** (e + 1) <= y:
        e += 1
    return e


def ceil2(E, k=2):
    s = F(10) ** (X(E) - k + 1)
    q = E / s
    return F(-((-q.numerator) // q.denominator)) * s


def round_q(y, q):
    s = F(10) ** q
    t = y / s
    fl = t.numerator // t.denominator
    rem = t - fl
    if 2 * rem < 1:
        r = fl
    elif 2 * rem > 1:
        r = fl + 1
    else:
        r = fl if fl % 2 == 0 else fl + 1
    return r * s


def is_decimal_digits(y, n):
    if y == 0:
        return True
    d = y.denominator
    for p in (2, 5):
        while d % p == 0:
            d //= p
    if d != 1:
        return False
    D, E = dec(y)
    return len(str(D)) <= n


def dec(y):
    """|y| = D 10^E with D not divisible by 10, y a nonzero decimal."""
    y = abs(y)
    E = 0
    while y.denominator != 1:
        y *= 10
        E -= 1
    D = y.numerator
    while D % 10 == 0:
        D //= 10
        E += 1
    return D, E


def fmt(y):
    if y == 0:
        return "0"
    sign = "-" if y < 0 else ""
    D, E = dec(y)
    ds = str(D)
    k = len(ds)
    Xv = E + k - 1
    if -4 <= Xv <= 20:
        if E >= 0:
            s = ds + "0" * E
        else:
            p = k + E  # digits before the point
            if p > 0:
                s = ds[:p] + "." + ds[p:]
            else:
                s = "0." + "0" * (-p) + ds
        return sign + s
    s = ds[0] + ("." + ds[1:] if k > 1 else "") + "e" + str(Xv)
    return sign + s


def real_str(mid, rad, n):
    if rad == 0 and is_decimal_digits(mid, n):
        return fmt(mid), mid, mid
    if mid == 0:
        R = ceil2(rad)
        return "0 +/- " + fmt(R), -R, R
    q = X(mid) - n + 1
    if rad > 0:
        q = max(q, X(rad) - 1)
    while True:
        M = round_q(mid, q)
        R = ceil2(rad + abs(M - mid))
        qq = X(R) - 1 if M == 0 else max(X(M) - n + 1, X(R) - 1)
        if (M / F(10) ** qq).denominator != 1:
            q = qq
            continue
        break
    return fmt(M) + " +/- " + fmt(R), M - R, M + R


def qrat(x):
    x = F(x)
    return str(x.numerator) if x.denominator == 1 else "%d/%d" % (x.numerator, x.denominator)


def fin_str(A, H, d):
    a = F(A, d)
    if H == 0:
        return qrat(a)
    return qrat(a) + " mod " + qrat(F(H, d))


def union_str(raw, n):
    items = []
    for (m, rho, A, H, d) in raw:
        rs, lo, hi = real_str(m, rho, n)
        items.append(((lo, hi, H, A), "(" + rs + " ; " + fin_str(A, H, d) + ")", (m - rho, m + rho)))
    items.sort(key=lambda t: t[0])
    out, keys = [], []
    for k, s, ex in items:
        if keys and keys[-1] == k:
            # equal printed keys: the texts must be equal, so dropping one keeps the set
            assert out[-1] == s, (out[-1], s)
            continue
        keys.append(k)
        out.append(s)
    return "union(" + ", ".join(out) + ") + Q", items


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cases', type=int, default=500)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--harness', default=os.path.join(HERE, 'harness'))
    args = ap.parse_args()
    rng = random.Random(args.seed)
    lines, digs = [], []
    for i in range(args.cases):
        n = [1, 3, 20, 1000, 2, 5][i % 6]
        prec = rng.choice([2, 5, 10, 20, 53, 128])
        if rng.random() < 0.3:
            form, pcs = gen_pieces(rng)
        else:
            form, pcs = 0, [gen_lift(rng)]
        lines.append(case_line(form, 10 ** 9, prec, n, 0, pcs))
        digs.append(n)
    t0 = time.time()
    out, _ = run(args.harness, lines)
    recs = parse(out)
    bad = same = reorder = dupl = enc_fail = 0
    for rec, n, ln in zip(recs, digs, lines):
        if 'bad' in rec or rec['R'][0] != 0:
            continue
        out_s, raw = stored_of(rec)
        want, items = union_str(raw, n)
        got = rec['T']
        # enclosure by the printer: printed interval contains the stored interval
        for k, s, ex in items:
            if not (k[0] <= ex[0] and ex[1] <= k[1]):
                enc_fail += 1
        stored_order = [s for k, s, ex in [(None, it[1], None) for it in
                        [(None, union_str([r], n)[0][6:-5]) for r in raw]]]
        if [it[1] for it in sorted(items, key=lambda t: t[0])] != stored_order:
            reorder += 1
        if len(set(it[0] for it in items)) < len(items):
            dupl += 1
        if got != want:
            bad += 1
            if bad <= 5:
                print('MISMATCH digits', n)
                print(' got ', got[:600])
                print(' want', want[:600])
                print(' line', ln[:400])
        else:
            same += 1
    print('seed %d: %d values compared, %d equal, %d differ; printed order differs from stored order in %d;'
          ' printed duplicates removed in %d; printed interval not enclosing stored in %d; %.1f s'
          % (args.seed, same + bad, same, bad, reorder, dupl, enc_fail, time.time() - t0))
    return 1 if bad or enc_fail else 0


if __name__ == '__main__':
    sys.exit(main())
