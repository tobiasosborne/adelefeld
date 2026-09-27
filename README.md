# adelefeld

A programming surface for mathematics in which the adeles are an ordinary number type: C on FLINT, arbitrary
precision, ball arithmetic. Licence: AGPL-3.0.

**Status (2026-09-28).** Milestone 0 (contracts) has landed its work packages: build scaffold; sources on disk with
hashes; proofs of the arithmetic, idele, quotient, function, catalogue and analysis rules, each reviewed by a second
model family and repaired; conventions draft 0.2 with golden vectors; benchmark harness; seams sketch; the Python
reference of milestone 1. Specification, plan and performance floors are at version 1.1, which applies the findings
of milestone 0. **The gate review of milestone 0 is pending. Nothing of the C library is implemented**: the public
header is empty until the gate is passed. The library will be C; Python is used for tests, reference oracles and
proof checks only; a Julia layer on the C interface comes later.

- `docs/SPEC.md`: scope and specification.
- `docs/PLAN.md`: work packages, status of milestone 0, acceptance tests.
- `docs/PERF.md`: derived lower bounds, per hardware profile, and the measurements set against them.
- `docs/conventions.md`: canonical forms, storage invariants, status codes, text grammar, foreign-function rules
  (draft for the gate review).
- `docs/proofs/`: proofs of every rule, with their review records.
- `docs/reviews/`: design reviews and proof reviews by a second model family.
- `docs/sources.md`, `refs/`: the sources, fetched by `refs/fetch_sources.sh` and `refs/fetch_intel.sh`, with
  hashes.
- `docs/seams.md`: the interface tested on paper against a number field and `F_q(T)`.
- `proto/`: Python checks of the proofs; `tests/ref/`: the Python reference; `tests/golden/`: golden vectors.
- `bench/`: the benchmark harness and its dated results.
- `CLAUDE.md`: the rules of work.

Spun off from `riemann-channel` (see its `notes/adeles/`).
