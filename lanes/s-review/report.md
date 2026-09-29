# Report, lane s-review

VALID 28 / MINOR 3 / INVALID 2. NOT READY.

Completed the adversarial review of all 33 statements, all 15 decisions, and all 13 proposed edits.
No design or specification file was changed. No git command or bd command was run.
No package was installed. Native threads were limited to 1; the target transport uses at most 2 processes.
The longest check reported 164.23 seconds. Initial free -g showed 21 GB available; a later check showed 22 GB.

## Important findings

- P3.9 is false: FLINT 3.0.1 returns 2, 3, 4 for X^2, X^3, X^4, with no exception.
- 3.11 is false: X^3 + 2X at 2 has only root 0, isolated before precision s + 1.
- The real reference raises OverflowError for X - 10^400; an exact integer-bound repair passed 4 cases.
- The seed API uses f for s but stores certificates for g; f = 27X at 3 exposes the conflict.
- A seed result lacks a specified unresolved partition, conflicting with the root-list invariant.
- The root-list verifier accepts false completeness flags because completeness is outside its stated checks.
- The real verifier accepts an interval with three roots when it checks only existence and n <= count.
- RR widening can break the API's final precision promise; final stored accuracy needs a check.
- The lowered-denominator shortcut cannot certify both 1/1 and -1/1 for m = 2, A = B = 1.
- Correction 5.1.2 needs Thue parameters 3, 3 for inclusive bounds 2, 2; 5.1.6 overstates FLINT behavior.
- The pending-row, kernel-counting, and p-adic partition/termination proofs close.
- 302,400 reconstruction calls and 1,875,272 modular systems gave 0 disagreements.
- 105,106 invented scalar certificates gave 237 acceptances and 0 false acceptances.
- 29,000 candidate root certificates gave 352 acceptances and 0 false acceptances.
- An incorrect limit-status variant changed 51,291 statuses and passed check_s3_limit.
- A real reference that ignores requested precision passed check_s2_real_completeness.

## Files written

- docs/reviews/s-design/review.md: complete review, statement table, decisions, edits, repairs, checks, limits.
- docs/reviews/s-design/review_checks.py: independent finite oracles and subprocess target transport.
- docs/reviews/s-design/checks/flint_probe.c: installed FLINT 3.0.1 probes with finite-span oracle.
- docs/reviews/s-design/checks/mutation_checks.py: 2 wrong variants that pass author checks.
- docs/reviews/s-design/checks/repair_checks.py: integer-bound repair and widening contract witness.
- docs/reviews/s-design/checks/audit_artifacts.py: syntax, width, count, and report-presence checks.
- lanes/s-review/report.md: this report.
- lanes/s-review/flint_probe: compiled probe, rebuilt after the corrections recorded below.
- lanes/s-review/author_checks_output.txt: all 25 author-check results.
- lanes/s-review/recon_output.txt, matrix_output.txt, extras_output.txt: independent arithmetic outputs.
- lanes/s-review/roots_output.txt, certificates_output.txt, real_output.txt: independent root outputs.
- lanes/s-review/flint_output.txt, mutation_output.txt, repair_output.txt, artifact_output.txt: other outputs.

## Commands and results

Python and executable checks used OPENBLAS_NUM_THREADS=1 and OMP_NUM_THREADS=1.
The raw logs contain the complete numerical outputs. The review reproduces the independent outputs.

- free -g: first availability 21 GB; later 22 GB. Both exceed the required 6 GB.
- timeout 170s python3 proto/solvers_checks.py: exit 0; 25 checks; 0 failures; reported 19.1 seconds.
- timeout 170s python3 docs/reviews/s-design/review_checks.py recon:
  exit 0; 60,480 boxes; 302,400 calls; 1,334 arbitrary valid certificate pairs; 0 failures; 4.38 seconds.
- timeout 170s python3 docs/reviews/s-design/review_checks.py matrix:
  exit 0; 17,330 matrices; 1,875,272 systems; 105,106 invented certificates; 0 failures; 164.23 seconds.
- timeout 170s python3 docs/reviews/s-design/review_checks.py roots:
  exit 0; 3,756 calls; 2,477 Newton comparisons; 0 failures; 2 reproduced findings; 0.92 seconds.
- timeout 170s python3 docs/reviews/s-design/review_checks.py real:
  exit 0; 59 planted polynomials; 106 exact intervals; 0 failures; 2 reproduced findings; 0.8 seconds.
- timeout 170s python3 docs/reviews/s-design/review_checks.py extras:
  exit 0; 32,946 Howell inputs; 250 ball systems; 791 finite-field polynomials; 0 failures; 6.66 seconds.
- timeout 170s python3 docs/reviews/s-design/review_checks.py certificates:
  first run exit 0; 29,000 candidates; 352 accepted; 0 false acceptances; 0.31 seconds.
  Extended run exit 0; also 48 depth-bound cases, 24,111 reduced pairs, 5,383 ball rationals,
  769 proper-inclusion witnesses; 0 failures; 0.39 seconds.
- cc -O2 -Wall -Wextra docs/reviews/s-design/checks/flint_probe.c
  -o lanes/s-review/flint_probe -lflint -lgmp -lmpfr:
  first compile had 1 implicit-declaration warning and 1 undefined symbol, fmpz_poly_divexact.
  The attempted executable run then exited 127. This was an error in the review probe.
  The replacement fmpz_poly_divides was read at refs/src/flint-3.0.1/fmpz_poly.rst:1923-1926.
  Two later compilations exited 0 without diagnostics; the second added HNF coverage.
- timeout 170s lanes/s-review/flint_probe:
  first successful version: 2 Howell engines, 2,560 matrices; 0 differences.
  Final version: 3 engines including HNF, 2,560 matrices; 712 large reconstructions; 60 real polynomials;
  0 differences; 3 reproduced outside-contract FLINT count findings; exit 0.
- timeout 130s python3 docs/reviews/s-design/checks/mutation_checks.py:
  exit 0; 2 surviving incorrect variants; 51,291 changed statuses; 2 precision violations; 0 reproduction failures.
- timeout 35s python3 docs/reviews/s-design/checks/repair_checks.py:
  exit 0; 4 huge linear polynomials pass the proposed repair; 1 widening counterexample; 0 failures.
- A Python line-width check initially found 2 lines above 116 characters in review.md; both were shortened.
- python3 docs/reviews/s-design/checks/audit_artifacts.py:
  exit 0; 7 files; 4 Python syntax checks; 0 overlong lines; 2 reports; 33 verdicts; 0 failures.
- test -s docs/reviews/s-design/review.md && test -s lanes/s-review/report.md:
  exit 0; both required files exist and contain text.

The unchanged author's check file has SHA-256
09588ed7a576dc85971458fe5a53069c09b917347f3ba9bd6696507b75fd6b36.

## Not done

No implementation or proposed edit was applied. No decision was taken on behalf of the owner.
No proof of the specialized multi-limb FLINT reconstruction internals or its internal root isolation was made.
No exhaustive test of every 2x2 matrix at N = 16 or 36, higher dimensions, all primes, or all precisions was made.
No future public-API ABI, memory, aliasing, concurrency, or maximum-size benchmark was run.
The root interface still needs the repairs listed in the review before literal implementation.

## Sources pending

- [source pending: a local statement/proof of the intermediate value theorem]
- [source pending: a local statement/proof of Sturm's theorem if internal justification is required]
- [source pending: the integer resultant Bezout identity if that optional sentence is retained]
- [source pending: FLINT 3.0.1 nmod polynomial documentation for the large-prime alternative]

The polynomial EEA source is on disk at refs/src/shoup-ntb/ntb-v2.txt:20510-20521; it is no longer pending.

## Findings against the specification

None established against existing SPEC 9.1 or 9.2. Findings concern the design and proposed edits.
