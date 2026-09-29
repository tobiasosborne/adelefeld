#!/usr/bin/env python3
"""Enumerate hand-built, canonical one-variable certificates modulo small rings."""
import itertools
import subprocess


def line(n, a, b, kind, ep, gp, v, z):
    g = [] if gp is None else [[gp]]
    e = [] if ep is None else [[ep]]
    vv = [] if ep is None else [[v]]
    x0 = [[z]] if kind == 0 else []
    y = [] if kind == 0 else [[z]]
    fields = [1, 1, n, kind, a, b]
    for rows, width in ((g, 1), (e, 1), (vv, 1),
                        (x0, 1 if kind == 0 else 0),
                        (y, 0 if kind == 0 else 1)):
        fields.extend((len(rows), width))
        fields.extend(x for row in rows for x in row)
    return ' '.join(map(str, fields)) + '\n'


def true_claim(n, a, b, kind, gp, z):
    kernel = {x for x in range(n) if a * x % n == 0}
    generated = {0} if gp is None else {q * gp % n for q in range(n)}
    solutions = {x for x in range(n) if a * x % n == b}
    if generated != kernel:
        return False
    if kind == 0:
        return {((z + x) % n) for x in generated} == solutions
    return not solutions and z * a % n == 0 and z * b % n != 0


def main():
    proc = subprocess.Popen(['lanes/s1-review/verify_probe'], stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, text=True, bufsize=1)
    total = accepted = rejected = false_accepted = 0
    for n in (4, 6, 8, 9):
        patterns = [(None, 1), (1, None)]
        patterns += [(d, n // d) for d in range(2, n) if n % d == 0]
        for a, b, kind, (ep, gp), v, z in itertools.product(
                range(n), range(n), (0, 1), patterns, range(n), range(n)):
            proc.stdin.write(line(n, a, b, kind, ep, gp, v, z))
            proc.stdin.flush()
            output = proc.stdout.readline()
            assert output
            canonical, verified = map(int, output.split())
            assert canonical == 1
            truth = true_claim(n, a, b, kind, gp, z)
            total += 1
            accepted += verified
            rejected += not verified
            if verified and not truth:
                false_accepted += 1
                print('FALSE_ACCEPTED', n, a, b, kind, ep, gp, v, z)
                break
        if false_accepted:
            break
    proc.stdin.close()
    assert proc.wait() == 0
    print(f'canonical={total} accepted={accepted} rejected={rejected} false_accepted={false_accepted}')
    assert false_accepted == 0


if __name__ == '__main__':
    main()
