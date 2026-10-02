# Lane f-review5: adversarial review of the series sin, cos, sinh, cosh at a prime (lane f-slice7)

Your task is to REFUTE, not to confirm. Lane f-slice7 (`lanes/f-slice7/report.md`, codex gpt-6-astra,
merged `c09663a`) built `adf_lball_sin`, `adf_lball_cos`, `adf_lball_sinh`, `adf_lball_cosh` in
`src/lfunc.c` (declared in `include/adelefeld/lfunc.h`), the `_at` forms `adf_sball_sin_at`, `cos_at`,
`sinh_at`, `cosh_at` at a prime and at the real place (`include/adelefeld/rfunc.h`, `src/rfunc.c`),
the driver commands (`tools/adf/adf.c`), and the statements F10 to F14 at the end of `docs/api-1f4.md`
with their proofs. Its oracle is `proto/lfunc_trig_checks.py` (exact rationals, fixtures under
`tests/ref/vectors/f-slice7/`); its tests are `tests/test_lfunc_trig.c`, additions to
`tests/test_rfunc_prime.c`, `tests/julia/lfunc_trig.jl`, `tests/driver/trig-*.cmd`. Decision N-D12
(`docs/SPEC.md` 15.4): `cos`, `cosh` of a centred ball `p^M Z_p` return the exponent `E = 2M - v_p(2)`
(the smallest hull of Proposition 10), every other ball `E = M`; `K = min(N, E)`; `sin`, `sinh`: `E = M`.
Contract: `docs/SPEC.md` 9.3.1, 9.3.2; `docs/proofs/functions.md` Definition 1, Lemma 5, Propositions 6, 7,
8, Lemma 9, Proposition 10; `docs/conventions.md` 3.1, 4.1, 4.3; the header comment blocks.

**You own:** `lanes/f-review5/` only. Everything else is read-only. No git command that changes state,
no `bd`. At most 2 cores. Build the archive ONCE with `make -j2 BUILD=lanes/f-review5/build` and link
your programs against it; do not run the test suites of the repository (the orchestrator has run them).

**Battery.** The machine runs on battery: no program over 60 s, every program under `timeout`, a few
thousand inputs per attack and small balls for enumeration (not millions); compile only your
reproducers with `-fsanitize=address,undefined` if you need it (`ASAN_OPTIONS=detect_leaks=0`: the codex
sandbox cannot run LeakSanitizer).

## What counts as a finding

- A wrong enclosure: a point `t` of an admitted input ball with `f(t)` outside the result ball (your OWN
  oracle, not the lane's: exact rational partial sums with a tail bound you prove, or `exp(i x)` at `p`
  with `i` in `Z_p` lifted by Hensel, or `exp(x)` for the hyperbolic ones; state its precision), for
  `p` in 2, 3, 5, 7, 13, 65537, `2^64 - 59`; exact inputs and balls; centred and not; `N` below, at and
  above `E`; large `N` (2000) at small `p`; inputs of thousands of bits.
- A wrong exponent: a result claimed with exponent `E` where the image is not inside `f(a) + p^E Z_p`
  (BLOCKER), or where N-D12 promises the smallest hull (centred `cos`, `cosh`) and the result is not
  the smallest (MAJOR: show two image points whose difference has valuation below `E`... or exactly
  that the hull is larger than claimed). The `v_p(2)` term at `p = 2`: `cos` of `4 Z_2`, `8 Z_2`,
  `cosh` likewise; `cos 4 = 9 mod 16`.
- A false step in F10 to F14 (referee them: the term counts of F10 against Proposition 7 for an
  alternating series with half the terms; the divisibility by `p^D` and the working precision `W` of
  F11; the "onto" claim of F12 for `sin`, `sinh` by contraction; F13's exponent arithmetic at the
  limits; F14's loss rule at the real place).
- A wrong status: `DOMAIN` for a ball that meets the domain; `OK` for a ball that meets the complement
  (`2 Z_2`, `p^0 Z_p`, exact `2` at 2, exact `p^0` units); `NOT_DETERMINED` where the whole ball is
  inside or outside; the exact 0 against a ball around 0; `LIMIT` decided after work or refused for a
  small result (`N` near `ADF_LBALL_EXP_MAX`, `v` near the bounds); outputs changed on a status other
  than OK; aliasing `y = x` in every function.
- `_at` forms: the wrong component, `where` not the prime on a status, the real `sinh_at`, `cosh_at`
  against `arb` with the loss rule of `exp_at` (F14), the precision limit `ADF_REAL_PREC_MAX`.
- Driver: a command of `tools/adf/README.md` with an operand of another type (unit coset, idele,
  class, complex adele) that prints a value instead of `error: UNSUPPORTED` / `DOMAIN` / `PARSE`;
  the text of a local ball result against the library's value.
- The reviewed `exp`, `log`, `Log`: any change of result (the lane says none; the stored fixtures
  `tests/ref/vectors/f-slice5/stored.jsonl`, `stored_large.jsonl` pass; compare a few hundred of your
  own inputs between `src/lfunc.c` at `27f7e5f`, the commit before the merge (`git show`, read-only) built under renamed symbols
  and the current one).
- A test of the lane that cannot fail; a sentence of a header or of N-D12 that the code does not
  keep; a cost that is clearly avoidable (the lane names one: the absent parity visited; measure it
  once at `N = 2000`, `p = 3`, not more).

## Report

`lanes/f-review5/report.md`, written once, at the end; running notes in `lanes/f-review5/progress.md`
as you go (if the session is cut off, they are the report). For each finding: severity (BLOCKER: a
wrong enclosure, a wrong exponent that loses points, a memory fault, undefined behaviour; MAJOR; MINOR),
the input, what the code returns, what is true and why, and the command that reproduces it with a
program in your lane directory. Then what you attacked without result, with counts and what would have
made a case fail. No praise, no summary.
