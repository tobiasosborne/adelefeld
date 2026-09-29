# Lane f-review2: adversarial review of partial balls and real functions, and closure of review f-review1

Your task is to REFUTE, not to confirm. Two parts.

Part A (new code, written by Claude Sonnet in lane f-slice2): `include/adelefeld/sball.h`, `rfunc.h`,
`src/sball.c`, `src/rfunc.c`, `tests/test_sball.c`, `tests/test_rfunc.c`, `tests/julia/sball.jl`, the
section "f-slice2" of `proto/functions_checks.py`, and the section of slice 2 of `docs/api-1f.md` with the
statements S1 to S7. Contract: the headers; `docs/conventions.md` 2 to 5.9, 7, 12; `docs/SPEC.md` 4 and 9
(9.3.1 for functions at a place); `docs/proofs/functions.md` Lemma 2, Proposition 14, Proposition 22.
Report of the author: `lanes/f-slice2/result.md`.

Part B (closure): lane f-repair1 repaired the findings F1 to F6 of `docs/reviews/f1/review-lball.md` in
`src/lball.c`, `include/adelefeld/lball.h`, `tests/test_lball.c`, `docs/api-1f.md` section 1 (new
statement L4a for the quotient). Its report: `lanes/f-repair1/result.md`. Judge each of F1 to F4 as
CLOSED or OPEN with a reproducer, and attack what the repair added: `sub` and `div` are now computed
directly (`lb_addsub`, the closed form of L4a): enclosure and smallest ball by your own enumeration, at
mixed and negative valuations, with exact operands, with balls containing 0 in the numerator, `sub(x, x)`
and `div(x, x)` on one object; the proof of L4a read as a referee; the rule "`LIMIT` only if an input or
the result is outside the limits".

**You own:** `lanes/f-review2/` only. Everything else is read-only. No git, no `bd`. At most 2 cores. Run
every program under `timeout`, none longer than 3 minutes. Build with
`make -j2 BUILD=lanes/f-review2/build` and link your programs against that archive.

## What counts as a finding in part A

- Real functions: a result with `OK` that does not contain the image of some point of the input ball
  (your own oracle: `mpmath` intervals at high precision, or exact rational bounds); a NaN or an
  infinite ball stored with `OK`; a wrong status at the edges of the domain (the end point 0 under `log`,
  `sqrt`, even roots; the exact 0; a ball across 0; odd roots of negative balls and of balls across 0);
  `sin` and `cos` on wide balls and on balls around multiples of `pi/2`; roots of degree 1, 2, 3, `2^20`,
  `2^63`; `prec` = 2, 53, 4096, `LONG_MAX` (a crash is a finding: the header states no bound on `prec`).
- Projection: a component at a prime that does not contain the image of some point of the adele's
  finite ball, or is not the smallest such ball; both backends of the finite ball; the real component;
  places in any order; repeated places; the empty set; 1000 primes; the prime `2^64 - 59`.
- Operations on partial balls: a tuple of points whose result is outside the result; `where` wrong or
  written on `OK`; output written on a status; aliasing.
- The cost that the author reports (finding 1 of his report: `adf_lball_set_fball` needs 13 s to return
  `LIMIT` for an adele with a radius of 87 million bits): find the worst case you can for a valid small
  answer, not only for a `LIMIT`.
- Predicates `equal_set`, `contains`, `overlaps`, `is_canonical` of partial balls against your oracle.
- A false statement among S1 to S7, a sentence of a header that the code does not keep, a test that
  cannot fail, a leak or undefined behaviour (sanitizer build; valgrind at `~/.local/bin/valgrind`).

## Report

`lanes/f-review2/report.md`, written once, at the end. Part B first: a table F1 to F6 with CLOSED or OPEN.
Then the findings: severity (BLOCKER: a wrong enclosure, a wrong predicate, a memory fault; MAJOR; MINOR),
the input, what the code returns, what is true and why, the command that reproduces it. Then what you
attacked without result, with counts and what would have made a case fail. No praise, no summary.
