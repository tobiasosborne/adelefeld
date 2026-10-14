#!/usr/bin/env python3
"""Search driver of lane t5-repair1: timeout 900 python3 -B lanes/t5-repair1/probe.py [NAME ...]

Builds lanes/t5-repair1/probe_lf.c against the plain archive lanes/t5-repair1/build/libadelefeld.a (the
original) and against each mutant of src/localfactor.c listed below (the mutant object linked before the
archive), runs each, and prints per mutant: the boxes with sample points outside the result (a candidate for a
non-enclosure), and the boxes whose result differs from the original's (wider, narrower, other status, more
midpoint bits). The scratch directory lanes/t5-repair1/probe/ is removed afterwards.
"""
import os
import shutil
import subprocess
import sys

B = 'lanes/t5-repair1/build'
FLAGS = ['-std=c11', '-O2', '-g', '-Wall', '-Wextra', '-Wpedantic', '-Werror']
D = 'lanes/t5-repair1/probe'

# the survivors of lanes t5-slice1 (seeds 51001, 51002) and t5-slice3 (seed 53001) in real_derivative_bound and
# real_factor: (line, old line text, new line text)
M = {
    '217': (217, '    mag_inv(inva, f);                                /* >= 1/A */\n', '\n'),
    '221': (221, '    mag_fac_ui(f, (ulong) (cb - 1));\n', '    mag_fac_ui(f, (ulong) (cb - 0));\n'),
    '222d': (222, '    mag_add(m0, inva, f);\n', '\n'),
    '223': (223, '    mag_mul(m1, inva, inva);\n', '    mag_add(m1, inva, inva);\n'),
    '225d': (225, '    mag_add(m1, m1, f);\n', '\n'),
    '237': (237, '        mag_sqrt_lower(d, d);\n', '\n'),
    '238': (238, '        mag_mul_lower(P, P, d);\n', '\n'),
    '244': (244, '    mag_set(oldsum, sum);\n', '\n'),
    '247': (247, '    mag_mul(oldsum, oldsum, m0);\n', '    mag_add(oldsum, oldsum, m0);\n'),
    '248': (248, '    mag_add(oldsum, oldsum, m1);\n', '    mag_sub(oldsum, oldsum, m1);\n'),
    '250': (250, '    mag_mul_2exp_si(oldsum, oldsum, -1);\n', '    mag_mul_2exp_si(oldsum, oldsum, -0);\n'),
    '252d': (252, '    mag_mul(sum, sum, m0);\n', '\n'),
    '253d': (253, '    mag_add(sum, sum, m1);\n', '\n'),
    '255': (255, '    mag_mul_2exp_si(sum, sum, -1);\n', '    mag_mul_2exp_si(sum, sum, -0);\n'),
    '344': (344, '                mag_mul(oldB, oldB, R);\n', '                mag_add(oldB, oldB, R);\n'),
    '367': (367, '        acb_set_round(previous, previous, prec);\n', '\n'),
}


def sh(cmd, timeout=600):
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=timeout, text=True)
    assert p.returncode == 0, (cmd, p.stderr[-2000:])
    return p.stdout


def build(name, obj):
    exe = os.path.join(D, 'probe_' + name)
    sh(['cc', '-Iinclude', '-Itests'] + FLAGS + ['lanes/t5-repair1/probe_lf.c'] + ([obj] if obj else []) +
       [B + '/libadelefeld.a', '-lflint', '-lgmp', '-lm', '-o', exe])
    return exe


def parse(out):
    rows = {}
    for line in out.splitlines():
        head, tail = line.split(' | ')
        f = head.split()
        rows[int(f[0])] = (f[1:], tail)
    return rows


def main():
    os.makedirs(D, exist_ok=True)
    try:
        base = parse(sh([build('orig', None)], 900))
        print('original:', len(base), 'calls,', sum(1 for v in base.values() if v[0][0] == '0'), 'OK,',
              sum(1 for v in base.values() if v[0][0] == '0' and v[0][1] != '0'), 'with a missed sample')
        with open('src/localfactor.c') as f:
            lines = f.readlines()
        for name in sys.argv[1:] or list(M):
            line, old, new = M[name]
            assert lines[line - 1] == old, (name, lines[line - 1])
            mod = list(lines)
            mod[line - 1] = new
            src = os.path.join(D, 'localfactor_%s.c' % name)
            with open(src, 'w') as f:
                f.writelines(mod)
            obj = os.path.join(D, 'm_%s.o' % name)
            sh(['cc', '-Iinclude', '-Isrc'] + FLAGS + ['-c', src, '-o', obj])
            rows = parse(sh([build(name, obj)], 900))
            miss, wider, narrower, status, bits, over, wl = [], 0, 0, 0, 0, 0, []
            for i, (f, tail) in rows.items():
                g, _ = base[i]
                if f[0] != g[0]:
                    status += 1
                    continue
                if f[0] != '0':
                    continue
                if f[1] != '0':
                    miss.append((i, f[1], tail))
                rf, rg = max(float(f[2]), float(f[3])), max(float(g[2]), float(g[3]))
                wider += rf > rg
                if rf > rg:
                    wl.append((i, '%.3g' % (rf / rg), tail))
                prec = int(tail.split()[-1][1:])
                over += int(f[4]) > prec or int(f[5]) > prec
                narrower += rf < rg
                bits += (f[4], f[5]) != (g[4], g[5])
            print('mutant %s: %d missed boxes, %d wider, %d narrower, %d other status, %d other bits, '
                  '%d above prec' % (name, len(miss), wider, narrower, status, bits, over))
            for m in miss[:6]:
                print('    miss', m)
            for m in sorted(wl, key=lambda u: -float(u[1]))[:8]:
                print('    wider', m)
    finally:
        shutil.rmtree(D)


if __name__ == '__main__':
    main()
