# Lane f-slice9: powers at a prime (milestone 1F, WP 1F.6): rational powers and principal-unit powers

SPEC 9.3.4 names four different operations called "power". Integer powers exist (`adf_lball_pow_si`, `lball.h`
line 338, statement L12 of `docs/api-1f.md`). The quasi-character comes with milestone 3. You build the other two,
on local balls at one prime, as thin working slices that a user can call (library, `_at` form, driver, Julia):

1. **Rational powers** `x^(e/n)`, `e` an integer, `n >= 1`, through the branches of `lroot.h`: Proposition 17
   (`docs/proofs/functions.md:577`): "integer powers and the selected n-th-root branches; reduce the fraction first
   and state the branch. A negative exponent excludes zero." The branch is selected by the seed of `lroot.h` (the
   identifier of the n-th root of `x`; N-D13, N-D14). Decide and prove the order (the root of `x` then its `e`-th
   power, or the other way: the branches differ when `gcd(e, n) > 1` is not reduced, which is why the fraction is
   reduced first; state the exponent of the result by composing R2 of `docs/api-1f5.md` with L12), the statuses
   (`DOMAIN` when no point has the root, `NOT_DETERMINED` outside the guard or for a ball containing 0 with `e < 0`
   or `n >= 2`, `NOT_UNIT` for the exact 0 with `e < 0`, `LIMIT` as the two components), the exact cases (an exact
   rational root raised to `e` is exact), and `K = min(N, E)` as `lfunc.h` and N-D14. `e/n` with `n = 1` must
   agree with `adf_lball_pow_si` exactly (test it); `e = 0` is the exact 1 as L12 says.
2. **Powers of principal units** `u^s = exp(s log u)`, `u` in `1 + p^c Z_p`, `s` in `Z_p`, both local balls at the
   same prime (the exponent is a `adf_lball` too: "the uncertainty of `s` and of `log u` both enter", SPEC 9.3.4
   item 3), Propositions 17 and 18 (`functions.md:577`, `:616`). Proposition 18 gives the EXACT image: for the base
   over `u_0 + p^A Z_p` (`A >= c`) and the exponent over `s_0 + p^B Z_p` (`B >= 0`), `ell = log u_0`,
   `alpha = v(ell)`, `beta = v(s_0)`, `R = min(A + beta, B + alpha, A + B)`, the set of powers is
   `exp(s_0 ell) + p^R Z_p`; an exact base gives `R = B + alpha`, an exact exponent `R = A + beta`, both exact give
   the exact-input case (a ball at the requested `N`, as `exp` of an exact input; or the exact 1 when an exact factor
   of the product is 0: `u = 1` or `s = 0`). Then `K = min(N, R)`. On all odd 2-adic units the integer-compatible
   extension is `w^(s mod 2) exp(s log u)` (Proposition 17; `w` the sign, the unit modulo 4): with `A >= 2` and
   `B >= 1` the sign and the parity are fixed (Proposition 18 step 5); at `A = 1` or `B = 0` the images of the two
   signs or parities must both be covered: decide whether the function returns the smallest ball containing the
   union (prove what it is) or `NOT_DETERMINED`, and say why; the odd primes have only principal units in the
   domain `1 + p Z_p` (a unit outside it, or a ball meeting the complement, is `DOMAIN` or `NOT_DETERMINED` as
   `lfunc.h` does for `exp`). The exponent `s` must lie in `Z_p`: a ball of `s` with negative valuation points is
   `DOMAIN` or `NOT_DETERMINED` likewise. Statuses, limits (`LIMIT` as `lfunc.h`, N-D7), aliasing (`y` may be
   either input), and the exact-zero conventions follow `lfunc.h` and `lroot.h`.
3. The `_at` forms at a prime on `adf_sball` in the pattern of `adf_sball_root_seed_at` (`rfunc.h`), the driver
   commands in the pattern of `root_at` (N-D10, N-D13: `powrat_at X with PRIME with E/N with SEED` and
   `powunit_at X with PRIME with S` or names you justify; a `with` separated integer pair for `e/n` or the value
   text of an exact rational; say which), `tools/adf/README.md`, a Julia example (`tests/julia/lpow.jl`: a user
   computes `9^(3/2)` at 5 on both branches, `(1 + 5)^s` for the exact `s = 1/2` (which lies in `Z_5`) and for a
   ball `s`, sees `DOMAIN` for `2^(1/2)` at 2 and `NOT_DETERMINED` where the guard fails).

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold for the
new header you write and for `rfunc.h`; rule 5 as written), `docs/SPEC.md` 9.3.1, 9.3.2, 9.3.3, 9.3.4, 15.4 (N-D7,
N-D9, N-D10, N-D13, N-D14), `docs/proofs/functions.md` Lemma 9, Propositions 11, 13, 15, 17, 18 (cite file and
line), `docs/PLAN.md` row 1F.6, `include/adelefeld/lball.h` (`pow_si`, L12), `lfunc.h`, `lroot.h`, `rfunc.h`,
`src/lfunc.c` (the pattern of a function of this family: statuses, limits, `K = min(N, E)`), `src/lroot.c` (as
repaired by lane f-repair4 today), `docs/api-1f4.md` (F1 to F15), `docs/api-1f5.md` (R1 to R8),
`docs/reviews/f1/review-lroot.md` (what a reviewer of this family attacks), `tests/test_lroot.c`,
`lanes/f-review6/oracle.py` (an oracle in exact integers, no logarithms: the pattern for yours),
`tools/adf/adf.c` (`root_at`, `exp_at`), `tools/adf/README.md`. `refs/src/` is on disk: FLINT is
`refs/src/flint-3.0.1/`.

**You own:** `include/adelefeld/lpow.h` (new) and `src/lpow.c` (new), `include/adelefeld/rfunc.h` and
`src/rfunc.c` (the `_at` forms only), `tests/test_lpow.c` (new), `tests/test_rfunc_prime.c` (additions),
`tests/julia/lpow.jl` (new), `proto/lpow_checks.py` (new, exact integers only; the oracle of this slice),
`tests/ref/vectors/f-slice9/` (new, below 1 MB), `docs/api-1f6.md` (new: the statements you add and their proofs,
in the style of `docs/api-1f5.md`), `tools/adf/adf.c`, `tools/adf/README.md`, `tests/driver/pow-*.cmd` and `.out`
(new), `lanes/f-slice9/`. In `include/adelefeld.h` and `tests/test_julia.sh` you may add the lines your files
need. `src/lfunc.c`, `src/lroot.c`, `src/lball*.c` and their headers are read-only: you call them. No git command
that changes state, no `bd`. At most 2 cores; every program under `timeout`; build into
`BUILD=lanes/f-slice9/build` while you work and into `build/` only for the final checks. Julia is on the PATH.

1. Header first: the comment block of every declaration (set statement, proposition with file and line, the
   exponent rule, the branch, aliasing, every status, the exact values, the limits).
2. Tests first, red then green (`lanes/f-slice9/redgreen.log`). Oracles, each with a STATED output precision, in
   exact integers (no logarithm, no exponential, no p-adic library):
   - rational powers: for `p` in 2, 3, 5, 7, small `e` and `n` (reduced and not), every ball of small exponent, the
     set `{t^(e/n)}` on the chosen branch computed by exhaustive n-th roots modulo `p^H` (as `lanes/f-review6/
     oracle.py`) followed by integer powers; agreement with `pow_si` for `n = 1`; the exact cases;
   - principal-unit powers: the set `{u^s}` over the base ball modulo `p^H` and over the EXPONENT ball by the
     integers `s_0 + p^B t`, `0 <= t < p^(H')` (integer exponents are dense in `Z_p`; for the set modulo `p^H` a
     finite `H'` suffices: prove which), compared with `exp(s_0 ell) + p^R Z_p` modulo `p^H`, with a witness at
     distance exactly `p^R` (the smallest ball, Proposition 18), for `p` in 2, 3, 5, 7, `A`, `B` small, `alpha`,
     `beta` from 1 to 3 and infinite (`u_0 = 1`, `s_0 = 0`); the 2-adic sign and parity cases at `A = 1`, `B = 0`;
   - identities with stated precision (compatibility with integer powers, `u^(s+t) = u^s u^t`, `(uv)^s = u^s v^s`);
   - every status, every aliasing combination (`y = x`, `y = s`), the prime `2^64 - 59` at `N` up to 200, the limits
     (`LIMIT`, outputs untouched), inputs of thousands of bits;
   - `_at` forms and driver golden files.
3. The code; the Julia example.
4. Five planted faults in a scratch copy under your build directory (among them: the fraction not reduced; the
   `A + B` term of `R` missing; the sign factor at 2 ignored for odd `s`; `beta` taken from the ball instead of the
   centre). Mutation testing of `src/lpow.c`, `lanes/COMMON-C.md` rule 5 (at most 60 mutants, `--seed 1`, 2 jobs,
   `--san`, `timeout 1300`).
5. Final checks: `timeout 900 make -j2 check-all` once (in `build/`); one `ASAN_OPTIONS=detect_leaks=1` sanitizer
   build of `test_lpow` and `test_rfunc_prime` in `BUILD=build/san` and their runs. Give the last line of each.

Decisions where the specification is silent are yours, listed with the alternatives in the report; the orchestrator
records them in `docs/SPEC.md` 15.4. Report: `lanes/f-slice9/result.md`, written ONCE, AT THE END (the harness
refuses the name `report.md` for a Claude subagent); running notes in `lanes/f-slice9/progress.md` as you go. In
it: functions built; decisions and alternatives; what is proved (the new statements) and what is not; the numbers
of the tests and what would have made a case fail; the fault table; mutation survivors one line each; findings
against the specification; the next slice you propose (1F.8 or 1F.9).
