# Lane f-review7: adversarial review of the powers at a prime (lane f-slice9, WP 1F.6) and of the new branch enumeration of the roots (lane f-repair4)

Your task is to REFUTE, not to confirm. Two pieces of new code, both Claude Opus, both merged today, neither reviewed:

A. Lane f-slice9 (`lanes/f-slice9/result.md`): `adf_lball_powrat` (rational powers `x^(e/n)` on a branch named by
   the seed of `lroot.h`, the fraction reduced first) and `adf_lball_powunit` (`exp(s log u)` for a principal unit
   `u` and an exponent `s` in `Z_p`, both local balls; at 2 the odd units through `w^(s mod 2) exp(s log u')`) in
   `include/adelefeld/lpow.h`, `src/lpow.c`; the `_at` forms `adf_sball_powrat_at`, `powunit_at` (`rfunc.h`,
   `src/rfunc.c`); the driver commands `powrat_at`, `powunit_at` (`tools/adf/adf.c`, `tools/adf/README.md`); the
   statements P1 to P8 of `docs/api-1f6.md` with their proofs; decision N-D15 (`docs/SPEC.md` 15.4). Its oracle is
   `proto/lpow_checks.py` (fixtures `tests/ref/vectors/f-slice9/`), its tests `tests/test_lpow.c`, additions to
   `tests/test_rfunc_prime.c`, `tests/julia/lpow.jl`, `tests/driver/pow-*`.
B. Lane f-repair4 (`lanes/f-repair4/result.md`, section F2): in `src/lroot.c` the branches of `adf_lball_roots` are
   now listed as `t0 zeta^i` with `zeta = g^((p-1)/d)` for a primitive root `g`, and `t0` one root of `T^d = w^e`
   built by CRT idempotents of `p - 1` and a digit search in each Sylow `q`-part (`dth_root`, `identifiers`);
   statement R8 of `docs/api-1f5.md`; the early `LIMIT` (`early_status`); the shared principal-unit root. Decision
   N-D14 (`K = min(N, E)` for ball roots).

Contract: `docs/SPEC.md` 9.3.3, 9.3.4, 15.4 (N-D13, N-D14, N-D15); `docs/proofs/functions.md` Lemma 3, Lemma 9,
Propositions 11, 13, 15, 17, 18; `docs/api-1f.md` L12 (`pow_si`); `docs/api-1f5.md` R1 to R8; `docs/api-1f6.md`
P1 to P8; `docs/conventions.md` 3.1, 3.2, 4.1, 4.3; the header comment blocks. Read `docs/reviews/f1/
review-lroot.md` and `lanes/f-review6/oracle.py` for what the review of this family looked like and how its oracle
was built (exact integers only, no logarithm); `lanes/f-review6/brief.md` is the pattern of this brief.

**You own:** `lanes/f-review7/` only. Everything else is read-only. No git command that changes state, no `bd`. At
most 2 cores. Build the archive ONCE with `make -j2 BUILD=lanes/f-review7/build lanes/f-review7/build/libadelefeld.a`
and link your programs against it (`-Iinclude -lflint -lgmp -lm`); do not run the test suites of the repository.
Every program under `timeout`; none over 120 s; a few thousand inputs per attack and small balls for enumeration.
Compile reproducers with `-fsanitize=address,undefined` where memory is in question. `refs/src/` is on disk.

## What counts as a finding

- A wrong enclosure (your OWN oracle in exact integers, with a stated precision): a point `(t, sigma)` of the input
  balls whose `t^sigma` (principal-unit powers: integer exponents `s_0 + p^B k` are dense in the exponent ball;
  powers modulo `p^H` by repeated squaring) or whose branch root raised to `e'` lies outside the result ball; for
  `p` in 2, 3, 5, 7, 13, 65537, `2^64 - 59`; `e/n` reduced and not, `e < 0`, `n` divisible by `p`; `A`, `B` small
  and large; `alpha`, `beta` finite and infinite (`u = 1`, `s = 0`, exact and as balls); inputs of thousands of bits.
- A wrong exponent: `min(N, E')` and `min(N, R)` claimed where the image is not inside (BLOCKER), or where the
  exact image is promised and the result is strictly larger (MAJOR; show two image points at distance exactly
  `p^E` or `p^R`, or that none exist). Proposition 18's `R = min(A + beta, B + alpha, A + B)`: the three terms each
  active; the 2-adic hull `1 + 2 Z_2` when the sign or parity is not fixed (is it the SMALLEST ball containing the
  union, as N-D15 claims? P5 says the union misses `5 + 8 Z_2` for sign `-1`, `B = 0`: check it, and check the cases
  where the lane returns an exact image "when that is one coset").
- A wrong status: `DOMAIN` where some point has a power; `OK` where the input meets the complement of the domain
  (`u` not a principal unit; `s` with negative valuation points); `NOT_DETERMINED` where the whole ball decides;
  the disagreement between `powrat` with `n' = 1` and `pow_si` (must be exact, statuses included); `powrat` with
  `n' >= 2` against `root_seed` then `pow_si`; `LIMIT` after work or for a small result; outputs changed on a status
  other than `OK`; aliasing `y = x`, `y = u`, `y = s`, `u = s`.
- B: a wrong branch list: compare `adf_lball_roots` with a search over all residues for every odd `p < 300` and `n`
  up to 40 (identifiers, order, count `gcd(n, p-1)`), then at `p = 65537` (`d` in 2, 4, 16, 256, 65536), at
  `p = 2^64 - 59` (`d` dividing `p - 1 = 2^2 * 3 * ...`: factor it; `d` with several prime factors, so that several
  Sylow parts are active in `dth_root`) with `t^d = w^e` and distinctness checked; `t0^d != A` cases (must not be
  listed); the early `LIMIT` against the per-branch one (same status, outputs untouched); the shared principal-unit
  root against a branch computed alone; N-D14 cases `N < E`, `N <= j`, `N = LONG_MIN` (`LIMIT` now).
- A false step in P1 to P8 or R8 (referee them: P1's counterexample `2/2` at 5 and the order root-then-power; P2's
  `E' = e' j + (M - m) - v_p(n') + v_p(e')` with `e' < 0` and `j < 0`; P3's working precision; P5's 2-adic cases; P6's
  `alpha = v(w0 u0 - 1)`; R8's distinctness and completeness and the digit search).
- `_at` forms and driver: the wrong component, `where`, the real place (`UNSUPPORTED`), an exponent text that is
  not a reduced rational, operands of other types, a fifth operand; the printed values against the library's.
- A test of the lanes that cannot fail; a sentence of `lpow.h`, `lroot.h`, N-D14, N-D15 that the code does not keep;
  a cost that is clearly avoidable (measure once, not more).

## Report

`lanes/f-review7/result.md`, written once, at the end (the harness refuses the name `report.md` for a Claude
subagent); running notes in `lanes/f-review7/progress.md` as you go (if the session is cut off, they are the
report). For each finding: severity (BLOCKER: a wrong enclosure, a wrong exponent that loses points, a memory fault,
undefined behaviour; MAJOR; MINOR), the input, what the code returns, what is true and why, and the command that
reproduces it with a program in your lane directory. Then what you attacked without result, with counts and what
would have made a case fail. No praise, no summary. Give the same text as your final message.
