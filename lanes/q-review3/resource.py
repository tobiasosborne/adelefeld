#!/usr/bin/env python3
"""Lane q-review3, hunt 3: resource inputs, one harness process per case, each timed (wall clock),
each under a 70 s timeout. Usage: python3 resource.py [--harness PATH] [--as BYTES]"""
import argparse, os, subprocess, sys, time
from fractions import Fraction as F
sys.set_int_max_str_digits(0)

HERE = os.path.dirname(os.path.abspath(__file__))
LONG_MAX = (1 << 63) - 1
CAP = 2097152


def P(mid_m, mid_e, rad_m, rad_e, c, N, bk=0):
    c, N = F(c), F(N)
    return "%d %d %d %d %d %d %d %d %d" % (mid_m, mid_e, rad_m, rad_e, c.numerator, c.denominator,
                                           N.numerator, N.denominator, bk)


def C(limit, prec, pieces, form=0, alias=1):
    return "C %d %d %d 0 %d %d %s" % (form, limit, prec, alias, len(pieces), " ".join(pieces))


CASES = [
    # (name, line, expected status, note)
    ("B=10^6 point, limit 10^6", C(10**6, 53, [P(1, -2, 0, 0, F(1, 3), F(1, 10**6))]), 0),
    ("B=10^6 point, limit 10^6-1", C(10**6 - 1, 53, [P(1, -2, 0, 0, F(1, 3), F(1, 10**6))]), 10),
    ("B=10^6 point, limit LONG_MAX", C(LONG_MAX, 53, [P(1, -2, 0, 0, F(1, 3), F(1, 10**6))]), 0),
    ("B=10^30, limit LONG_MAX", C(LONG_MAX, 53, [P(1, -2, 0, 0, 0, F(1, 10**30))]), 10),
    ("B=2^5000, limit LONG_MAX", C(LONG_MAX, 53, [P(1, -2, 0, 0, 0, F(1, 2**5000))]), 10),
    ("B=2^5000 numerator 2^5000+1", C(LONG_MAX, 53, [P(1, -2, 0, 0, 0, F(2**5000 + 1, 2**5000))]), 10),
    ("A=2^5000+1, B=2, point", C(10, 53, [P(1, -2, 0, 0, F(1, 7), F(2**5000 + 1, 2))]), 0),
    ("width 10^6, N=1, limit 10^6+1", C(10**6 + 1, 53, [P(1, -1, 10**6 // 2, 0, F(1, 3), 1)]), 0),
    ("width 10^6, N=1, limit 10^6", C(10**6, 53, [P(1, -1, 10**6 // 2, 0, F(1, 3), 1)]), 10),
    ("width 10^6, N=0, limit 10^6+1", C(10**6 + 1, 53, [P(1, -1, 10**6 // 2, 0, F(1, 3), 0)]), 0),
    ("width 2^100 (>10^30), limit LONG_MAX", C(LONG_MAX, 53, [P(0, 0, 1, 99, 0, 1)]), 10),
    ("width 2^100, B=3, limit LONG_MAX", C(LONG_MAX, 53, [P(0, 0, 1, 99, 0, F(1, 3))]), 10),
    ("width 2^62 (count 2^62+1), limit LONG_MAX", C(LONG_MAX, 53, [P(0, 0, 1, 61, F(1, 3), 0)]), 'abort?'),
    ("limit 1, point", C(1, 53, [P(1, -2, 0, 0, F(1, 3), 0)]), 0),
    ("limit 0, point", C(0, 53, [P(1, -2, 0, 0, F(1, 3), 0)]), 10),
    ("limit -1, point", C(-1, 53, [P(1, -2, 0, 0, F(1, 3), 0)]), 10),
    ("limit LONG_MIN, point", C(-LONG_MAX - 1, 53, [P(1, -2, 0, 0, F(1, 3), 0)]), 10),
    ("prec 2", C(10, 2, [P(3, -2, 1, -3, F(1, 3), 2)]), 0),
    ("prec 1", C(10, 1, [P(3, -2, 1, -3, F(1, 3), 2)]), 0),
    ("prec 0", C(10, 0, [P(3, -2, 1, -3, F(1, 3), 2)]), 0),
    ("prec -5", C(10, -5, [P(3, -2, 1, -3, F(1, 3), 2)]), 0),
    ("prec LONG_MIN", C(10, -LONG_MAX - 1, [P(3, -2, 1, -3, F(1, 3), 2)]), 0),
    ("prec cap, dyadic piece", C(10, CAP, [P(1, -2, 1, -3, 0, 2)]), 0),
    ("prec cap, centre 1/3", C(10, CAP, [P(3, -2, 1, -3, F(1, 3), 2)]), 'see'),
    ("prec cap-64, centre 1/3", C(10, CAP - 64, [P(3, -2, 1, -3, F(1, 3), 2)]), 0),
    ("prec cap+1", C(10, CAP + 1, [P(1, -2, 1, -3, 0, 2)]), 10),
    ("prec LONG_MAX", C(10, LONG_MAX, [P(1, -2, 1, -3, 0, 2)]), 10),
    ("mid exponent 2^20 (arf exp 2^20+1)", C(10, 53, [P(1, 2**20, 0, 0, F(1, 3), 0)]), 10),
    ("mid exponent 2^20-1 (arf exp 2^20)", C(10, 53, [P(1, 2**20 - 1, 0, 0, F(1, 3), 0)]), 0),
    ("mid exponent -2^20-1 (arf exp -2^20)", C(10, 53, [P(1, -2**20 - 1, 0, 0, F(1, 3), 0)]), 0),
    ("mid exponent -2^20-2", C(10, 53, [P(1, -2**20 - 2, 0, 0, F(1, 3), 0)]), 10),
    ("rad exponent 2^20-1, limit LONG_MAX", C(LONG_MAX, 53, [P(0, 0, 1, 2**20 - 1, 0, 1)]), 10),
    ("rad exponent -2^20-1", C(10, 53, [P(1, -1, 1, -2**20 - 1, F(1, 3), 1)]), 0),
    ("centre 2^21-bit numerator", C(10, 53, [P(1, -1, 0, 0, F(2**(2**21 - 2) + 1, 3), 0)]), 'see'),
    ("centre 2^21+1-bit numerator", C(10, 53, [P(1, -1, 0, 0, F(2**(2**21) + 1, 3), 0)]), 10),
    ("centre 2000-bit, N 7/360, width 3", C(10**6, 53, [P(5, 0, 3, -1, F(2**2000 + 1, 360), F(7, 360))]), 0),
    ("local 360, d=7, width 5", C(10**6, 53, [P(5, 0, 5, -1, F(11, 7), F(360, 7), 1)]), 0),
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--harness', default=os.path.join(HERE, 'harness'))
    ap.add_argument('--as', dest='asl', type=int, default=4 * 10**9)
    args = ap.parse_args()
    for name, line, want in CASES:
        t0 = time.time()
        try:
            cmd = ['prlimit', '--as=%d' % args.asl, args.harness] if args.asl else [args.harness]
            env = dict(os.environ, ASAN_OPTIONS='detect_leaks=1')
            r = subprocess.run(cmd, input=line + "\n", env=env,
                               capture_output=True, text=True, timeout=70)
            dt = time.time() - t0
            rl = [l for l in r.stdout.splitlines() if l.startswith('R ') or l.startswith('X')]
            res = rl[0] if rl else ('exit %d: %s' % (r.returncode, r.stderr.strip()[-200:]))
            if r.returncode != 0 or r.stderr.strip():
                res += ' [exit %d, stderr %d bytes: %s]' % (r.returncode, len(r.stderr), r.stderr.strip()[:300])
        except subprocess.TimeoutExpired:
            dt = time.time() - t0
            res = 'TIMEOUT'
        print('%-45s want %-7s got %-40s %.2f s' % (name, want, res, dt))
        sys.stdout.flush()


if __name__ == '__main__':
    main()
