# Lane s13-review: adversarial review of the second halves of `resid.h` and `linsolve.h`

Your task is to REFUTE, not to confirm. Two pieces of code, by two authors:

A. `src/resid.c`, the functions that lane s3-slice3 added (written by the free model `space-bunny-alpha`
   and not read by anyone since): `adf_resid_reconstruct_first`, `adf_resid_verify_result`,
   `adf_resid_set_rat`, `adf_resid_contains_rat`, `adf_resid_set_fball_forget`, the life cycle functions,
   and the static `resid_solve` into which the body of `adf_resid_reconstruct` was moved. Report:
   `lanes/s3-slice3/report.md` (its four "points worth the attention of the reviewer" are yours to judge).
B. `src/linsolve.c`, the functions that lane s1-slice2 added (Claude Sonnet): `adf_linsolve_fball`,
   `adf_linsol_verify_fball`, `adf_linsol_get_fball`, `adf_linsol_contains`, `adf_linsol_kernel_order`,
   `adf_linsol_get_image_cert`, `set`, `swap`, `identical`. Report: `lanes/s1-slice2/result.md`.

The contract: the two headers; `docs/api-s.md` sections 2 and 3 with their notes;
`docs/proofs/solvers.md` Propositions 1.7, 1.10, 1.11, 2.9, 2.10, Lemma 2.3; decisions in `docs/SPEC.md`
15.3. Earlier reviews: `docs/reviews/s3/review.md`, `docs/reviews/s1/review.md` (do not repeat them).
Spend about two thirds of the effort on A.

**You own:** `lanes/s13-review/` only. Everything else is read-only. No git, no `bd`. At most 2 cores. Run
every program under `timeout`. Build the library with `make -j2` and link your own programs against
`build/libadelefeld.a`.

## What counts as a finding

- A: `adf_resid_reconstruct` returning anything different from what it returned before the extraction
  (compare with the commit `29845cc` on a grid and on large operands, status, `q`, certificate);
  `reconstruct_first` with a wrong `q`, a wrong `count` (1 only if the set has one element, 2 only if at
  least two, 0 with `OK` only after a cut search), or a `q` that is not the first in the order of
  Algorithm R; `verify_result` accepting a false claim (every status, every `q`, every certificate, built
  by hand) or refusing a result of the library where the header says it decides; `set_rat`,
  `contains_rat`, `set_fball_forget` against Lemma 1.2 and Proposition 1.10 by your own enumeration, also
  for balls that are not canonical, local balls, `m = 1`, huge operands, negative denominators written by
  hand; a running time without bound (the enumeration inside `verify_result` when
  `floor(B/abs(T))` is large: is there an input where it runs for minutes?).
- B: a system of balls whose set of solutions in `Zhat^c` is not the inverse image of the coset returned
  (your own exact rational arithmetic); a coordinate ball that misses a coordinate of a solution or is
  larger than the set of coordinates modulo `N`; `contains` or `kernel_order` against enumeration;
  `verify_fball` accepting a false value.
- Both: aborts, overflows, leaks, allocation before a `DOMAIN`, `LIMIT` or `UNSUPPORTED`, an output written
  on a status for which the header says it is untouched, aliasing, a sentence of a header that the code
  does not keep, a test that cannot fail.

## Report

`lanes/s13-review/report.md`, written once, at the end. For each finding: severity (BLOCKER: a wrong
result, a false claim accepted, a memory fault inside the contract; MAJOR; MINOR), the input, what the
code returns, what is true and why, and the command that reproduces it with a program in your lane
directory. Then: what you attacked without result, with the number of cases and what would have made a
case fail. No praise, no summary of the code.
