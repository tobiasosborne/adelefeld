# Lane s2r-review: adversarial review of real roots (S.2 slice 3)

Your task is to REFUTE, not to confirm. The code under review is the part of `src/roots.c` that lane
s2-slice3 added (`adf_roots_real`, `adf_rootlist_get_arb`, the real place of `adf_rootlist_is_canonical`,
`adf_rootlist_verify_entries`, `adf_rootlist_verify_complete`, with their statics), written by Claude
Opus. Its contract: `include/adelefeld/roots.h`; `docs/api-s.md` section 4; `docs/proofs/solvers.md`
Lemma 3.1, Propositions 3.8, 3.9, 3.10 with Algorithm RR, 3.13; decisions S-D11, S-D13, S-D17, S-D19,
S-D20 in `docs/SPEC.md` 15.3; the author's report `lanes/s2-slice3/result.md` (with four header findings
and six findings about FLINT). Earlier reviews of the same file: `docs/reviews/s2/` (do not repeat them).

**You own:** `lanes/s2r-review/` only. Everything else is read-only. No git, no `bd`. At most 2 cores. Run
every program under `timeout`. Build the library with `make -j2` and link your own programs against
`build/libadelefeld.a`.

## What counts as a finding

- `adf_roots_real` returns `OK` with a list that misses a real root of `f`, lists a ball with no root or
  with two, or two balls of one root; or with a ball whose accuracy is below `max(prec, 2)` and that is
  not exact. Your own oracle: polynomials built from planted rational and algebraic roots whose positions
  you know exactly (products of linear factors with rational roots, of `X^2 - a`, of factors with no real
  root), and your own exact sign counts; not `proto/solvers_checks.py`, not FLINT's count.
- A sign or a comparison decided by a floating-point number or by a ball that is not exact. Read the code
  for it: every decision must be the sign of an integer or an exact comparison of dyadic numbers.
- `adf_rootlist_verify_entries` or `adf_rootlist_verify_complete` accepting a list built by hand whose
  claim is false (a ball with infinite or NaN midpoint or radius, a radius of `2^(10^9)`, balls out of
  order, touching balls, a ball whose end point is a root, `count` and `n` that agree with each other
  and not with `f`, `g` that is not the normalised polynomial of `f`); or reading `count` from the list;
  or aborting or allocating without bound on such a list.
- An abort inside FLINT reached through the public functions (the author reports that FLINT aborts on a
  polynomial that is not squarefree: can a caller reach that?), an allocation without bound, a running
  time the header does not admit (the author measured 28 s for two roots near `10^400`: find how the time
  grows and whether an input of modest size takes minutes), a leak, `L` written on a status other than
  `OK`.
- A sentence of the header that the code does not keep; a test of `tests/test_roots_real.c` that cannot
  fail; a statement of `docs/proofs/solvers.md` Propositions 3.8 to 3.10 that is false.

Places to attack first: roots at dyadic points and at 0; a double root of `f` next to a simple root at
distance `2^-200`; roots of size `2^(+-3000)`; degree 0, 1 and 60; content and leading coefficient huge;
`prec` 2 and at `ADF_ROOTS_REAL_PREC_MAX`; the widening step (does a widened ball still isolate? can it
swallow a neighbour?); a root that is an end point of the ball FLINT returns.

## Report

`lanes/s2r-review/report.md`, written once, at the end. For each finding: severity (BLOCKER: a wrong or
incomplete list called complete, a false list accepted, a memory fault, an abort inside the contract;
MAJOR; MINOR), the input, what the code returns, what is true and why, and the command that reproduces
it with a program in your lane directory. Then: what you attacked without result, with the number of
cases and what would have made a case fail. No praise, no summary of the code.
