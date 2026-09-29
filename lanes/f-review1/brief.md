# Lane f-review1: adversarial review of the local balls `adf_lball` (milestone 1F, slice 1)

Your task is to REFUTE, not to confirm. The code under review was written by Claude Sonnet in lane
f-slice1: `include/adelefeld/lball.h`, `src/lball.c`, `tests/test_lball.c`, `tests/julia/lball.jl`, the
section of lane f-slice1 at the end of `proto/functions_checks.py`, and `docs/api-1f.md` with the
statements L0 to L8 and their proofs. Its contract: the header; `docs/conventions.md` 2 to 5.8 and 12;
`docs/SPEC.md` 4 and 9; `docs/proofs/functions.md` Proposition 4. The author's report:
`lanes/f-slice1/result.md`. Every later slice of milestone 1F is built on this type, so a defect here is
expensive later.

**You own:** `lanes/f-review1/` only. Everything else is read-only. No git, no `bd`. At most 2 cores. Run
every program under `timeout`, none longer than 3 minutes. Build with `make -j2` and link your own
programs against `build/libadelefeld.a`.

## What counts as a finding

- A result of `add`, `sub`, `neg`, `mul`, `inv`, `div` that does not contain some result of points of
  the inputs (enclosure), or that is not the smallest ball (the header promises it). Your own oracle,
  in exact rational arithmetic, written by you: enumeration of points `c + p^N a` with several digits of
  `a`, including operands of different valuations, negative valuations, exact operands mixed with balls,
  balls that contain 0, `p = 2`, and the prime `2^64 - 59`.
- A false statement among L0 to L8 of `docs/api-1f.md`, or a proof step that does not follow. Read them
  as a referee. L3 (product, exponent `min(v(c) + M, v(d) + N, N + M)`) and L4 (inverse) first.
- A wrong answer of `equal_set`, `contains`, `overlaps`, `is_canonical`, `identical`, `valuation`, `abs`,
  `decompose`, `set_fball` (projection from both backends), `set_rat_ball`, `get_center`.
- `is_canonical` decides `u < p^(N - v)` by bit lengths: find a value it judges wrongly.
- The limits `ADF_LBALL_EXP_MAX = 2^60` and `ADF_LBALL_BITS_MAX = 2^26`: an overflow of an exponent
  sum, an allocation that grows with the request before `LIMIT` is decided, a result of an operation on
  admitted inputs that is outside the limits and is stored anyway, a valid small result refused.
- An abort, a leak, undefined behaviour, an output written on a status other than `OK`, an aliasing
  combination that gives another result than the call without aliasing.
- A sentence of the header that the code does not keep; a status that contradicts `conventions.md` 3;
  a test of `tests/test_lball.c` that cannot fail.
- Build with `-DADF_CHECK_INVARIANTS` (`make INV=1`): an entry check that refuses a valid value or lets
  a non-canonical one through.

## Report

`lanes/f-review1/report.md`, written once, at the end. For each finding: severity (BLOCKER: a wrong
enclosure, a wrong predicate, a memory fault; MAJOR; MINOR), the input, what the code returns, what is
true and why, and the command that reproduces it with a program in your lane directory. Then: what you
attacked without result, with the number of cases and what would have made a case fail. Then your
judgment of the author's decisions 2 to 5 (statuses, limits, two primes, `is_canonical`), each in two
sentences. No praise, no summary of the code.
