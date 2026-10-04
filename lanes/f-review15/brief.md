# Lane f-review15: adversarial review of the local zeta factors (lane f-slice14, `src/localfactor.c`)

Your task is to REFUTE, not to confirm. Lane f-slice14 (Claude Opus; you are of another model family on
purpose) landed on 2026-10-05, not reviewed: `include/adelefeld/localfactor.h`, `src/localfactor.c`:
`adf_local_zeta_factor_at(y, where, s, v, prec)`: `(1 - p^(-s))^(-1)` at a prime and `pi^(-s/2) Gamma(s/2)` at
the real place on complex balls (`acb`); the driver command `local_zeta_factor_at` (`tools/adf/adf.c`,
`tools/adf/README.md`); `tests/test_localfactor.c`, `tests/ref/vectors/f-slice14/zeta.jsonl`,
`tests/julia/localfactor.jl`, `tests/driver/localfactor-*`; statements Y16, Y17 (`docs/api-1f9.md`, the last
106 lines). Contract: the comment block of `localfactor.h`; `docs/SPEC.md` 9.3.7 (row "local zeta factor");
`docs/conventions.md` lines 944-956 and the status table (row "Characters, Gauss sums, local factors"); the
design `docs/design/local-zeta.md` (Z1 to Z9; the decision rule Z4 and its proof; written by a model of YOUR
family, lane d-zeta: it is under review too where the code relies on it). The lane's report:
`lanes/f-slice14/result.md` (read its "Findings against the design" and "Mutation testing": two surviving
mutants make the bound `B` of Z6 smaller and no test notices).

**You own:** `lanes/f-review15/` only. Everything else is read-only. No git command that changes state, no
`bd`. At most 2 cores. Build the archive once: `timeout 600 make -j2 BUILD=lanes/f-review15/build
lanes/f-review15/build/libadelefeld.a`, link your programs against it (`-Iinclude -lflint -lgmp -lm`). Do not
run the repository's suites as a whole. Every program under `timeout`, none over 170 s.

The contract in two sentences: `OK` is never returned for an input ball that contains a pole, and on `OK` the
returned ball contains `L_v(s)` for EVERY point `s` of the input ball; on every other status `y` is untouched
and no non-finite ball is stored. A violation of either is a BLOCKER.

Hunt, in this order; stop a line of attack after a few thousand cases without a finding:
1. **Your own oracle**, not `proto/zeta_checks.py` (do not import it) and not `acb_gamma` or `acb_pow` of the
   same FLINT: `mpmath` at 400 digits or more with a stated margin (`mpmath.gamma`, `mpmath.power`), and for
   the finite factor exact rational checks where they exist (`s` a real integer: `p^s / (p^s - 1)`).
2. **Poles inside `OK` balls.** At primes 2, 3, 5, 7, 65537, `2^64 - 59`: poles `2 pi i k / log p` for `k` up
   to `10^6` and for huge `k` (`Im s` near `2^100`, `2^1000`); boxes whose CLOSED boundary touches the pole
   (compute the pole to 2000 bits; a midpoint a few ulps off it with a radius just above and just below the
   distance: just above must not be `OK`; just below, if `OK`, must contain the value at sampled points
   including the corner nearest the pole); thin boxes (one radius 0); boxes of huge radius containing
   thousands of poles; `Re s` slightly off 0 with a radius reaching it. At the real place: `s = -2 n` for `n`
   up to `10^6` and `2^200`, boxes touching a pole at an end point, boxes between two poles of width almost 2,
   boxes whose imaginary interval excludes 0 by one ulp, exact non-dyadic-looking inputs (`-2` plus `2^-5000`).
3. **Containment on `OK`.** 5000 random boxes at each place (midpoints of every size from `2^-200` to `2^200`,
   radii from 0 to 1, both signs of both components), `prec` from 2 to 4096: 20 sample points of each box
   (corners, edge midpoints, random interior), the true value from your oracle, contained in the result? Then
   the same near poles (distance `d`, radius `d/2`, `d/16`, `d/1000`). The real place where the code leaves
   direct Gamma: the recurrence fallback (`n` up to 64; the product computed by a loop since `acb_rising_ui`
   failed), and above all the REFINEMENT (Y16; design Z6): the result is intersected with a midpoint value
   enlarged by `R B`, where `B` must bound `|L'|` on the whole box. If `B` is too small the intersection
   cuts true values off. Compute `sup |L'|` on boxes by dense sampling with your oracle and compare with what
   the code uses (instrument a scratch copy under your build directory to print `B`), on boxes near poles, on
   boxes far in the left half-plane, on wide boxes: one box with `B < sup |L'|` is a finding even if the
   final ball still contains the samples (say which).
4. **Statuses and order**: every status the header names, with the inputs that give it; `LIMIT` for `prec`
   above the cap decided first; an exact pole far left (`s = -200`) is `DOMAIN`, not `LIMIT`; a ball
   containing a far-left pole is `NOT_DETERMINED`, not `LIMIT`; non-finite `s` (each of the four parts NaN or
   infinite); `y` untouched on every failure (compare the representation, with `y = s` too); `where` written
   on failure and untouched on `OK`, `where = NULL`; a forged place handle and non-prime `p` (what does the
   header promise?).
5. **Cost**: time at `prec` 64, 4096, `2^16`, `2^20`, at `p = 2` and the real place; `Im s = 2^1000`,
   `Re s = +-2^1000`; midpoints with exponent near `2^60` and `-2^60` (the lane did not measure these): a call
   that does not return within 60 s where the header promises a status is a finding (MAJOR), with the input.
6. **Memory**: two of your programs under `-fsanitize=address,undefined` (leak detection off if your sandbox
   cannot ptrace: say so).
7. **The driver**: thirty hostile lines for `local_zeta_factor_at` (operand kinds, a real adele in place of a
   complex one, places that are not places, 4 as a prime, `2^64`, balls with radius in one component only);
   printed values against the library's.
8. **Tests that cannot fail**: plant at least ten faults of your own in a scratch copy of `src/localfactor.c`
   (among them: `B` halved; the refinement applied without the enlargement `R B`; the radius `Eplus` of Z4
   step 3 computed with the lower instead of the upper bound; the closed integer test made open at one end;
   the fallback allowed for `n = 65`; rounding to `prec` inward; the stable sign chosen by the wrong
   comparison; `log p` taken at `prec` instead of the working precision without its radius), build
   `test_localfactor` against each and record which are detected. A fault that the test passes is a finding
   (MAJOR if it gives a wrong enclosure or status), with the smallest input that distinguishes it.

Report: `lanes/f-review15/report.md`, written once, at the end; notes in `lanes/f-review15/progress.md` as you
go. For each finding: severity (BLOCKER: an `OK` for a ball with a pole, a true value outside a returned
ball, a non-finite ball stored, a memory fault, undefined behaviour; MAJOR; MINOR), the input (exact: mantissas
and exponents), what the code returns, what is true and why, the command that reproduces it. Then what you
attacked without result, with counts and what would have made a case fail; the fault table. No praise, no
summary. Delete `lanes/f-review15/build` and your executables at the end.
