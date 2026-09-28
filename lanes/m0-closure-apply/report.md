# Report of lane m0-closure-apply, written by the orchestrator

Date: 2026-09-28. The lane (pi, mimo-v2.6-pro) ended after three attempts without writing its report (status
FAILED in `lane.log`). What it left was checked by the orchestrator and by the lane m1-headers.

## Applied by the lane
- `docs/conventions.md` 0.4 with E1, E2, C2, C3, C4, C5 (checked against `closure.md` by lane m1-headers).
- `docs/SPEC.md` and `docs/PLAN.md` 1.3 with E1, E2, C1 and the record of the gate and the closure check in
  PLAN section 8.
- `docs/proofs/analysis.md`: the scope note E4 and its line in the review record.
- C1 in the Python reference: the comparison of points returns the fixed integer values 0, 1, 2; the vectors
  `tests/ref/vectors/compare.jsonl` were regenerated.

## Damage, repaired by the orchestrator
The lane regenerated `tests/golden/dump.tsv`, `rfun.tsv`, `realball_print.tsv` and `realball_read.tsv` from the
out-of-date generator and so dropped the vectors of the gate findings G3, G4 and G7 (the hazard its brief
warned of). Two tests of the text grammar failed. The four files were restored from the last commit.

## Checks run by the orchestrator after the repair
| Command | Result |
|---|---|
| `python3 -m unittest proto/test_text_grammar.py` | 31 tests, OK |
| `python3 -m unittest discover -s tests/ref/tests` | OK |
| `python3 tests/ref/mutants.py` | 19 killed, 0 survived |
| `python3 proto/analysis_checks.py` | 3339 assertions, 0 failed groups |
| `docs/reviews/m0-gate/closure-checks/contracts.py`, `printing.py` | exit 0, exit 0 |

## Not done
- E3 (the surviving claim that the midpoint predicate proves a piece's construction) and C3, C4 in the status
  tables were not checked line by line by the orchestrator.
- `lanes/m0-conventions/write_golden.py` was changed by the lane but is not verified to reproduce
  `tests/golden/` byte for byte. Until it is, `tests/golden/` is edited by hand and the generator is not run.
