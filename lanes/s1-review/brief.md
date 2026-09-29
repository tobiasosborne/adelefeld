# Lane s1-review: adversarial review of linear systems modulo `N` (S.1 slice 1)

Your task is to REFUTE, not to confirm. The code under review is `src/linsolve.c` (788 lines) with its
header `include/adelefeld/linsolve.h`, written by Claude Opus. Its contract: the header; `docs/api-s.md`
section 3; `docs/proofs/solvers.md` section 2 (Definition 2.1 to Proposition 2.8, 2.11); decisions S-D6
to S-D9 in `docs/SPEC.md` 15.3.

**You own:** `lanes/s1-review/` only. Everything else is read-only. No git, no `bd`. At most 2 cores.
Build the library with `make -j2` and link your own programs against `build/libadelefeld.a` (see how
`Makefile` builds a test).

## What counts as a finding

An input `(A, b, N)`, a value `sol`, or a sequence of calls for which
- `adf_linsolve_mod` returns a status, a particular solution, a kernel or a vector `y` that the
  enumeration of `(Z/N)^c` contradicts (your own oracle, written by you; not
  `proto/solvers_checks.py` and not the tests of the author), or a `G` that is not the Howell form of the
  kernel (canonical: the same for every system with the same solution set);
- `adf_linsol_verify` accepts a value whose claim is false: a coset that is not the solution set, a
  kernel that is not complete, a `y` that does not prove emptiness. Build the values by hand, field by
  field; every value that satisfies `adf_linsol_is_canonical` is a fair input, and the header says the
  verifier never aborts on such a value;
- a function aborts (the solver calls `flint_abort` if its own checker refuses its result: find an input
  that reaches it), reads or writes out of bounds, leaks, divides by zero, or allocates before a `LIMIT`
  or `DOMAIN` is decided;
- an output is written on a status for which the header says it is untouched; aliasing (`sol` reused
  between calls, `A` and `b` the same matrix, `N` a member of `sol`).
Also a finding: a sentence of the header that the code does not keep; a test of
`tests/test_linsolve.c` that cannot fail; a statement of `docs/proofs/solvers.md` section 2 that is false
(give the counterexample). The author reports that the pending pair of the third case of Algorithm H is
implied by the others (`lanes/s1-slice1/result.md`, findings): check that claim, it is either a
simplification or an error.

Places to attack first: `N = 1`; `N` a prime power, `N` with many prime factors, `N` a square; zero rows
and zero columns in every combination; `r > c` and `r < c`; entries that are multiples of `N`, negative,
or thousands of bits long; a matrix whose entries are all zero divisors; systems whose kernel is not a free
module; pivots that are zero divisors; the greedy reduction of `b` when a pivot does not divide the entry;
the unit normalisation of a pivot; the order of the conditions (K1) to (K7) when an earlier one fails
(does a later one read outside a matrix?); a certificate with the right shapes and entries in `[0, N)` but
zero rows of `G` where a generator is needed.

## Report

`lanes/s1-review/report.md`, written once, at the end. For each finding: severity (BLOCKER: a wrong
result, a false certificate accepted, or a memory fault inside the contract; MAJOR; MINOR), the input,
what the code returns, what is true and why, and the command that reproduces it with a program in your
lane directory. Then: what you attacked without result, with the number of cases and what would have made
a case fail. No praise, no summary of the code.
