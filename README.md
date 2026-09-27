# adelefeld

A programming surface for mathematics in which the adeles are an ordinary number type: C on FLINT, arbitrary
precision, ball arithmetic. **Status (2026-09-27): specification, plan and performance floors drafted; nothing is implemented.** Licence: AGPL-3.0.

- `docs/SPEC.md`: scope and specification (current draft: 2).
- `docs/proofs/precision.md`: proofs of the arithmetic rules.
- `docs/reviews/`: design reviews by a second model family.
- `docs/PLAN.md`: work packages and acceptance tests.
- `docs/PERF.md`: derived lower bounds and reference measurements of the FLINT primitives (`bench/baseline.c`).
- `proto/precision_rules.py`: brute-force check of the precision rules quoted in the specification
  (`python3 proto/precision_rules.py`).

Spun off from `riemann-channel` (see its `notes/adeles/`).
