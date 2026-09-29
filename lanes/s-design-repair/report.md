# Report, lane s-design-repair (repairs of the design of milestone S after its review)

Written by the lane (Claude Sonnet 5.5, effort medium, 02:36 to 03:20, 542k tokens) as its final message;
saved to this file by the orchestrator, who added the last section. Base `62f15fe`. No C was written;
nothing under `docs/reviews/` was touched.

Changed: `docs/proofs/solvers.md`, `proto/solvers_checks.py`, `docs/api-s.md`, `docs/sources.md` (table 3
and its notes only), `lanes/s-design-repair/`.

## 1. Table of answers (finding, answer, where)

Answer codes: REPAIRED = as the review proposes. REPAIRED-OTHERWISE = with the reason given. UNCHANGED =
verdict VALID or SOUND, the check was run. CONDITIONAL-STATED = alternatives written neutrally for the owner.
DEFERRED. No finding is contested (section 5).

### 1.1 Statements

| Finding | Answer | Where |
|---|---|---|
| D2.1 associate of zero and source lines (MINOR) | REPAIRED (R1). The zero residue has representative 0. Lines diss2up.txt:481, 521, 565-567 read. | solvers D2.1; `check_s1_assoc` |
| P2.11 empty input, line 293 (MINOR) | REPAIRED (R2). Padding is for NONEMPTY matrices; empty ones return early (strong_echelon_form_mod.c:167-168, howell_form_mod.c:20-21). Probed on 28 empty matrices. | solvers P2.11(2); `probe_s1_flint_empty` |
| Remark 3.6 (MINOR) | REPAIRED-OTHERWISE (R3). It is now Proposition 3.6 with proof. It is stated for gcd(f,f')=1 and shown for the normalised g* (new L3.1(3)). The resultant is NOT asserted (source pending). Shoup 17.4 (ntb-v2.txt:20462) is cited. | solvers P3.6; `check_s2_bezout_depth` |
| P3.9(1) blanket outside-contract claims (INVALID) | REPAIRED (R4), plus more: the code strips X^i and adds i, so X^3-X^2 returns 3 although it has 2 distinct roots (num_real_roots.c:107-129, 140-149, 155-159). | solvers P3.9(1); `probe_s2_flint_real` |
| 3.11(5) "s+1 is minimal" (INVALID) | REPAIRED (R5), plus more: even X-1 (s=0) is isolated at precision 0; formula for planted roots. | solvers 3.11(5); `check_s2_isolation` |
| P1.7 limit test weaker (VALID) | Statement kept; claim 3 restated with ell=max(limit,0); the check rewritten. | solvers P1.7; `check_s3_limit` |
| P3.2(3) "abbreviated reference" | REPAIRED. Claim 2 is now stated for every k' and proved; claim 6 added (equality is not enough, X^2+3 at 2). | solvers P3.2 |
| P3.5 (W), depth, termination; P2.5/P2.6 audit | UNCHANGED. (W) already covers only classes whose g has a root mod p. | solvers |
| L3.1, P3.7 lacked a polynomial Bezout source | REPAIRED. (EEA-poly) added: Shoup Thm 17.4 ntb-v2.txt:20462-20470, algorithm :20510-20521. | solvers section 0, L3.1, P3.7 |
| P3.8 conditional on IVT and a correct count | UNCHANGED. Remark added: a stored count is not a count. | solvers P3.8 |
| P3.10 "reference and API need separate repairs" | REPAIRED. Step 6 and claim 4 (accuracy of the output balls). | solvers P3.10 |
| Other VALID statements (D1.1 ... P2.12, P3.3, P3.4) | UNCHANGED. | solvers |
| Table omits D1.1, D1.3, D2.1 | REPAIRED. The table now has 41 rows with a Review column, plus a "Review record". | solvers end |

### 1.2 Reference failure and interface contradictions (eight items)

| Finding | Answer | Where |
|---|---|---|
| 1. `real_roots_ref([-10^400,1],6)` OverflowError | REPAIRED (R6): integer bound, regression added. | `real_roots_ref`; `check_s2_real_planted` |
| 2. Seed: condition and s on f, certificates on g (27X) | REPAIRED (R7): everything refers to g*; new Proposition 3.12; f=0 refused. | solvers L3.1(3), P3.12; api seed row; `seed_root`; `check_s2_seed` |
| 3. Seed returns complete=0 with no partition | REPAIRED (R7): field `scope` PARTITION or SEED (S-D16). | api section 4 |
| 4. Entries verifier passes an empty list for X(X-1) | REPAIRED (R7): `adf_rootlist_verify_entries` and `_verify_complete`; Proposition 3.13; S-D17. | api section 4; `padic_verify_*`; `check_s2_lists` |
| 5. Real interval [0,4] accepted; supplied count trusted | REPAIRED (R7): the complete verifier recounts. `real_cert_ok` keeps its meaning (checker of P3.8 with a trusted count); new `real_verify_entries` and `real_verify_complete`. | solvers P3.8 remark, P3.13 |
| 6. Accuracy promise absent from P3.10; widening | REPAIRED (R8): arb_rel_accuracy_bits >= max(prec,2) measured on the output balls, else NOT_DETERMINED; S-D19. | solvers P3.10(4); `rr_finish`; `probe_s2_flint_real` |
| 7. NOT_UNIQUE by lowering B (m=2,c=1,A=B=1) | REPAIRED (R9): note deleted; two solutions or full enumeration certify; new Proposition 1.11. | solvers P1.11; api note 4, `adf_resid_verify_result`; `check_s3_verify` |
| 8. `count` meaning; prototypes returning int; `_get_poly` cost | REPAIRED: count defined on every status; `_get_particular` and `_get_dual` written as `int`; `_get_poly` costs a copy. | api sections 2, 3, 4 |

### 1.3 Sources (table 3)

| Finding | Answer |
|---|---|
| Corr. 1 Thue inequality; corr. 2 pair not reduced fraction | UNCHANGED. The row already gives strict bounds, r*=A+1, the pair (-2,2), and A=B=2 having no solution. Note 4 got the same wording. |
| Corr. 3 (fmpz_mod_mat), 4 (nmod Howell), 6 (generic count not always Sturm), 7 (left kernel), 8 (arb_calc not used) | REPAIRED in the rows and notes 5, 6, 9. |
| Corr. 5 (denominator above D) | REPAIRED: note 3 sharpened with the hypotheses of P1.9(2). |
| Review adds: note 2 (1/1 and -1/1); Shoup 4202 hypothesis | REPAIRED: note 2 and the row "what that row gives". |
| Other source findings | UNCHANGED. |

`check_quotes.py`: rows 57, quoted characters 9259, failures 0 (quotes untouched).

### 1.4 Decisions and edits

| Finding | Answer |
|---|---|
| S-D1, S-D2, S-D5, S-D9, S-D11, S-D15 (SOUND) | UNCHANGED, with the fair alternatives named. |
| S-D3 | REPAIRED (R9): four conditions with ell. The reading "limit 0 is 2AB>=m" is refuted (6210 problems). The LIMIT alternative is stated as consistent. |
| S-D4 | REPAIRED: count defined on every status. |
| S-D6 | CONDITIONAL-STATED: recommended exception plus two alternatives (OK-with-kind; separate report argument). |
| S-D7 | REPAIRED: order stated (dimensions and K1-K5 before the greedy reduction consumes a FLINT certificate; K6/K7 last). |
| S-D8, S-D10, S-D12, S-D13 | CONDITIONAL-STATED. |
| S-D14 | REPAIRED: cylinder meaning; certificate-only alternative. |
| Missing decisions | Added as S-D16 (seed scope), S-D17 (meaning of verification), S-D18 (resource limits, checked slong for e+j, k+s, 2k-s), S-D19 (final real accuracy and status). |
| E-S1, E-S2, E-S3, E-S5, E-P1, E-P2, E-C4 | REPAIRED with the texts of R10 and R9. |
| E-S4, E-C2, E-C3 (conditional); E-S6, E-C1 (accept) | UNCHANGED, conditions stated. E-C3 lists all four types. |
| E-C5 | DEFERRED until the invariants and prototypes are frozen. |

### 1.5 Checks ("Other limitations")

| Finding | Answer |
|---|---|
| `check_s3_limit` weak; mutant changing 51291 statuses | REPAIRED: exact four conditions, points recounted by Cramer's rule; mutant KILLED. |
| `check_s3_complete` did not exhaust A | REPAIRED: A in 0..2m for every m<=36 (366208 problems). |
| `check_s3_edge` counted attempts | REPAIRED: 3000 verified (1283 draws repeated). |
| `check_s2_descent` skipped silently | REPAIRED: 123 compared, 1 skipped, named in the output. |
| `check_s2_real_completeness` weak; mutant 2 | REPAIRED: mpmath required, gap and outside counts, accuracy by a second formula, more changed lists; mutant KILLED. |
| FLINT probes bounded | Probe with coefficients up to 400 digits added; "a probe is not a proof" stated. |
| Mutation must separate false results from other valid certificates | REPAIRED: `check_s1_checker_mutants` (each of K1..K7 removed accepts false certificates: [400, 681, 208, 1372, 87, 1454, 677]). |

## 2. Red and green logs (`lanes/s-design-repair/logs/`)

Red, on the file before the repair (`before/solvers_checks_before.py`):
- `red_checks_before.txt`: 9 of 9 RED probes FAIL.
  - real overflow at X-10^400;
  - [0,4] accepted;
  - 27X seed certificate does not refer to g;
  - lowering shortcut does not certify (2,1,1,1);
  - accuracy 9 bits below the requirement;
  - check_s3_edge prints 3000 but 2096 were called;
  - check_s3_complete lacks A-ranges for 34 of 36 moduli;
  - check_s2_descent skips silently;
  - D2.1 associate of zero, 39 residues.
- 3 REFUTED probes: the reading of S-D3 (4004 counterexamples); P3.9 (X^2, X^3, X^4 return 2, 3, 4); 3.11(5)
  (X^3+2X).
- `red_mutation_checks_before.txt` and `red_mutants_runner_before.txt`: both mutants of the reviewer SURVIVE
  (exit 0).
- The case m=2,c=1,A=1,B=3,limit=1 is the witness of mutant 1: ('NOT_DETERMINED', [], (2,0,1,1)).
- `baseline_author_checks.txt`: the old 25 checks, 28.7 s, 0 failures.

Green, on the repaired file:
- `green_checks_final.txt`: 0 of 9 RED probes fail.
- `green_mutants_runner_final.txt`: both mutants of the reviewer KILLED (exit 1). The reviewer's own script
  stops at the first nonzero exit, so `run_reviewer_mutants.py` runs both with its mutant texts unchanged.
  `green_mutation_checks_after.txt` shows the first one killed (51291 failures).
- `mutants_final.txt`: the lane's own 21 mutants of the new functions: 20 KILLED, 1 EQUIVALENT
  (`real_entry_weak_sign`, argued in the script). `mutants_run1.txt` is the first run: 3 survivors, then
  repaired.
- Red not computable for the finding on P2.11 (the FLINT call is right; the finding is prose).

## 3. Output of the final runs (file hash 1e957a8f...)

- `python3 proto/solvers_checks.py`: exit 0, `total time 52.2 s` (68.8 s in the last run, under concurrent
  load), 34 PASS, 0 FAIL (was 25). Output in `checks_output.txt`. The PASS lines of the two runs are
  identical.
- `run_reviewer_mutants.py`: `surviving mutants of the reviewer: 0 of 2`.
- `red_checks.py proto/solvers_checks.py`: `red probes failing: 0 of 9`.
- `mutants.py`: `mutants: 20, killed: 20, survived: 0`.
- `check_quotes.py`: rows 57, failures 0.
- `python3 -m pytest -q proto`: 35 passed, 13 subtests passed, 18.4 s.
- The reviewer's suites (`review_suites_final.txt`):
  - `recon`, `certificates`, `extras`: failures=0.
  - `roots`: failures=0, findings=2. These two FINDING lines (3.11-minimum, seed-normalization) are printed
    unconditionally by the suite; both are repaired in the design.
  - `real`: failures=1. The failing check is "overflow reproduction": it expects the OverflowError that R6
    removes, so that FINDING line (`reference-overflow`) is no longer true. The line `untrusted-real-count`
    still reproduces, because `real_cert_ok` keeps its name and meaning; the suite calls it by name. The
    repair is `real_verify_complete`.
  - `matrix`: its hard-coded `signal.alarm(165)` fires here even for the UNCHANGED file of 62f15fe (exit 142
    for both old and new; the review reports 164.23 s on its machine). A scratch copy with alarm(480) on the
    repaired file (hash 1e1c3996..., differing from the final only by one docstring word) gave
    `matrices=17330, systems=1875272, vectors=6036862, certificates=1875272, failures=0`,
    `invented_scalar_certificates tried=105106 accepted=237 false_accepted=0`, 215.5 s. This exceeded the
    guideline of about 3 minutes once (log: `review_suite_matrix_patched_alarm.txt`).

## 4. What is not done

- The repaired documents were not reviewed again. No C was written; no ABI, allocation or performance test
  exists.
- Not proved: the multi-limb paths of fmpq_reconstruct_fmpz_2; IVT and Sturm's theorem (sources pending); a
  resultant bound (not asserted); the larger-prime nmod_poly alternative (pending).
- S-D1 to S-D19 are recommendations. SPEC, PLAN and conventions are not edited (E-S1 to E-C4 are proposals;
  E-C5 is deferred).
- `proto/solvers_checks.py` is 129.9 KB, near the 131072-byte limit for one `python3 -c` argument that the
  reviewer's mutation script uses. About 1 KB more would break that script. The file's docstring says so.
- The reference gives a ball with midpoint 0 and positive radius no accuracy (a choice; arb.rst:499-509 does
  not spell it out). It returns the exact point 0 there.
- The numbers in the documents that come from random checks (bezout 211 and 42; isolation 574/133/440) are
  those of `checks_output.txt` and change if the file or the order of the checks changes.

## 5. Where the review is held to be wrong

None of its verdicts is contested. Each replacement text was read against its source and none was found
false. Differences of detail:
- (a) R8 says to test accuracy after widening. The reference has no engine, so it also refines each ball
  until the accuracy holds, and returns NOT_DETERMINED only if a test fails afterwards.
- (b) R3 is stated for gcd(f,f')=1 and shown for g*, not for "nonconstant squarefree f".
- (c) The review says the row on Thue "also needs this distinction". The row at 62f15fe already has it; it
  was left, as the brief says.
- (d) The alarm of the reviewer's suite `matrix` is a timing property of that suite, not a finding.

## Checks of the orchestrator before the merge (2026-09-29, 03:21)

- The lane changed the four files it owns and its lane directory (`git status`).
- `python3 proto/solvers_checks.py`: 34 PASS, `failures: 0`, 39.6 s.
- `python3 lanes/s-design-repair/run_reviewer_mutants.py`: `surviving mutants of the reviewer: 0 of 2`.
- `python3 lanes/s-sources/check_quotes.py`: 57 rows, 0 failures.
- NOT checked by the orchestrator: any repaired proof, any text of `docs/api-s.md`. The design is DRAFT 2.
  It was written by Claude Fable and repaired by Claude Sonnet: its judge is codex (closure check,
  `gpt-6-sol`), and the new statements (Propositions 1.11, 3.6, 3.12, 3.13, the new claims of P3.2, P3.10)
  have been seen by no reviewer yet.
