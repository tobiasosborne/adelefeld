# Report: m0-review-ideles

Status: finished. Reviewed 51 theorem statements; all owned deliverables are written and validated.
Verdicts: **46 VALID / 3 MINOR / 2 INVALID**. Six definitions were checked separately.
Summary 26 needs a qualification and is outside the author's 51-statement count.

## Important items

- INVALID, policies P24: cancellation changes the canonical tuple, but the set still has its old representation.
- INVALID, policies P25: A = d = 2, q = 4 gives an integer centre despite the shared denominator/block factor.
- MINOR, policies L6: the necessity proof drops the divisor K; its representability criterion remains true.
- MINOR, policies P10: the unqualified word-gcd cost claim requires K to fit a word; all formulas remain valid.
- MINOR, ideles P19: use t and t+M from the image to justify minimality; the stated radius remains valid.
- Author finding 6 is overbroad: canonical sums and products can change H even when raw storage keeps it.
- A wrong mixed-modulus refinement formula survives the author's quotient check; an explicit witness is supplied.
- SPEC 4.1 exact-tag preservation constrains the cap choice; both enclosures are not equally compliant.
- Both pending sources are on disk: Milne ANT line 321 (factorization), Milne CFT line 9215 (Tychonoff).
- No counterexample to an enclosure radius was found; Python arithmetic does not establish C overflow safety.

## What was done

Read CLAUDE.md, the required SPEC sections and backend/exact-tag context, the relevant PLAN milestones,
all three proofs, all three author check scripts, the author's report and mutation file, and the cited local
source passages. Checked necessity, sufficiency, minimality, zero cases, signs, canonical moduli, shared factors,
closed endpoints, exact width thresholds and quotient gluing. No proof or specification file was edited.

Wrote a 51-row verdict table, details of all five non-VALID statements, verdicts on all seven author findings,
eight paste-ready repair sections, a source audit, an assessment of author test coverage, and independent checks.
The independent checker imports 0 author modules and uses 6 standard-library modules.

## Files written

Primary deliverables:

- `docs/reviews/m0-proofs/ideles-review.md`
- `docs/reviews/m0-proofs/ideles_review_checks.py`
- `lanes/m0-review-ideles/report.md`

Supporting scripts and preserved outputs in this lane:

- `author_mutation_probe.py`: executes mutated copies of author code in memory; originals remain read-only.
- `validate_review.py`: width, syntax, verdict-count, log and import checks.
- `author_policies.txt`, `author_ideles.txt`, `author_quotient.txt`: complete baseline rerun outputs.
- `author_mutants.txt`, `author_probe.txt`: author's 22 mutants and the 2 additional mutation probes.
- `initial_counterexamples.txt`, `policies_backend_run.txt`, `through_ideles_run.txt`, `full_checks.txt`:
  incremental checker runs, retained to record what was actually run.
- `final_checks.txt`: final independent checker output and process status.
- `validation.txt`: final deliverable validation output.

## Checks run

Each Python computation used one core. At most two computations ran concurrently. Subprocesses had a
175-second limit; none reached it. No system package was installed, and no git or bd command was run.

| Command | Result | Seconds |
|---|---|---:|
| `python3 -B proto/policies_checks.py` | 12 groups pass, exit 0 | 7.522 |
| `python3 -B proto/ideles_checks.py` | 11 groups pass, exit 0 | 46.876 |
| `python3 -B proto/quotient_checks.py` | 8 groups pass, exit 0 | 13.750 |
| `python3 -B lanes/m0-proofs-ideles/mutants.py` | 22 killed, 0 survived, exit 0 | 63.448 |
| `python3 -B lanes/m0-review-ideles/author_mutation_probe.py` | 1 killed, 1 survived, exit 0 | 6.863 |

Independent checker command at every stage:
`python3 -B docs/reviews/m0-proofs/ideles_review_checks.py`

| Stage | Assertions passed | Literal claims refuted | Checker exit | Seconds |
|---|---:|---:|---:|---:|
| Initial two counterexamples | 4 | 2 | 1 | not timed |
| Policies and backend | 39140 | 2 | 1 | 3.760 |
| Through ideles | 58417 | 2 | 1 | 6.221 |
| First full suite | 110677 | 2 | 1 | 6.579 |
| Final suite | 113317 | 2 | 1 | 3.208 |

Exit 1 is retained because policies P24 and P25 fail as written. No assertion checking the corrected formulas
or counterexample witnesses failed. The initial shell printed the redirected log after the checker, so that
shell returned 0; the checker itself returned 1. Later logs record the checker status explicitly.

The final suite has 35 count groups. It includes 6 explicit negative-control witnesses, 46 prime root checks
(all odd primes below 200 to p^4; 2 to 2^12), 9 divisor checks, 192 width-threshold checks, and 3480 assertions
on arbitrary mixed-modulus piece families. Full numeric group output is in the review and final_checks.txt.

Initial inline line-width audit: Python read both owned review files and counted lines longer than 116;
review 259 lines and checker 597 lines, 0 overlong lines in each. Final audit command:
`python3 -B lanes/m0-review-ideles/validate_review.py` (output in validation.txt).
Two runs: 5 files checked, 0 lines over 116, 3 Python files parsed, 0 syntax errors; exit 0 on both runs.
The audit confirms 51 verdicts in counts 46/3/2, the final 113317/2 checker log, and 0 author imports.

## What is not done

No changes were applied to proof files, SPEC, PLAN or implementation code. Repairs await the orchestrator.
No C tests, C overflow checks, benchmarks or character proofs were undertaken. The 51 statements exclude
additive-character and class-character proofs. Numerical checks do not prove topological claims or infinite
prime tails; those were reviewed from the written arguments and local sources.

## Sources pending

None for this review. The author's two pending markers can be closed using `refs/src/milne-ant/ANT.txt:321`
and `refs/src/milne-cft/CFT.txt:9215`; the review gives exact replacement citations. The Warwick topology
paragraph omits nonintegral translates when read literally; cite `refs/src/milne-cft/CFT.txt:9373` for the
restricted product topology. Citation replacements have not been applied to the author's files.

## Findings against the specification

1. SPEC 6 piece count: the stated construction has k+1 pieces for k strict interior integer crossings.
2. Exact unit case: finite cosets cannot represent a rational unit exactly; current conversion may enclose.
3. Exact cap: gcd(0,C) = C conflicts with preserving exact tags if applied indiscriminately; clarify the exception.
4. Scaled product: the reported loss h is correct and already allowed by SPEC; no specification error.
5. Powers: the tight coset hull is correct; enclosure and exponent-zero return semantics still need a choice.
6. Context changes: the author's narrowing is false for canonical outputs; SPEC 4.1 should keep its qualification.
7. Idele division: the proposed radius is correct after the minor proof repair; this fills an omitted rule.

The two INVALID proof clauses do not refute the original specification. The review supplies concrete sum and
product counterexamples to the author's finding 6, and distinguishes raw representation from canonical storage.
