# Lane f-slice8: local roots (milestone 1F, WP 1F.5)

The n-th roots of a local ball at a prime: existence by the criterion of `docs/proofs/functions.md`
Proposition 13 (`n | m`; the root of unity `w` an n-th power in the torsion; `v(log u) >= c + v(n)`),
the count (`gcd(n, p - 1)` roots at odd `p`, `gcd(n, 2)` at 2; 0 has the one root 0), the branches
(every root listed with an identifier, or one selected by a seed: a residue modulo `p` at odd `p`, a sign
at 2; "whatever the library returns" is not a branch, SPEC 9.3.3), and the precision of Proposition 15:
for an input ball `a + p^N Z_p` with `N - m >= c + v(n)` (the guard) the branch near `b` has the image
EXACTLY `b + p^(N - v(n) - (n-1) v(b)) Z_p`; outside the guard the status is `NOT_DETERMINED` (SPEC
9.3.3 keeps the strong guard; Remark 15r, odd `n` at 2 with `N - m >= 1`, is an optional refinement:
implement it only if it costs nothing, and say so). Square roots at 2 (`m` even and the unit part 1
modulo 8; 3 has no square root in `Q_2`, 9 has `+-3`, although the derivative `2x` is not a unit, so
simple Hensel lifting misses it), degree divisible by `p` (unit `p`-th roots lose one digit), negative
`m`, degree 1 (the identity), the exact 0 and balls containing 0 (`NOT_DETERMINED`: an uncertain zero,
conventions 3.1), an exact input whose root is rational (then the exact root; decide how you detect it:
`fmpz_root` on numerator and denominator of the unit part is the pattern of Proposition 16) and an exact
input whose root is irrational (a ball result at the requested absolute precision `N`, as `lfunc.h` does
for `exp`). Then `adf_sball_sqrt_at`, `adf_sball_root_at` at a prime (`include/adelefeld/rfunc.h`,
`UNSUPPORTED` at a prime now; at a prime they need a branch: decide the signature, for instance a seed
argument on new `_at` functions, and keep the two existing ones `UNSUPPORTED` at a prime if their
signature cannot select a branch; say what you decided), and driver commands in the pattern of
`exp_at` (`tools/adf/adf.c`, N-D10): the user asks for all roots at a place and sees them with their
identifiers, or one by a seed.

This is intricate numerical work on proved statements: a result must contain the branch's root of EVERY
point of the input ball, with exactly the exponent Proposition 15 gives (and the tests must tell
"exactly" from "safe"); a status is never a guess. The decomposition `a = p^m w u` exists:
`adf_lball_decompose_teich` (`lball.h`, line 315: `m`, the Teichmueller `w` and its index, the principal
unit `u`), `adf_lball_teichmuller` (line 290); `adf_lball_log`, `adf_lball_exp` (`lfunc.h`) give the
principal-unit root `exp(log(u)/n)` of Proposition 13 step 2, where the division by `n` needs the
guard; `adf_lball_pow_si` (line 355) checks a root. Decide, as a numerical analyst would, whether the
root is computed by `exp(log(u)/n)` through `lfunc.h` or by Newton's iteration on `T^n - a` with a
proved starting residue and a proved number of steps (Lemma 3 item 2 is the simple-root lifting; at
`p | n` the derivative is not a unit and the exp/log route or a shifted iteration is needed): the exp/log
route is the proved one and is the default; an iteration needs its own proved statement. Decisions where
the specification is silent are yours, listed with the alternatives in the report; the orchestrator
records them in `docs/SPEC.md` 15.4.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold
for `lfunc.h`, `rfunc.h`: you add declarations; rule 5 is replaced by item 4 below), `docs/SPEC.md`
9.3.1, 9.3.3, 15.4 (N-D7, N-D9, N-D10, N-D12), `docs/proofs/functions.md` Proposition 4, Lemma 3, Lemma 9,
Propositions 11, 13, 15, Remark 15r, Proposition 16 (cite file and line in the code), `docs/PLAN.md` rows
1F.5, 1F.6 (integer powers exist: `adf_lball_pow_si`; rational powers come after this slice),
`include/adelefeld/lball.h`, `lfunc.h`, `rfunc.h`, `src/lball_decomp.c`, `src/lfunc.c` (`adf_lball_exp`,
`adf_lball_log`, `adf_lball_Log`: the pattern of a function of this family, its statuses, limits, F1 to
F7), `docs/api-1f4.md` (F1 to F14), `docs/api-1f.md`, `tests/test_lfunc.c`, `tests/test_lfunc_trig.c`
(lane f-slice7, merged just before you started: the pattern of an exact oracle in `proto/` with a
fixture under `tests/ref/vectors/`), `tools/adf/adf.c` (`exp_at`, `sin_at`), `tools/adf/README.md`,
`docs/reviews/f1/review-lfunc.md` (what a reviewer of this family looks for).

**You own:** `include/adelefeld/lroot.h` (new) and `src/lroot.c` (new) for the local roots (or additions to
`lfunc.h`/`src/lfunc.c` if you prefer; say which), `include/adelefeld/rfunc.h` and `src/rfunc.c` (the
`_at` forms at a prime), `tests/test_lroot.c` (new), `tests/test_rfunc_prime.c` (additions),
`tests/julia/lroot.jl` (new), `proto/lroot_checks.py` (new; exact arithmetic, the oracle of this slice),
`tests/ref/vectors/f-slice8/` (new, below 1 MB), `docs/api-1f5.md` (new: the statements you add and their
proofs, in the style of `docs/api-1f4.md`), `tools/adf/adf.c`, `tools/adf/README.md`,
`tests/driver/root-*.cmd` and `.out` (new), `lanes/f-slice8/`. In `include/adelefeld.h` and
`tests/test_julia.sh` you may add the lines your files need. Everything else is read-only (another lane
is REVIEWING `src/text.c`, `src/lball.c`, `src/sball.c` at this moment and writes nothing). No git command
that changes state, no `bd`. Build into `BUILD=lanes/f-slice8/build` while you work and into `build/`
only for the final checks.

**Battery.** The machine runs on battery today. Keep the compute small: at most 2 cores; no benchmark;
no program over 60 s and every one under `timeout`; no repeated clean rebuilds (build once, then
incrementally); enumeration oracles over small balls only (`p` in 2, 3, 5, 7; exponents up to about 8;
`n` up to 12), random comparisons of a few thousand calls. At the end ONE `make -j2 check-all` (no
`make clean` before it; the whole run takes about 6 minutes on this machine and MUST be allowed to
finish: `timeout 900`) and ONE `ASAN_OPTIONS=detect_leaks=0 timeout 120 make -j2 BUILD=build/san SAN=1
build/san/test_lroot build/san/test_rfunc_prime` followed by running those two binaries (the codex
sandbox cannot run LeakSanitizer; the orchestrator runs the default sanitizer suite, clang, INV and
`check_headers.sh` on master). Do not run the clang or INV suites.

1. Header first: the comment block of every declaration (set statement, proposition with file and
   line, the exponent rule, the branch identifier, aliasing, statuses `DOMAIN` (no root at any point of
   the ball: the criterion fails for the whole ball), `NOT_DETERMINED` (the guard fails, or the ball
   meets both cases, or contains 0), `LIMIT` as in `lfunc.h`, the exact values).
2. Tests first, red then green (`lanes/f-slice8/redgreen.log`). Oracles, each with a STATED output
   precision:
   - enumeration: for `p` in 2, 3, 5, 7, `n` in 1..12 and every ball of small exponent, the set of
     n-th roots of EVERY point of the ball modulo a higher power (your reference: exact integers
     modulo `p^H`, roots by exhaustive search modulo `p^H`, `H` chosen and justified) is covered by the
     returned branches, each branch's result ball has exactly the exponent of Proposition 15 (show a
     point of the image at distance exactly `p^E`), and the count of branches is `gcd(n, p - 1)` or
     `gcd(n, 2)`;
   - the examples of the PLAN and of Proposition 13 step 4 and Proposition 15 step 5: 3 has no square
     root at 2, 9 has `+-3`; `1 + 4 Z_2` with `n = 2` is `NOT_DETERMINED` (5 is not a square); `1 + 3 Z_3`
     with `n = 3` is `NOT_DETERMINED`; unit square roots lose nothing at odd `p` and one digit at 2;
     unit `p`-th roots lose one digit; negative `m`; `n = 1`;
   - exact inputs with a rational root (exact result), without one (ball at `N`), the exact 0;
   - every status, every aliasing combination, the prime `2^64 - 59` at `N` up to 200, the limits
     (`LIMIT`, outputs untouched), `n` up to `WORD_MAX` (the count `gcd(n, p - 1)` and `v(n)` must not
     overflow; `pow_si` of the result as a check where it is cheap);
   - `_at` forms at a prime and driver golden files.
3. The code. `tests/julia/lroot.jl`: a user computes the two square roots of 9 at 5 to 20 digits (as
   balls or exact), sees `DOMAIN` for the square root of 3 at 2 and the cube root of 2 at 7 (`2` is a
   unit; is it a cube modulo 7? your reference decides), and `NOT_DETERMINED` for `1 + 4 Z_2`.
4. Show that the tests bite: five faults in a scratch copy under your build directory (among them: the
   guard off by one; the exponent of the result without the `(n-1) v(b)` term; a branch missing when
   `gcd(n, p - 1) > 1`; the criterion at 2 with modulo 4 instead of modulo 8). No mutation run, no
   fuzz target.
5. Final checks as under "Battery"; give the last line of each.

Report: `lanes/f-slice8/report.md`, written ONCE, AT THE END; running notes in
`lanes/f-slice8/progress.md` (write them as you go: if your session is cut off, they are the report).
In it: functions built; decisions and their alternatives; what is proved (the new statements) and what
is not; the numbers of the tests and what would have made a case fail; the fault table; findings
against the specification; the next slice you propose (1F.6 rational powers and principal-unit powers).
