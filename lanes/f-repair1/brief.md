# Lane f-repair1: repairs of `adf_lball` after the review f-review1

The review `docs/reviews/f1/review-lball.md` (codex gpt-6.1-sol) found no wrong enclosure and six
findings: F1 to F4 MAJOR, F5 and F6 MINOR. Its programs are in `lanes/f-review1/` (`boundaries.c`,
`invariant_probe.c`, `check_invariants.py`, `proof_probe.py`, `test_assertion.c`); build them as the
report says and see each finding reproduce BEFORE you change anything (that is your red run; log it in
`lanes/f-repair1/redgreen.log`).

Decisions of the orchestrator:
- F1 (inverse of an exact power `p^v` with `|v|` at the exponent limit returns `LIMIT`): repair. An exact
  value has no precision; the guard on `N - 2v` does not apply to it.
- F2 (`sub` returns `LIMIT` for small results because it first forms the canonical negated operand):
  repair. Compute the difference directly, as the sum is computed, with the sign inside; no canonical
  temporary of the negated operand.
- F3 (`div` returns `LIMIT` for small results because it first forms the whole inverse): repair. Compute
  the quotient directly: decide the exponent `K` of the result from the valuations and precisions first
  (statement for it with proof in `docs/api-1f.md`, next to L3 and L4), then compute the centre only to
  the relative precision the result needs. The exact 0 divided by a ball of units is the exact 0. After
  the repair the rule of the header is: `LIMIT` only if the RESULT, or an input, is outside the limits,
  never because of an intermediate value. Say this sentence in `lball.h` for every arithmetic function,
  and check `add`, `mul`, `neg`, `inv` against it too (look for the same defect there, with inputs at the
  limits of both signs; `neg` of `1 + 5^E Z_5` has a centre that needs `5^E`: there the RESULT is outside
  the bit limit, so `LIMIT` is right; say so).
- F4 (nine entry paths without the check of `ADF_CHECK_INVARIANTS`): repair as conventions 4.4 says.
  `set_rat` and `set_rat_ball` with an `adf_rat` that is not canonical: the entry check under the flag;
  without the flag the behaviour is as M1-D11 says (no promise). If conventions 4.4 exempts predicates
  or accessors, follow the conventions and say which paths are exempt and why.
- F5 (sign in the proof of L0, `docs/api-1f.md:71`): correct the equality.
- F6 (`tests/test_lball.c:976`, an assertion that cannot fail): replace it by one that fails for the
  wrong unit of the reviewer's example.

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold for `lball.h`; rule
5 is replaced by item 3 below), `docs/reviews/f1/review-lball.md`, `docs/api-1f.md`,
`include/adelefeld/lball.h`, `src/lball.c`, `docs/conventions.md` 4.4 and 5.8.

**You own:** `src/lball.c`, `include/adelefeld/lball.h`, `tests/test_lball.c`, `docs/api-1f.md` (ONLY
section 1, the section of slice 1: another lane adds a section at the end of the file at this moment),
`proto/functions_checks.py` (ONLY the section of lane f-slice1), `lanes/f-repair1/`. Everything else is
read-only.

1. Tests first: the seven cases of `boundaries.c`, the nine entry paths of F4 (the invariant tests are
   compiled only with the flag; see how `tests/` does it for other types), the example of F6, and for
   F2 and F3 the enumeration test of `tests/test_lball.c` extended to `sub` and `div` at mixed
   valuations if it does not cover them. Red, then green.
2. The repairs. No change of a result on inputs that gave `OK` before: the vectors of
   `tests/ref/vectors/f-slice1/` must pass unchanged.
3. Show that the new tests bite: for F1, F2, F3 the old code is the fault (the red run). No mutation run.
4. `make clean && make -j2 check-all`, `make clean && make -j2 check SAN=1`,
   `make clean && make -j2 check CC=clang`, `make clean && make -j2 check INV=1`,
   `sh lanes/m1-headers/check_headers.sh` pass, each under `timeout 900`; give the last line of each.
   Then the reviewer's programs again: `boundaries` exits 0, `check_invariants.py` exits 0.

Result: `lanes/f-repair1/result.md` and the same text as your final message: each finding with what was
changed and the test that shows it, what is not done.
