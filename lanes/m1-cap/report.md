# Lane m1-cap: report (Claude sonnet; saved by the orchestrator from the final message of the lane)

## Note on the brief

`lanes/m1-cap/brief.md` did not exist in the worktree of the lane: the harness made the worktree from
`948e1b5`, which is older than the commit of the briefs. The lane wrote a brief of its own from the task
message (`brief-as-used.md`) before any test or code. The brief of the orchestrator asked in addition for a
chain of 50 random capped operations and for the same expression evaluated tight and capped with containment
after every step; see "What is not done".

## What was done

The five functions at the end of `include/adelefeld/scaled.h`, global backend (policies.md Definition 13 to
Proposition 15, SPEC 4.4 item 3, conventions 5.4, api-m1.md Choices item 5):
`adf_fball_cap` applies Definition 13 to the set of `x`; `adf_fball_add_cap`, `sub_cap`, `mul_cap`,
`mul_rat_cap` compute the tight operation into a temporary and cap it.

Red-green, function by function, in `redgreen.log`: the first test gave a link error; after that each
function was tested against a stub returning `ADF_DOMAIN`, failed by assertion, and passed with the real
body. Two red runs found errors in the tests (operands of `fmpq_div` swapped in a divisibility check; a
sentinel ball that was not canonical).

## HEADER-FINDING

`scaled.h` does not say what a capped operation does with an input in the local backend. All five functions
return `ADF_UNSUPPORTED`, output untouched, when an input is not `ADF_GLOBAL`; when also `C <= 0`,
`ADF_UNSUPPORTED` wins (conventions 3.3, maximum).

## Files written

`src/cap.c` (172 lines); `tests/test_cap.c` (28 tests); `tests/test_cap_vectors.c` (5 tests);
`lanes/m1-cap/gen_vectors.py`; `tests/ref/vectors/m1-cap/cap_ops.jsonl` (2500 rows: 650 each of `add_cap`,
`sub_cap`, `mul_cap`, 550 of `mul_rat_cap`; seed 20260928); `lanes/m1-cap/redgreen.log`.

## Checks run

- `./build/test_cap`: 28 tests, 874 checks, 0 failed.
- `./build/test_cap_vectors`: 5 tests, 49930 checks, 0 failed; all 300 rows `absolute_cap` of
  `tests/ref/vectors/policies.jsonl` and the 2500 rows of the lane.
- `make -j2 check`: passed, 11 test programs. `make clean && make -j2 check SAN=1`: passed, 11 programs.
- `make mutate FILES=src/cap.c`: 29 mutants, 21 killed, 0 survived, 8 not compiled, 0 timed out, 0 excused;
  62.0 s.
- Orchestrator, in the worktree of the lane: `make clean && make -j2 check SAN=1`: passed, 11 programs.

GCC 13 with `-O2 -Werror` gave a false `-Wstringop-overread` error on `fmpq_gcd(g->q, N->q, C->q)` with three
`adf_rat_t`; worked around by copying through `fmpq_t` temporaries.

## What is not done

No benchmark and no fuzz target (none asked for). Not done because the lane did not have the brief of the
orchestrator: the invariant of Proposition 15 along a chain of 50 random capped operations; the same
expression evaluated tight and capped with containment after every step; enumeration of members for small
radii. Proposition 15.1 to 15.3 are tested on single operations.

## Sources pending

None.

## Findings against the specification

None.
