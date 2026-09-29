# Report: s-closure

Original statements: CLOSED 32, CLOSED WITH EDIT 1, OPEN 0. New items: VALID 13, MINOR 1, INVALID 0.
Verdict: NOT READY. S-D13 needs an owner scope decision; text edits and a regression remain.

## What was done

- Read `CLAUDE.md`, SPEC 9.1-9.2, PLAN milestone S, the first review, the repair report, and the closure rules.
- Read the repaired proofs, reference algorithms, API, table 3, and cited source passages under `refs/src/`.
- Rechecked all 33 original statements, eight interface failures, table-3 corrections, decisions, and edits.
- Reviewed 14 new statements and decisions in refute mode. Wrote an independent edge and mutation probe.
- Found a false derivative-valuation formula in 3.11(5). The original minimum-precision error was repaired.
- Found conflicting meanings of `reduced` in the root-list API for f=2X.
- Found an unguarded seed temporary k0=5000 while 2K=2 in the S-D18 resource description.
- Found that a mutant skipping final real accuracy survives 60 FLINT calls; a targeted enclosure kills it.
- The real review suite fails only its old OverflowError expectation. The matrix suite exited 142 at its alarm.

## Files written

- `docs/reviews/s-design/closure.md`
- `docs/reviews/s-design/closure-checks/closure_probe.py`
- `lanes/s-closure/report.md`

No binary was written in these directories. No non-owned file was changed.

## Checks run

`OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1` was set for numerical Python commands.

| Command | Result |
|---|---|
| `timeout 170s python3 proto/solvers_checks.py` | Exit 0; 34 PASS, 0 FAIL; 36.8 s. |
| `timeout 170s python3 docs/reviews/s-design/closure-checks/closure_probe.py` | Exit 0; 2 killed, 1 survived. |
| `python3 lanes/s-design-repair/run_reviewer_mutants.py` | Exit 0; 2 of 2 killed. |
| `python3 lanes/s-design-repair/red_checks.py proto/solvers_checks.py` | Exit 0; 0 of 9 probes fail. |
| `python3 lanes/s-sources/check_quotes.py` | Exit 0; 57 rows, 9259 characters, 0 failures. |
| `timeout 170s python3 docs/reviews/s-design/review_checks.py recon` | Exit 0; 60480 boxes, 0 failures. |
| `timeout 170s python3 docs/reviews/s-design/review_checks.py certificates` | Exit 0; 29000 triples. |
| `timeout 170s python3 docs/reviews/s-design/review_checks.py extras` | Exit 0; 32946 matrices. |
| `timeout 170s python3 docs/reviews/s-design/review_checks.py roots` | Exit 0; 2 stale findings. |
| `timeout 170s python3 docs/reviews/s-design/review_checks.py real` | Exit 1; 1 stale overflow failure. |
| `timeout 170s python3 docs/reviews/s-design/review_checks.py matrix` | Exit 142; SIGALRM at 165 s. |
| `awk 'length($0)>116 {print FNR}' docs/reviews/s-design/closure.md lanes/s-closure/report.md` | 0 lines. |

The first `closure_probe.py` run exited 1 because its source path ended in `docs/proto/solvers_checks.py`.
The second exited 1 on an overbroad textual match in the mutant harness. Both were fixed in the owned script;
the final run exited 0 and tested six 400-digit real cases, five seed cases, 454 certificate truncations,
and three changed root lists. Both development failures occurred before a mutant ran.

## What was not done

- No C implementation or ABI, allocation, aliasing, or threading test was available.
- The matrix suite has a 165-second alarm; its 1,875,272-system total was not reproduced here.
- FLINT's multi-limb reconstruction and complex-root internals were not proved line by line.
- No change to SPEC, PLAN, conventions, or the repaired design was made; those paths are read-only here.
- E-C5's conventions subsections and prototypes remain deferred. E-S4 needs the owner scope decision.

## Sources pending

- [source pending: a local IVT proof for the real-completeness argument]
- [source pending: Sturm's theorem if an own Sturm count replaces the documented FLINT count]
- [source pending: an integral resultant Bezout identity if the optional bound is restored]
- [source pending: FLINT 3.0.1 nmod polynomial documentation for the larger-prime S-D10 alternative]

## Findings against the specification

None. The squarefree scope of S-D13 requires a decision against the present wording of SPEC 9.1.
