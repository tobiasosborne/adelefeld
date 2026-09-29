# Lane s2-review4: adversarial review of the roots modulo large primes (S.2 slice 4, issue adf-e0n)

Your task is to REFUTE, not to confirm. The code under review is what lane s2-slice4 (Claude Opus) added:
in `src/roots.c` the search of the roots modulo `p` above `ADF_ROOTS_P_EVAL_MAX = 128` (reduction of `g`,
`gcd(h, X^p - X)` with `X^p` by powering modulo `h`, the candidates of `nmod_poly_roots`, the test of every
candidate by evaluation, the comparison of the count with the degree of the gcd, the abort of decision
S-D20), the sentences of `include/adelefeld/roots.h` about it, `tests/test_roots_bigp.c`,
`tests/fuzz/diff_roots_padic.py`, `bench/bench_roots_modp.c`, the rows of `docs/sources.md`. Its contract:
`include/adelefeld/roots.h`; `docs/proofs/solvers.md` Proposition 3.7 and Algorithm P; decisions S-D10,
S-D18, S-D20 of `docs/SPEC.md` 15.3; the author's report `lanes/s2-slice4/result.md`. Earlier reviews:
`docs/reviews/s2/` (do not repeat their findings). The brief of the author: `lanes/s2-slice4/brief.md`.

**You own:** `lanes/s2-review4/` only. Everything else is read-only. No git, no `bd`. At most 2 cores. Run
every program under `timeout`, none longer than 3 minutes. Build with `make -j2` and link your own
programs against `build/libadelefeld.a`.

## What counts as a finding

- A list with `complete = 1` that misses a root of `f` in `Z_p` or lists a ball without a root, for a prime
  above 128, in particular of 33 to 64 bits. Your own oracle, not the author's: polynomials whose roots
  modulo `p` you know by construction (products of linear factors; `X^2 - a` with `a` a residue or a
  non-residue by Euler's criterion computed by you; `X^k - 1` with `gcd(k, p - 1)` roots; irreducible
  factors planted by you), lifted roots compared with your own Hensel lift.
- Arithmetic of one word: an overflow or a wrong reduction for `p` near `2^63` and `2^64` (`2^64 - 59`),
  coefficients that are negative, above `p`, or of thousands of bits; a leading coefficient divisible by
  `p`; degree of the reduction 0 or 1; degree above `p` is impossible here, degree above 1000 is not.
- The trust base. The author names `nmod_poly_powmod`, `nmod_poly_gcd` and `nmod_poly_roots` of FLINT.
  Say exactly which statements about these routines the completeness of the list rests on, which of them
  the code checks itself (every candidate by evaluation, the count against `deg gcd`), and which it
  trusts. A wrong gcd that is too small is the case to think about: is it caught? Read the C sources under
  `refs/src/flint-src-3.0.1/` and quote file and line.
- An input that reaches the abort of S-D20 without a defect of FLINT (then the abort is a defect of ours).
- A running time the header does not admit; an allocation or loop of `p` steps on the new route; a leak;
  `L` written on a status other than `OK`.
- The three findings that the author reports against `docs/proofs/solvers.md` (the source-pending line of
  S-D10; Algorithm P step 3 covers only the evaluation; the cost statement P3.5(7)): judge each, and write
  the exact replacement text for the proof file where it is right.
- A sentence of the header that the code does not keep; a test of `tests/test_roots_bigp.c` that cannot
  fail; a crossover 128 that the benchmark does not support (run `bench/bench_roots_modp.c` yourself).
- `tests/fuzz/diff_roots_padic.py`: does it assert the contract on the new route, and would it fail for a
  missing root? Show it with a fault planted in a scratch copy.

## Report

`lanes/s2-review4/report.md`, written once, at the end. For each finding: severity (BLOCKER: a wrong or
incomplete list called complete, a false list accepted, a memory fault; MAJOR; MINOR), the input, what the
code returns, what is true and why, and the command that reproduces it with a program in your lane
directory. Then: what you attacked without result, with the number of cases and what would have made a
case fail. No praise, no summary of the code.
