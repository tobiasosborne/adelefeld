# d-idlog running notes

2026-10-03. Read CLAUDE.md, workflow.md, COMMON.md, both f-slice10 briefs, and the required contracts.
Only docs/design/idele-log.md, proto/idlog_checks.py and this lane directory will be written.

Statement IL1 proved: Lemma 5 gives independent local factors. For p | N the factor is
r c + p^(v_p(r) + v_p(N)) Z_p. For p not dividing N it is p^(v_p(r)) Z_p^x.
At an odd unrestricted prime this is not an additive ball. At 2 it is an additive ball of exponent
v_2(r) + 1. The draft's assertion that every unrestricted factor is not a ball needs this exception.

Statement IL2 proved: Log kills p^m and is additive. For restricted odd primes its image is
Log(r' c) + p^k Z_p. The prime-free rational r' contributes its logarithm and cannot be dropped.

Statement IL3 proved: at 2, k >= 2 gives Log(r' c) + 2^k Z_2; k = 0 or 1 gives 4 Z_2.
k = 1 cannot occur in normal form, but occurs in valid stored constructor inputs (CV-17).

Statement IL4 proved: unrestricted units map onto p Z_p at odd primes and onto 4 Z_2 at 2.
Lemma 9 states and proves the inverse bijections. Translation by Log(r') changes neither group.

Statement IL5 proved: the finite image is the product of these factors, is contained in D, and hence
in 4 Zhat. A finite set of integral local refinements can be combined with 4 Zhat by integer CRT.
Exact local zero must be rounded to a requested exponent before this finite-ball construction.

Oracle run: timeout 120 python3 -B proto/idlog_checks.py --H 5
--fixtures lanes/d-idlog/fixtures.jsonl, exit 0. H = H' = 5. Components: 4280 cases, 4108800 points.
Images: 4280 set equalities, 4108800 enumerated units, 34240 cap checks.
Factor 2: 1644 equalities. Exact units: 160 cases, 40 exact zero results, 80 sign pairs.
Kernel: 17084 units, 3115 output classes, 16 independent square-route checks.
CRT: 120 cases, 91160 membership comparisons. Planted finite faults: 8, rejected: 8.
Fixtures written: 21880 JSONL rows. No implementation or real-coordinate test has run.

Expanded kernel checks: odd-prime torsion lifting, 2-adic square route, all isometry differences.
The expanded H=5 run exited 0: 17068 torsion checks, 16 square checks, 68320 isometry checks.
An expanded H=6 run concurrent with H=5 exited 124 at timeout 120 after completing components and images
(4280 cases each, 27307600 units, 34240 caps). It did not finish the remaining checks.
The earlier H=6 version without expanded kernel checks had completed all checks with exit 0.
Removed repeated construction of large coarse cap residue sets; inclusion and the exact exponent
still determine the same capped ball, using residue tests and the proved class cardinality.
The full-image comparison remains set equality, unchanged. A fresh final run will follow.

Statement IL6 proved: unit enumeration modulo p^H supplies exactly the image modulo p^H.
The precision is H'=H for the tested k<=4, H=5 or 6. Normalized input exponent H means absolute
input exponent m+H; Log kills the content's p-power. Isometry justifies the residue precision.

Statement IL7 proved: keeping degrees below 2H has a tail in p^H Z_p, since v_p(j)<=j/2.
Work at H+floor(log_p(2H-1)) allows exact p-power division in every retained term.
Odd-prime torsion lifting and the 2-adic log(a^2)/2 route give independent removal checks.

Statement IL8 established: expected_ball returns canonical local centre/exponent or proved exact zero.
expected_refinement rounds exact zero to N and intersects the baseline; fixtures carry integer data only.

Interface proposal and implementation plan written, including six declarations in Markdown only,
eight finite mutations, proposed driver commands and an unexecuted Julia example.
The second full expanded H=6 run also exited 124 after components and images. The final oracle now
allows --checks so the remaining groups can run as separate bounded commands, without reducing any grid.

Final H=5 oracle run exited 0, with the full original grid and the expanded independent checks.
4280 component cases; 4280 image equalities; 4108800 unit residues; 34240 capped results;
1644 factor-2 pairs; 160 exact cases, 40 exact zero cases; 17084 kernel units;
17068 torsion lifts, 16 square routes, 68320 isometry checks; 120 CRT cases, 91160 memberships;
8 planted finite faults rejected; 21880 fixtures regenerated.
Style/syntax check exited 0: design 658 lines, oracle 461 lines, notes 55 lines; 0 lines over 116;
1 Python syntax check, 0 errors. No C, driver or Julia test was run.

H=6 remaining groups completed with exit 0 under --checks factor_two exact kernel crt faults.
1644 factor-2 pairs; 160 exact cases, 40 exact zeros, 80 sign pairs; 113860 kernel units;
113828 torsion lifts, 32 square routes, 569268 isometry differences; 120 CRT cases,
91160 memberships; 8 finite mutants rejected at H=5.
The component enumeration now visits its congruence progression directly, rather than scanning
and rejecting all other residues. The enumerated set and comparison are identical. The cap modulus
is computed once per comparison. A final standalone H=6 grid run is in progress.
Artifact check exited 0: 21880 parsed fixture rows, 120 exact-zero fields, 4 primes,
5 content valuations, 0 canonical-field errors; 2 Python syntax checks, 0 errors; 0 long lines.

The standalone H=6 component/image grid completed with exit 0: 4280 set equalities in each group,
27307600 unit residues and 34240 cap checks. Together with the bounded tail command, every expanded
H=6 group has completed on the final rules. No full expanded H=6 run is claimed to have exited 0.
The design records the numerical table, the split commands and the two timeout exits.
