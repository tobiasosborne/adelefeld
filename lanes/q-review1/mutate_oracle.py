#!/usr/bin/env python3
"""Mutate a scratch copy of the author's oracle and record which check group notices.

The copy lives in lanes/q-review1/mutants/. Nothing outside this lane directory is written.
Run: timeout 170 python3 lanes/q-review1/mutate_oracle.py
"""
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
SRC = HERE.parents[1] / 'proto' / 'quotient3_checks.py'
OUT = HERE / 'mutants'

MUTANTS = {
    'M1_sign_of_character':
        ("b, B = (x.fin.a-x.mid) % 1, x.fin.N.denominator",
         "b, B = (x.mid-x.fin.a) % 1, x.fin.N.denominator"),
    'M2_piece_count_off_by_one':
        ("stop = first + 1 if lo == hi else ceil(hi)",
         "stop = first + 1 if lo == hi else ceil(hi) - 1"),
    'M3_closed_upper_end_made_open':
        ("for n in range(ceil(p.lo-s), floor(p.hi-s)+1):",
         "for n in range(ceil(p.lo-s), max(ceil(p.lo-s), floor(p.hi-s))):"),
    'M4_rounding_inward':
        ("r = round_binary(max(m-p.lo, p.hi-m), 30, True)",
         "r = round_binary(max(m-p.lo, p.hi-m), 30, False)"),
    'M5_midpoint_invariant_dropped':
        ("                assert 0 <= (q.lo+q.hi)/2 <= 1\n", ""),
    'M5b_midpoint_left_of_zero':
        ("    m = round_binary((p.lo+p.hi)/2, max(prec, 2))",
         "    m = round_binary((p.lo+p.hi)/2, max(prec, 2)) + F(1, 4)"),
    'M6_finite_radius_ignored_in_phase':
        ("b, B = (x.fin.a-x.mid) % 1, x.fin.N.denominator",
         "b, B = (x.fin.a-x.mid) % 1, 1"),
    'M4b_rounding_inward_without_successor':
        ("    r = round_binary(max(m-p.lo, p.hi-m), 30, True)\n    if r:\n",
         "    r = round_binary(max(m-p.lo, p.hi-m), 30, False)\n    if False:\n"),
    'M8_refine_to_one_residue':
        ("                residues.update(range(m % p.N, L, p.N))",
         "                residues.add(m % p.N)"),
    'M9_contains_direction_reversed':
        ("        inside_xy &= R <= S and all(e % L in S or e in D for e in E)",
         "        inside_xy &= S <= R and all(d % L in R or d in E for d in D)"),
    'M10_equality_only_one_way':
        ("        inside_yx &= S <= R and all(d % L in R or d in E for d in D)",
         "        inside_yx = True"),
    'M11_overlap_ignores_exact_points':
        ("        overlap |= bool(R & S or E & D or any(e % L in S for e in E)",
         "        overlap |= bool(R & S or any(e % L in S for e in E)"),
    'M12_hull_ignores_radius':
        ("    return max(F(0), min(t, 1-t)/value.order-value.rad)",
         "    return max(F(0), min(t, 1-t)/value.order)"),
    'M13_zero_radius_becomes_modulus_one':
        ("                residues.update(range(m % p.N, L, p.N))\n            else:\n"
         "                points.add(m)",
         "                residues.update(range(m % (p.N or 1), L, p.N or 1))"),
    'M7_no_glue':
        ("        if s == 0 and p.hi == 1:\n            centers.append(p.m-1)\n", ""),
}


def run(path):
    p = subprocess.run(['timeout', '150', 'python3', str(path)], capture_output=True, text=True)
    groups = [ln.split()[1].rstrip(':') for ln in p.stdout.splitlines() if ln.startswith('PASS ')]
    return p.returncode, groups, p.stderr.strip().splitlines()[-1] if p.stderr else ''


def main():
    OUT.mkdir(exist_ok=True)
    base = SRC.read_text()
    # the copy sits three levels below the repository root
    fixed = base.replace("parents[1]", "parents[3]")
    results = {}
    rc, groups, err = run_of(fixed)
    results['unmutated'] = (rc, len(groups), err)
    print(f'unmutated: exit {rc}, {len(groups)} groups, last line: {err}')
    for name, (old, new) in MUTANTS.items():
        assert old in fixed, name
        text = fixed.replace(old, new, 1)
        path = OUT / f'{name}.py'
        path.write_text(text)
        rc, groups, err = run(path)
        ORDER = ['reduction', 'count_limit', 'sets', 'arithmetic', 'rounding', 'phases', 'hulls',
                 'local_images', 'ball_additivity', 'golden_phases', 'golden_qclass',
                 'full_and_width', 'gauss_boundary', 'examples_findings', 'fault_witnesses']
        failing = ORDER[len(groups)] if len(groups) < len(ORDER) else '(none)'
        results[name] = (rc, failing, err)
        print(f'{name}: exit {rc}; failing group {failing} (after {len(groups)} passed); {err}')
    print('summary')
    for k, v in results.items():
        print('   ', k, v)


def run_of(text):
    OUT.mkdir(exist_ok=True)
    path = OUT / '_unmutated.py'
    path.write_text(text)
    return run(path)


if __name__ == '__main__':
    sys.exit(main())