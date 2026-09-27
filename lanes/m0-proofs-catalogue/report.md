# Lane m0-proofs-catalogue report

## Done

Wrote 15 numbered statements and stepwise proofs for the requested Tier A catalogue rows. The proof covers
finite input precision, statuses, all 64 2-adic square-class pairs, the rational Hilbert product, and the
unrestricted finest modulus for profinite powers. It gives `N=5,c=2,e=0,M=2`: `D=1`, finest modulus 24.
The 2-adic table is generated independently by primitive-solution enumeration modulo 16.

## Files written

- `docs/proofs/catalogue.md`
- `proto/catalogue_checks.py`
- `lanes/m0-proofs-catalogue/report.md`

## Checks run

- `python3 -B proto/catalogue_checks.py`: final exit 0, 0.084 seconds. Counts: symbols 2,279; Hilbert
  solvability 96 class pairs, including 64 at 2; rational Hilbert product 690 pairs; zeta 30; power 2,050
  target-modulus and 738 unrestricted-modulus cases; binomial 980; content/volume 827; cyclotomic 1,604.
  The binomial case `(0,8,4)` gave radius 2. The 2-adic table is printed by this command.
- Earlier `python3 -B proto/catalogue_checks.py` runs: one exited 1 on an incorrect sufficient-condition
  assertion in the symbol check; one exited 1 on a missing argument to `vp` in the check; two later runs
  exited 0. Both check-code errors were fixed before the final run.
- `python3 -m py_compile proto/catalogue_checks.py`: exit 0. The generated catalogue bytecode file was removed.
- `awk 'length($0)>116 {print NR, length($0)}' docs/proofs/catalogue.md`: exit 0, zero lines reported.
- `rg -c '^Check:' docs/proofs/catalogue.md`: exit 0, count 15.
- `find refs -type f 2>/dev/null | wc -l`: exit 0, count 0.

## Not done

No production API was implemented. The checks cover finite samples and exact finite quotients; they are not a
general-purpose complex-ball zeta evaluator. External source verification remains for the pending items below.

## Sources pending

No source under `refs/` was available during this lane. The proof marks these exact obligations:

- Kronecker conventions at negative and zero denominators.
- Simple and strong Hensel lemmas in the stated forms.
- Quadratic reciprocity and the supplementary laws for -1 and 2.
- Gamma meromorphic continuation and its simple pole set.
- Local Haar-measure scaling.
- The logarithm isomorphism on odd-prime principal units and on `1+4 Z_2`.
- Exponents of `(Z/p^k)^x` for odd `p` and for `p=2`.
- Reciprocity sources fixing the arithmetic and geometric names for the two cyclotomic exponent conventions.

## Findings against the specification

None found in the requested passages. The existing qualification `D` is largest only among divisors of `N`
is necessary; the proof and check give a strictly finer unrestricted modulus 24 in the example above.
