# m0-repair-analysis report

Date: 2026-09-27.

## What was done

- Applied R1 to R6 after checking their inequalities and algebra step by step. Lemma 6 now bounds the
  sum of absolute values and retains the former bound as a corollary. Lemma 8 defines primitive.
- Supplied the small-norm convergence estimate needed to unfold Proposition 12 and removed the two
  references to a nonexistent Lemma 15.
- Sharpened Proposition 15, stated which composite-conductor terms vanish, added the Im(s) accuracy
  remark, and made exact upper incomplete Gamma integration the main method.
- Applied R7. Before changes, all three named mutants survived. After changes, each is rejected.
- Added checks for the right Poisson tail, theta at a nonidentity idele, general Proposition 12 with
  f(0) != F f(0), the general adelic bound, and conductors 9, 16, 25, 27, and 15.
- Added the requested review record to the proof file.

## Files written

- `docs/proofs/analysis.md`
- `proto/analysis_checks.py`
- `lanes/m0-repair-analysis/mutation_probe.py`
- `lanes/m0-repair-analysis/report.md`

## Checks run

All commands were run from the repository root. The first mutation probe used this inline command:

```sh
python3 - <<'PY'
import sys
sys.path.insert(0, 'proto')
import analysis_checks as a
original_lattice = a.lattice_bound
a.lattice_bound = lambda *args: original_lattice(*args) / 2
try:
    a.check_tail_bounds()
    print('factor_2_dropped: SURVIVES', a.COUNTS['check_tail_bounds'])
except AssertionError as exc:
    print('factor_2_dropped: KILLED', exc)
a.lattice_bound = original_lattice
original_error = a.continuation_error
a.continuation_error = lambda *args, **kwargs: original_error(*args, **kwargs) * a.mp.mpf('1e-6')
try:
    a.check_continuation()
    print('error_times_1e-6: SURVIVES', a.COUNTS['check_continuation'])
except AssertionError as exc:
    print('error_times_1e-6: KILLED', exc)
a.continuation_error = lambda s,chi,C,N=24,R=420: original_error(s,chi,1,N,R)
try:
    a.check_continuation()
    print('conductor_1_bound: SURVIVES', a.COUNTS['check_continuation'])
except AssertionError as exc:
    print('conductor_1_bound: KILLED', exc)
PY
```

Before repair: 3 survived, with 80, 62, and 124 assertions completed; exit 0. The same inline
mutations after repair killed all 3 at assertions 81, 41, and 88; exit 0.

```sh
python3 proto/analysis_checks.py --only \
  check_tail_bounds check_continuation check_continuation_bounds check_local_gamma
```

Result: 379 assertions, 0 failed groups, 9.368 s.

```sh
python3 proto/analysis_checks.py --only \
  check_certified_continuation check_poisson_right_and_idele
```

Result: 46 assertions, 0 failed groups, 0.120 s.

```sh
python3 proto/analysis_checks.py --only \
  check_general_splitting check_general_adelic_bound check_composite_conductor
```

Result: 11 assertions, 0 failed groups, 39.040 s.

- The command `python3 proto/analysis_checks.py` before the output-label edit:
  3339 assertions, 0 failed groups, 63.893 s.
- The command `python3 lanes/m0-repair-analysis/mutation_probe.py`:
  3 mutants killed, 0 survived, 5.341 s.
- The command `python3 proto/analysis_checks.py` final:
  3339 assertions, 0 failed groups, 66.844 s; exit 0.
- The command `awk 'length($0)>116 {print FNR ":" length($0)}'` on all four written files:
  0 lines reported.
- The command `rg -n 'Lemma 15' docs/proofs/analysis.md`: 0 matches.

The inline mutation commands had exit code 0 because they recorded survival or rejection; the saved
probe has exit code 0 only if all three mutants are rejected. The final run used one core and took
less than three minutes. The new inequality checks using FLINT balls have 42 certified continuation
comparisons, 3 certified right Poisson-tail comparisons, and 4 certified general adelic comparisons.

## What is not done

- The repaired proof text has not had a second independent reader.
- Parameter-ball propagation in Proposition 15 is not tested; there is no implementation yet.
- The full read-only reviewer check file was not rerun. Its prior result is in the review, not this report.

## Sources pending

The proof marks these imports or external guarantees as source pending: Tate's thesis section 2.2;
Haar existence and uniqueness; product compactness; continuous characters of R; Fubini, dominated
convergence, and holomorphic parameter integration; the identity theorem; Fourier uniqueness;
Taylor's integral remainder; the Chinese remainder theorem; unique factorization in Z; a vertical-strip
Stirling bound; FLINT/Arb ball enclosure guarantees; and FLINT's upper incomplete Gamma enclosure.
No source under `refs/` was newly quoted in this lane.

## Findings against the specification

None found.
