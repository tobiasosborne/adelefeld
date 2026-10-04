"""Plant independent faults in owned scratch copies; test only test_localfactor."""
from pathlib import Path
import argparse
import json
import subprocess
import time

lane = Path(__file__).resolve().parent
root = lane.parent.parent
src = (root / 'src/localfactor.c').read_text()


def replace(old, new, count=1):
    assert src.count(old) >= count, old
    return src.replace(old, new, count)


faults = {
    'half-B': replace('mag_mul(B, sum, f);', 'mag_mul(B, sum, f); mag_mul_2exp_si(B, B, -1);'),
    'no-enlargement': replace('acb_add_error_mag(ym, B);', '/* fault: no R B enlargement */'),
    'lower-E': replace('mag_mul(E, e1, e2);', '''
    /* Compute the intended E from precise scalar operations, but take its LOWER bound. */
    {
        arb_t vr, vi, ve, vx;
        arb_init(vr); arb_init(vi); arb_init(ve); arb_init(vx);
        arf_set_mag(arb_midref(vr), arb_radref(acb_realref(s)));
        arf_set_mag(arb_midref(vi), arb_radref(acb_imagref(s)));
        arb_hypot(vr, vr, vi, w);
        arb_mul(ve, vr, a, w); arb_expm1(ve, ve, w);
        arb_set_arf(vx, arb_midref(acb_realref(s))); arb_abs(vx, vx);
        arb_mul(vx, vx, a, w); arb_neg(vx, vx); arb_exp(vx, vx, w);
        arb_mul(ve, ve, vx, w); arb_get_mag_lower(E, ve);
        arb_clear(vr); arb_clear(vi); arb_clear(ve); arb_clear(vx);
    }'''),
    'open-integer': replace('if (arf_cmp(h, lo) >= 0)', 'if (arf_cmp(h, lo) > 0)'),
    'fallback-65': replace('if (n > ADF_LOCAL_ZETA_SHIFT_MAX) st = ADF_LIMIT;',
                           'if (n > ADF_LOCAL_ZETA_SHIFT_MAX + 1) st = ADF_LIMIT;'),
    'inward-round': replace('acb_set_round(out, out, prec);', '''
    arf_set_round(arb_midref(acb_realref(out)), arb_midref(acb_realref(out)), prec, ARF_RND_DOWN);
    arf_set_round(arb_midref(acb_imagref(out)), arb_midref(acb_imagref(out)), prec, ARF_RND_DOWN);'''),
    'wrong-sign': replace('neg = arf_sgn(arb_midref(acb_realref(s))) < 0;',
                         'neg = arf_sgn(arb_midref(acb_realref(s))) > 0;'),
    'lowprec-log-no-radius': replace('arb_log_ui(a, p, w);',
                                    'arb_log_ui(a, p, prec); mag_zero(arb_radref(a));'),
    'max-radius': replace('mag_hypot(R, arb_radref(acb_realref(s)), arb_radref(acb_imagref(s)));',
                         'mag_max(R, arb_radref(acb_realref(s)), arb_radref(acb_imagref(s)));'),
    'no-logpi-B': replace('mag_add(sum, sum, lp);', '/* fault: omit log pi */'),
    'no-M0-B': replace('mag_mul(sum, sum, m0);', '/* fault: omit M0 */'),
    'drop-pi': replace('acb_mul(g, g, pref, w);', '/* fault: omit prefactor */'),
    'wrong-Gamma-scale': replace('acb_mul_2exp_si(z, s, -1);', 'acb_set(z, s);'),
}


def main(args):
    dest = lane / 'mutants'
    dest.mkdir(exist_ok=True)
    rows = []
    chosen = args.fault or list(faults)
    for name in chosen:
        start = time.monotonic()
        path = lane / f'build/mutant-{name}.c'
        path.write_text(faults[name])
        exe = lane / f'build/test-mutant-{name}'
        cmd = ['timeout', '30', 'cc', '-std=c11', '-O2', '-g', '-Iinclude', '-Isrc', '-Itests',
               str(path), 'tests/test_localfactor.c']
        cmd += [str(p) for p in sorted((lane / 'build/support').glob('*.o'))]
        cmd += [str(lane / 'build/libadelefeld.a'), '-lflint', '-lgmp', '-lm', '-o', str(exe)]
        built = subprocess.run(cmd, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if built.returncode:
            (dest / f'{name}.log').write_text(built.stdout)
            row = {'fault': name, 'build_exit': built.returncode}
        else:
            testcmd = ['timeout', '20', str(exe)]
            test = subprocess.run(testcmd, cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            (dest / f'{name}.log').write_text(test.stdout)
            first = next((s for s in test.stdout.splitlines() if s.startswith('FAIL ')), None)
            last = next((s for s in reversed(test.stdout.splitlines()) if 'failed checks' in s), None)
            row = {'fault': name, 'build_exit': 0, 'test_exit': test.returncode,
                   'first_failure': first, 'totals': last,
                   'seconds': round(time.monotonic() - start, 3)}
        rows.append(row)
        (dest / f'{name}.json').write_text(json.dumps(row, indent=2) + '\n')
        print(json.dumps(row), flush=True)
    return 0


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--fault', action='append', choices=list(faults))
    raise SystemExit(main(p.parse_args()))
