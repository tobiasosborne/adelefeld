# Lane s2-review: adversarial review of roots at a prime from a seed (S.2 slice 1)

Your task is to REFUTE, not to confirm. The code under review is `src/roots.c` with its header
`include/adelefeld/roots.h`, written by Claude Opus. Its contract: the header; `docs/api-s.md` section 4;
`docs/proofs/solvers.md` section 3 (Lemma 3.1, Definition and Proposition 3.2, 3.3, 3.12, 3.13(1), 3.11);
decisions S-D13, S-D14, S-D16, S-D17, S-D18 in `docs/SPEC.md` 15.3; `lanes/s2-slice1/result.md` (the
author's report, with its header findings).

**You own:** `lanes/s2-review/` only. Everything else is read-only. No git, no `bd`. At most 2 cores. Run
every program under `timeout`. Build the library with `make -j2` and link your own programs against
`build/libadelefeld.a` (see how `Makefile` builds a test).

## What counts as a finding

- `adf_root_padic_from_seed` returns `OK` with a ball `a + p^K Z_p` that does not contain exactly one root
  of the normalised polynomial `g` in `Z_p`, or whose root is not the one near the seed, or with a wrong
  `s`, `K`, `g` or `reduced`; or returns `NOT_DETERMINED` for a seed that satisfies the strong form for
  `g`; or `OK` for one that does not. Your own oracle: enumeration modulo a power of `p` and your own
  squarefree part, not `proto/solvers_checks.py` and not the author's tests.
- `adf_rootlist_verify_entries` accepts a list whose claim is false. Build lists by hand, field by field;
  every value that satisfies `adf_rootlist_is_canonical` is a fair input, and also values that do not: the
  verifier and the predicate must return 0 and never abort or read out of bounds on any struct whose
  pointers and lengths are consistent.
- A call aborts (the author names two `flint_abort`: find an input that reaches one), overflows a word
  (`prec_p`, `K`, `s`, `k + s`, `2 k - s`, the bit length of `p^(2K)`), allocates before a `LIMIT`, runs
  for a time the header does not admit, leaks, or writes `L` on a status other than `OK`.
- Aliasing: `f` or `a` a field of `L`; `L` reused between calls with another prime.
- A sentence of the header that the code does not keep; a test of `tests/test_roots_seed.c` that cannot
  fail; a statement of `docs/proofs/solvers.md` section 3 that is false (give the counterexample).

Places to attack first: `p = 2` with `s > 0`; polynomials with content divisible by `p`, with leading
coefficient divisible by `p`, with a repeated factor whose squarefree part changes `s`; constants and
degree 1; negative and huge seeds (thousands of bits, not reduced); roots that are 0 or a multiple of a
high power of `p`; `k0 >= K` (the path that does not lift); `prec_p` at the limit and one above; `p`
near `2^64`; the two survivors of the author's mutants (checked additions replaced by plain ones): is
the claim "cannot overflow" true for every input that passes the limit test?

## Report

`lanes/s2-review/report.md`, written once, at the end. For each finding: severity (BLOCKER: a wrong
result, a false list accepted, or a memory fault; MAJOR; MINOR), the input, what the code returns, what is
true and why, and the command that reproduces it with a program in your lane directory. Then: what you
attacked without result, with the number of cases and what would have made a case fail. No praise, no
summary of the code.
