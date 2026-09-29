# Lane f-review3: adversarial review of `exp`, `log`, `Log` at a prime (milestone 1F, WP 1F.4)

Your task is to REFUTE, not to confirm. The code under review was written by Claude Opus in lane
f-slice4: `include/adelefeld/lfunc.h`, `src/lfunc.c`, `tests/test_lfunc.c`, `tests/julia/lfunc.jl`,
`proto/lfunc_checks.py`, and `docs/api-1f4.md` with the statements F1 to F7 and their proofs. Contract:
the header; `docs/SPEC.md` 9.3.2 (the table of precision cases); `docs/proofs/functions.md` Definition 1,
Lemma 5, Propositions 6, 7, 7b, 8, Lemma 9, Propositions 10, 11. Report of the author:
`lanes/f-slice4/result.md`. The library evaluates the series itself; FLINT's `padic` is only a second
opinion in the tests.

The claim to break: for every input ball `x` in the domain and every requested absolute precision `N`
the result is a ball that contains `f(t)` for EVERY point `t` of `x`, of exponent `K = min(N, E)` with
`E` the exponent of the image (an exact input: `K = N`), and at `N >= E` it is the smallest ball.

**You own:** `lanes/f-review3/` only. Everything else is read-only. No git, no `bd`. At most 2 cores. Run
every program under `timeout`, none longer than 3 minutes. Build with
`make -j2 BUILD=lanes/f-review3/build` and link your programs against that archive.

## What counts as a finding

- A point `t` of an input ball with `f(t)` outside the result. Your own oracle, in exact rational
  arithmetic written by you, with your own proved tail bound for the series (do not import the author's
  reference): for `p` = 2, 3, 5, 7, 11 and the prime `2^64 - 59`; centres with negative valuation under
  `Log`; centres that are rationals with denominators; points `t` that differ from the centre in the
  LAST digit the ball admits; the edges of each domain (`exp`: `v(t) = 1` at odd `p`, `v(t) = 2` at 2,
  and one below; `log`: `1 + p Z_p`, `3 + 4 Z_2`, `-1` at 2); `p = 2` and `p = 3` first, where the
  factorial valuations are largest.
- A result that is too small: `E` wrong (the losses at `x = 3, 12` for `p = 3` and `x = 2, 10` for
  `p = 2`; `Log` of a ball whose centre has positive valuation; `r = 1` at `p = 2`), or a result
  called exact that is not.
- A truncation or working precision that is too small for some input: look for the worst case of
  `v_p(n!)` and `v_p(n)` in the counts of F4 and F5 (`n` a power of `p`, `N` just above a power of `p`),
  `N` from 1 to 300 exhaustively at `p` = 2, 3 for a few centres, and `N` = 2000, 10000.
- A false statement among F1 to F7, or a proof step that does not follow; F4 (one common denominator
  `L!`), F5 (the lower bound of a valuation taken from a residue) and the shortcuts that form no power
  first. Also Propositions 7b and 8 of `functions.md`, which have had no second reader: referee them.
- The route of `Log` at odd `p` through `log(a^(p-1))/(p-1)`: the precision lost or gained by the
  division, `p - 1` with many bits (`2^64 - 59`), `a^(p-1)` formed modulo which power.
- A wrong status: `DOMAIN` for a ball that meets the domain (it must be `NOT_DETERMINED`), `OK` for a
  ball that leaves it; `LIMIT` and the order of the checks; output written on a status other than `OK`;
  aliasing `y = x`.
- An abort, a division by 0 (the author's fault 5 aborted by a division by 0 in the count: can a valid
  input reach it), a leak, undefined behaviour, a running time above 10 s for `N <= 10000` at a prime
  of one word.
- A sentence of the header that the code does not keep; a test that cannot fail.

## Report

`lanes/f-review3/report.md`, written once, at the end. For each finding: severity (BLOCKER: a wrong
enclosure, a memory fault; MAJOR; MINOR), the input, what the code returns, what is true and why, and the
command that reproduces it with a program in your lane directory. Then what you attacked without
result, with counts and what would have made a case fail. No praise, no summary of the code.
