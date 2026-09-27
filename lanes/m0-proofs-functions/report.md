# Work package 0.3a: functions proofs

Status: deliverables written; 20 numerical check groups pass. Four source obligations remain explicit.

## Work done

Read CLAUDE.md, SPEC 9.3.1 to 9.3.6, PLAN 0.3 and 1F.3 to 1F.8, both named reviews and their checks,
and docs/proofs/precision.md. No git command or tracker command was run.

Wrote 22 numbered statements with hypotheses, stepwise arguments, Check and Used by labels, and a final
statement index. The file covers every requested topic. It proves lifting, cyclicity, factorial valuations,
series identities and convergence locally, rather than leaving them as unquoted standard theorems.

The truncation formulas are literal integer formulas. They cover all six series, all primes, input valuation
bounds on the stated domains, and every integer target exponent. The numerical enumeration uses p=2,3,5,13,
four valuation bounds, all target exponents from -3 to 40, and omitted degrees through 4096. Infinite tails
are covered by the written proof, not by the finite enumeration. Working precision includes denominator loss.

The root argument proves equality of the guarded branch image with the specified ball. It includes negative
root valuations and failures outside the guard. The global obstruction is proved for every degree n>=2 by
an elementary construction of an unrestricted prime, without a theorem on primes in arithmetic progressions.
The R4 rational-root operation and the independent infinitely many adelic branches are distinguished.

For independent principal-unit base and exponent balls, the exact output exponent is
min(A+v_p(s_0), B+v_p(log(u_0)), A+B). The proof and checks include zero centres, one exact input,
and the separate sign/parity conditions at 2.

## Files written

- docs/proofs/functions.md
- proto/functions_checks.py
- lanes/m0-proofs-functions/report.md
- lanes/m0-proofs-functions/checks.txt: output of the complete numerical suite.
- lanes/m0-proofs-functions/timing.txt: timing and exit status of that suite.
- lanes/m0-proofs-functions/validate.py: repeatable structural and line-length audit.
- lanes/m0-proofs-functions/validation.txt: final structural audit output.

No path outside these owned files or the lane directory was written. SPEC.md was not changed.

## Checks run

Numerical and structural runs, in execution order:

1. `python3 -B proto/functions_checks.py`: exit 1, with the initial cutoff function deliberately unimplemented.
   The truncation test raised NotImplementedError before producing any passing check line. This was the red run.
2. `python3 -B proto/functions_checks.py`: exit 0 after implementing cutoffs and the first nine check groups.
   Nine groups printed their counts; these are the first nine groups in the results table below.
3. `python3 -B proto/functions_checks.py`: exit 0 after adding roots, powers, global and typed checks.
   Nineteen groups printed their counts; these are the first nineteen groups below.
4. `python3 -B -`: inline line-length scan of the proof, prototype and draft report, exit 0.
   Found 6 proof lines, 1 prototype line and 0 report lines longer than 116 characters. This scan only reported.
5. The complete timed numerical run used this command, on one Python process with no worker threads:

       /usr/bin/time -f 'elapsed_seconds=%e user_seconds=%U system_seconds=%S max_rss_kib=%M exit=%x' \
         -o lanes/m0-proofs-functions/timing.txt python3 -B proto/functions_checks.py \
         > lanes/m0-proofs-functions/checks.txt

   Exit 0; 20 check groups; elapsed 1.70 seconds; user 1.70 seconds; system 0.00 seconds;
   maximum resident memory 14,976 KiB. No numerical assertion failed.
6. `python3 -B -`: inline AST/regex link, statement-number and line-length audit, exit 1.
   It found 22 numbered statements, 22 Check labels, 22 Used by labels and 19 linked check functions.
   The only failing assertion was 1 remaining prototype line longer than 116 characters; proof and report had 0.
7. `python3 -B lanes/m0-proofs-functions/validate.py`: exit 0 after wrapping that line.
   Four files had 0 lines longer than 116 characters; 22 numbered statements and 22 index rows agreed.
8. `python3 -B lanes/m0-proofs-functions/validate.py > lanes/m0-proofs-functions/validation.txt`:
   final audit after completing this report; exit 0. Four files have 0 lines longer than 116 characters.
   There are 22 statements, 22 Check labels, 22 Used by labels, 19 linked functions and 22 index rows.

The inline audits in runs 4 and 6 were replaced by the saved validate.py in runs 7 and 8. The final suite
uses only the Python standard library. It does not call FLINT or compare one FLINT wrapper with itself.
No package was installed. The only changes after the timed run were prose and line wrapping.

| Check function | Result counts |
|---|---|
| check_legendre | 4,100 factorials through 1,024; 484 exact factorial comparisons |
| check_domains | 2,169 term cases; 24 zero values; 9 boundary-subsequence pairs |
| check_lifting_and_groups | 76 unique lifts; 32 finite power maps |
| check_decomposition | 6,462 decompositions at valuations -2,0,2; 2 sign examples |
| check_truncation | 4,224 scenarios; 11,500,509 omitted terms; maximum target 40 |
| check_working_precision | 876 rounded partial sums; targets -2,0,1,7,20,40 |
| check_series_identities | 182 comparisons at precision 8; includes 2 golden residues |
| check_series_radii | 240 distance pairs; 16 cosine hulls; exp(12)-exp(3) valuation 2 at 3 |
| check_log_radii | 884 distance pairs; 60 exact quotient images; 4 special cases |
| check_global | 300 rational witnesses; 900 excluded-ball witnesses; 16 Log coordinates |
| check_root_criteria | 13,436 unit classes; 7,133 solvable fibers; 120 scaled roots; 2 examples |
| check_root_precision | 96 branch images; 96 preimages; 288 scaled images; 2 guard failures |
| check_global_roots | 1,200 rational cases; 9 unrestricted-prime witnesses; 256 sign tuples |
| check_powers | 84 integer powers; 49 sign powers at 2; 7 Log/torsion obstructions |
| check_power_precision | 372 uncertain images; 231 one-exact-input images; 186 zero-product centres |
| check_fractional_parts | 1,500 unique sections; 1,500 digit oracles; 6,000 ball witnesses |
| check_real_and_character | 136 real integer root sets; 528 rational phases; 36 additive phases |
| check_no_order | 5 Q_5 root quotients; 11 sum-of-squares quotients |
| check_typed_and_projection | 120 valuation balls; 15 real jump balls; 3 projections |
| check_regression_mutations | 13 incorrect rules rejected by 13 explicit witnesses |

The series oracle uses a longer exact rational sum with its own proved conservative tail estimate. Cutoff
checks count denominator valuations independently. Root and power image tests enumerate actual powers in
small quotients, including sets of all outputs. Mutations include constant fake sine/cosine, missing factorial
or logarithmic denominator loss, missing guard precision, wrong Log precision, missing root guards and losses,
and discarded exponent/cross uncertainty. These are explicit counterexample tests, not a coverage score.

## Not done

No C implementation or live FLINT wrapper was changed or tested. This lane supplies proofs and a prototype
oracle for those later tasks. In particular the exp(3)/exp(12) regression is checked by exact series here,
not by claiming to exercise an unimplemented ball wrapper.

No external mathematical review has been performed in this lane. Finite checks do not prove infinite
statements or the standard real analytic results. Real and complex claims depending on Lemma 2 are marked
conditional in the statement index. The idele-class decomposition and choice of character used by the
quasi-character are treated as input data; their construction belongs to other work packages.

## Sources pending

The refs directory was absent in the source inventories during this lane. No unavailable source was cited
from memory. Four explicit pending markers remain in the proof:

1. Lemma 2: real intermediate value theorem; continuity of polynomials; real/complex exponential, logarithm,
   periodicity, circle values, and real convergence, with the exact statements used there.
2. Proposition 4: the Teichmueller name/convention. Its existence, uniqueness and the 2-adic distinction
   are proved here and do not depend on that attribution.
3. Proposition 11: the Iwasawa logarithm name and the normalisation Log(p)=0. Its formulas are proved here.
4. Proposition 20: an on-disk Tate source for attribution of SPEC 6's sign convention. The convention is
   adopted as a definition, and triviality on Q is proved here.

## Findings against the specification

None found against the current SPEC 9.3.1 to 9.3.6. The examples outside the root guard, the failure of
exp(Log(p)) to equal p, and the infinitely many branches of sqrt(1) are already handled by the current text.
The no-order assertion is proved for every Q_p, in addition to the requested explicit Q_5 witness.
