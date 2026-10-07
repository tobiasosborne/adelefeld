#!/usr/bin/env python3
"""Lane q-review3, hunt 5: the harness built with -fsanitize=address,undefined and ADF_CHECK_INVARIANTS against
build-san (SAN=1 INV=1), ASAN_OPTIONS=detect_leaks=1, on generated cases (lifts, PIECES, re-reduction).
The output must equal that of the plain harness, and the sanitizer must report nothing.
Usage: python3 sanrun.py --cases N --seed S"""
import argparse, os, random, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from check import gen_lift, gen_pieces, case_line, twice_input, R_count, fin_set  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--cases', type=int, default=1000)
    ap.add_argument('--seed', type=int, default=1)
    args = ap.parse_args()
    rng = random.Random(args.seed)
    lines = []
    for i in range(args.cases):
        prec = rng.choice([2, 20, 53, 128])
        r = rng.random()
        if r < 0.7:
            form, pcs = 0, [gen_lift(rng)]
        elif r < 0.9:
            form, pcs = gen_pieces(rng)
        else:
            form, pcs = 1, twice_input(os.path.join(HERE, 'harness'), rng, prec)
            if pcs is None:
                continue
        count = sum(R_count(p[0] - p[1], p[0] + p[1], *fin_set(p[2], p[3])) for p in pcs)
        lim = count if rng.random() < 0.7 else count - 1
        digits = rng.choice([0, 0, 2, 20])
        lines.append(case_line(form, lim, prec, digits, 1, pcs))
    data = "\n".join(lines) + "\n"
    t0 = time.time()
    plain = subprocess.run([os.path.join(HERE, 'harness')], input=data, capture_output=True, text=True,
                           timeout=170)
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:abort_on_error=0',
               UBSAN_OPTIONS='print_stacktrace=1:halt_on_error=1')
    san = subprocess.run([os.path.join(HERE, 'harness-san')], input=data, capture_output=True, text=True,
                         timeout=170, env=env)
    strip = lambda s: [" ".join(l.split()[:-1]) if l.startswith('R ') else l for l in s.splitlines()]
    same = strip(plain.stdout) == strip(san.stdout)
    print('seed %d: %d cases; sanitized exit %d; stderr %d bytes; outputs equal (timing column ignored): %s;'
          ' %.1f s' % (args.seed, len(lines), san.returncode, len(san.stderr), same, time.time() - t0))
    if san.stderr:
        print(san.stderr[:3000])
    return 0 if (san.returncode == 0 and not san.stderr and same) else 1


if __name__ == '__main__':
    sys.exit(main())
