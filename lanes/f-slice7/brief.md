# Lane f-slice7: `sin`, `cos`, `sinh`, `cosh` at a prime (milestone 1F, WP 1F.7; adf-d98)

The four factorial series at a prime on local balls: `adf_lball_sin`, `adf_lball_cos`, `adf_lball_sinh`,
`adf_lball_cosh`, with the domain `p^c Z_p` (c = 1 for odd p, c = 2 at 2) of `docs/proofs/functions.md`
Proposition 6 and the radii of Proposition 10 (`exp`, `sin`, `sinh` preserve distances: the image of
`a + p^M Z_p` is the ball `f(a) + p^M Z_p`; `cos`, `cosh` are 1-Lipschitz: exponent `M` is safe, and on
the centred ball `p^M Z_p` the smallest enclosing ball is `1 + p^(2M - v_p(2)) Z_p`), as `docs/SPEC.md`
9.3.2 states them. Then the `_at` forms on partial balls (`adf_sball_sin_at`, `adf_sball_cos_at` exist in
`include/adelefeld/rfunc.h` and return `UNSUPPORTED` at a prime; `sinh_at`, `cosh_at` are new at both
places: at the real place through `arb_sinh`, `arb_cosh` in the pattern of `exp_at`), and the driver
commands `sin_at`, `cos_at`, `sinh_at`, `cosh_at` in the pattern of `exp_at` (`tools/adf/adf.c`).

This is intricate numerical work on proved statements: a result must contain `f(t)` for EVERY point `t`
of the input ball, with the exponent the propositions give, and a stated output precision `N` (absolute,
as in `lfunc.h`); identities alone do not count (`docs/PLAN.md` row 1F.7). The pattern is `src/lfunc.c`
`adf_lball_exp` (reviewed twice: `docs/reviews/f1/review-lfunc.md`, `review-lfunc-fast.md`): the
common-denominator Horner sum with the truncation count of Proposition 7 and the working precision of
Proposition 8, the precision rule F6 (`K = N` for an exact input, `K = min(N, E)` for a ball), the exact
values (`sin 0 = 0`, `cos 0 = 1`, `sinh 0 = 0`, `cosh 0 = 1` for the exact 0 only), the statuses and
limits of `lfunc.h`. Where Proposition 7's count is for `exp`, state and prove in `docs/api-1f4.md`
(new statements F10 onward, in the style of F1 to F9) what the count is for the four series (the
alternating or every-second-term structure; the tail bound from Lemma 5), the working precision, and
the exponent of a ball result for `cos`, `cosh`: decide whether the result for a ball keeps `M` (safe,
SPEC) or uses the smallest ball on a centred ball (tight, Proposition 10 step 4-5), state which, prove
it, and make the tests tell the two apart. Decisions where the specification is silent are yours (a
careful numerical analyst's), listed with the alternatives in the report; the orchestrator records them
in `docs/SPEC.md` 15.4.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold
for `lfunc.h` and `rfunc.h`: you add declarations; rule 5 is replaced by item 4 below), `docs/SPEC.md`
9.3.1, 9.3.2, 15.4 (the decisions N-D9, N-D10), `docs/proofs/functions.md` Definition 1, Lemma 5,
Propositions 6, 7, 8, Lemma 9, Proposition 10 (cite file and line in the code), `docs/PLAN.md` rows
1F.4, 1F.7, `include/adelefeld/lfunc.h`, `lball.h`, `rfunc.h`, `src/lfunc.c` (all), `src/rfunc.c` (the
`_at` functions at a prime of lane f-slice6), `docs/api-1f4.md`, `docs/api-1f.md` (the `_at` slice),
`tools/adf/adf.c` (`exp_at`), `tools/adf/README.md`, `tests/test_lfunc.c`, `tests/test_rfunc_prime.c`,
`proto/lfunc_checks.py` (the exact reference of lane f-slice4; extend it or write your own next to it),
`tests/julia/lfunc.jl`, `tests/julia/f_at.jl`. FLINT's `padic` module has no sin or cos
(`refs/src/flint-3.0.1/padic.rst`); FLINT stays out of the library (N-D9).

**You own:** `include/adelefeld/lfunc.h` (additions), `src/lfunc.c` (additions: new functions, shared
helpers factored out only if `exp`, `log`, `Log` give IDENTICAL results afterwards, which the stored
fixtures of `tests/test_lfunc.c` check), `include/adelefeld/rfunc.h` and `src/rfunc.c` (the four `_at`
functions), `tests/test_lfunc_trig.c` (new), `tests/test_rfunc_prime.c` (additions),
`tests/julia/lfunc_trig.jl` (new), `proto/lfunc_trig_checks.py` (new; exact arithmetic, the oracle of
this slice), `tests/ref/vectors/f-slice7/` (new, below 1 MB), `docs/api-1f4.md` (new statements at its
end), `tools/adf/adf.c`, `tools/adf/README.md`, `tests/driver/trig-*.cmd` and `.out` (new),
`lanes/f-slice7/`. In `tests/test_julia.sh` you may add the line your file needs. Everything else is
read-only; another lane is adding a FIXTURE to `tests/test_lfunc.c` and `tests/ref/vectors/f-slice5/` at
this moment, so do not touch those two paths. No git command that changes state, no `bd`. Build into
`BUILD=lanes/f-slice7/build` while you work and into `build/` only for the final checks.

**Battery.** The machine runs on battery today. Keep the compute of the lane small: at most 2 cores;
no benchmark; no run of more than 3 minutes; every program under `timeout`; no repeated clean rebuilds
(build once, then incrementally); oracles by enumeration only over small balls (`p` in 2, 3, 5, 7, 13;
exponents up to about 8), and random comparisons of a few thousand calls, not more. At the end ONE
`make clean && make -j2 check-all` and ONE `make -j2 check SAN=1` with `ASAN_OPTIONS=detect_leaks=0`
(the codex sandbox cannot run LeakSanitizer; the orchestrator runs the default sanitizer suite, clang,
INV and `check_headers.sh` on master). Do not run the clang or INV suites.

1. Header first: the comment block of every declaration (set statement, proposition with file and
   line, the exponent rule, aliasing, statuses `DOMAIN`, `NOT_DETERMINED`, `LIMIT` exactly as `exp` has
   them, the exact values).
2. Tests first, red then green (`lanes/f-slice7/redgreen.log`). Oracles, each with a STATED output
   precision:
   - enumeration: for `p` in 2, 3, 5, 7 and every ball of the domain with small exponent, the value
     of the series at EVERY point of the ball modulo a higher power (your reference, exact rationals,
     a proved tail bound) lies in the result ball, and the result has the exponent your statement
     promises (the smallest ball where you claim it);
   - the independent exact truncations of the PLAN: `cos 4 = 9 mod 16` at 2, `sin 3 = 3 mod 9` at 3,
     `v_3(cos 3 - 1) = 2`;
   - against `exp`: `sinh x = (exp x - exp(-x))/2`, `cosh x = (exp x + exp(-x))/2` as containment at
     every prime (mind `v_p(2)` at 2); `sin`, `cos` against `exp(i x)` at `p = 5` and `13`, where
     `i = sqrt(-1)` lies in `Z_p` (compute `i` modulo `p^H` by Hensel in your reference; say how `H` is
     chosen);
   - the precision rule: an exact input at `N`; a ball input with `N` above and below `E`; the exact 0;
     the loss or gain of `cos`, `cosh` on a centred ball; `p = 2` with `c = 2` (the ball `2 Z_2` is
     NOT_DETERMINED for every one of the four; the exact 2 is DOMAIN);
   - every status, every aliasing combination (`y = x`), the prime `2^64 - 59` at `N` up to 200, the
     limits of `lfunc.h` (`LIMIT`, output untouched).
   - `_at` forms: at a prime through the new functions (statuses passed on with `where` the prime);
     `sinh_at`, `cosh_at` at the real place against `arb` with the loss rule of `exp_at`; driver
     golden files in `tests/driver/` (the pattern of `tests/driver/f-*.cmd`).
3. The code. `tests/julia/lfunc_trig.jl`: a user computes `sin(5)`, `cos(5)` at `p = 5` to 20 digits,
   `cosh(4)` at 2, and sees `DOMAIN` for `sin(1)` at 5.
4. Show that the tests bite: five faults in a scratch copy under your build directory (among them: one
   term too few; the exponent of a ball result taken from the input precision without the rule; the
   exact value claimed for a non-exact 0; a sign error in an alternating term). No mutation run, no
   fuzz target.
5. Final checks as under "Battery"; give the last line of each.

Report: `lanes/f-slice7/report.md`, written ONCE, AT THE END; running notes in
`lanes/f-slice7/progress.md` (write them as you go: if your session is cut off, they are the report).
In it: functions built; decisions and their alternatives; what is proved (the new statements) and what
is not; the numbers of the tests and what would have made a case fail; the fault table; findings
against the specification; the next slice you propose (1F.5 local roots, or 1F.6 powers).
