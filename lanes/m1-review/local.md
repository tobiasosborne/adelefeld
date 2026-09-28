# Reviewer `local`: the local backend of finite balls, and the absolute cap

Read `lanes/m1-review/COMMON.md`; it binds you. Authors: Claude opus (local backend), Claude sonnet (cap).
You are of another model family.
Files under review: `src/fball_local.c`; in `src/fball.c` everything that concerns local values (life cycle
with the residue array, predicate L, accessors through the canonical triple, neg, add, sub, mul, mul_rat,
div_rat, predicates); `src/cap.c`; `tests/test_fball_local.c`, `tests/test_fball_local_vectors.c`,
`tests/ref/adfref/local_ref.py`; and the change of `tests/test_fball.c` that came with the lane (four
tests replaced: `git log -p --follow tests/test_fball.c`, merge `def075f`): was anything weakened?
Proofs: `docs/proofs/policies.md` sections 3 and 4 (you reviewed an earlier version of this file in
milestone 0 and refuted Propositions 24 and 25 as then stated; the restated versions are what the code
implements). `docs/conventions.md` 5.2, 5.3, 5.14.

Look in particular at: a denominator that shares a factor with a block (`A = d = 2`, block 4, and the same
with several blocks, with `d` a multiple of a block, with `d` a product of primes of different blocks);
cancellation that keeps the set but changes the canonical triple (`(2; 2)` in context `(4)`); the product
when `h > 1`: the code computes the tight global product and converts back with `adf_fball_set_local`,
claiming that this succeeds exactly when `h` divides `d e`: find inputs where the result is wrong, not
tight, or local when it must be global; sums whose canonical `H` changes; a local and a global operand;
two contexts with equal moduli and different block order; blocks near 2^64; 128 blocks; the residue array
under `set`, `swap`, `clear`, a change of backend, and an output that aliases an input of another context
(leaks and double frees: valgrind); `adf_fball_identical` and `equal_set` on two local values of the same
set with different raw data; the surviving mutant at `src/fball.c` with `i <= k` (one word read past the
arrays: is there a real out-of-bounds read in the code as it stands?); the cap: `gcd(R, C)` with rational
`R` and `C`, the invariant of Proposition 15 along chains, and the refusal of local inputs.
