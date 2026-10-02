# Lane f-review6: adversarial review of the local roots (lane f-slice8, WP 1F.5)

Your task is to REFUTE, not to confirm. Lane f-slice8 (`lanes/f-slice8/report.md`, codex gpt-6-astra, merged
`2fa244b`) built `adf_lball_root_count`, `adf_lball_root_seed`, `adf_lball_sqrt_seed`, `adf_lball_roots`
(`include/adelefeld/lroot.h`, `src/lroot.c`), the `_at` forms `adf_sball_root_seed_at`, `sqrt_seed_at`,
`roots_at` at a prime (`include/adelefeld/rfunc.h`, `src/rfunc.c`), the driver commands `roots_at`, `root_at`
(`tools/adf/adf.c`, `tools/adf/README.md`), and the statements R1 to R7 of `docs/api-1f5.md` with their
proofs. Its oracle is `proto/lroot_checks.py` (fixtures under `tests/ref/vectors/f-slice8/`); its tests are
`tests/test_lroot.c`, additions to `tests/test_rfunc_prime.c`, `tests/julia/lroot.jl`, `tests/driver/root-*`.
Decision N-D13 (`docs/SPEC.md` 15.4): a BALL input returns the exact image exponent
`E = M - v_p(n) - (n-1) v_p(b)` of Proposition 15 regardless of the requested `N` (unlike `lfunc.h`, where
`K = min(N, E)`); an exact input returns the ball at `N`, or the exact rational root where one exists; outside
the guard `N - m >= c + v(n)` the status is `NOT_DETERMINED`; an invalid degree or seed is `DOMAIN`.
Contract: `docs/SPEC.md` 9.3.1, 9.3.3; `docs/proofs/functions.md` Lemma 3, Proposition 4, Lemma 9,
Propositions 11, 13, 15, Remark 15r, Proposition 16; `docs/conventions.md` 3.1, 3.2, 4.1, 4.3; the header
comment blocks. Read `docs/reviews/f1/review-lfunc.md` and `docs/reviews/f1/review-lfunc-trig.md` for what a
review of this family found before, and `lanes/f-review5/brief.md` for the pattern of this brief.

**You own:** `lanes/f-review6/` only. Everything else is read-only. No git command that changes state, no
`bd`. At most 2 cores. Build the archive ONCE with `make -j2 BUILD=lanes/f-review6/build` and link your
programs against it (`-Iinclude -lflint -lgmp -lm`); do not run the test suites of the repository (the
orchestrator has run them). Every program under `timeout`; no program over 120 s; a few thousand inputs per
attack and small balls for enumeration (not millions). Compile reproducers with `-fsanitize=address,undefined`
where memory is in question. `refs/src/` is NOT on disk in this container: the proofs under `docs/proofs/` and
`docs/api-*.md` are what you referee; a claim that needs a source is marked `[source pending: ...]`.

## What counts as a finding

- A wrong enclosure: a point `t` of an admitted input ball whose root in the stated branch lies outside the
  result ball (your OWN oracle, not the lane's: exhaustive n-th roots modulo `p^H` in exact integers, or
  Hensel lifting in Python integers with a proved step count; state its precision), for `p` in 2, 3, 5, 7,
  13, 65537, `2^64 - 59`; `n` in 1..12, `n = p`, `n = p^2`, `n` a multiple of `p - 1`, `n` of a word; exact
  inputs and balls; `m` zero, positive, negative, divisible and not divisible by `n`; `N` below, at and
  above the guard; inputs of thousands of bits.
- A wrong exponent (N-D13 promises the EXACT image for a ball): a result with exponent `E` where the image
  of the branch is not inside `b + p^E Z_p` (BLOCKER); or where the image is strictly smaller than the result
  ball, i.e. the result is a safe enlargement and not the exact image (MAJOR): show two image points whose
  difference has valuation exactly `E`, or show that none exist. The `(n-1) v_p(b)` term with `v_p(b) < 0`;
  the `v_p(n)` term at `n = p^k`; the one-digit loss of unit square roots at 2 and of unit `p`-th roots.
- A wrong count or a missing branch: `gcd(n, p-1)` at odd `p`, `gcd(n, 2)` at 2; the identifiers (the unit
  residue of the root modulo `p` at odd `p`; 1 and 3 for the signs at 2); two branches with the same
  identifier; R5 (the degree `gcd(n, p-1)` polynomial `T^d - w^e`): referee the proof and test it at `p = 13`,
  `p = 65537` with `n` sharing a large factor with `p - 1`.
- A wrong status: `DOMAIN` for a ball some point of which has a root; `OK` or `DOMAIN` outside the guard
  (must be `NOT_DETERMINED`); the criterion at 2 for squares (1 modulo 8, not modulo 4; 3 has no square root,
  9 has `+-3`, 17 has, 5 has not); `1 + 4 Z_2` with `n = 2`; `1 + 3 Z_3` with `n = 3`; the exact 0 (one root 0)
  against a ball containing 0 (`NOT_DETERMINED` for `n >= 2`, the identity for `n = 1`); `n = 0`; a seed that
  is not an identifier (`DOMAIN`), a seed of a valid branch that the code rejects; `LIMIT` decided after work
  or refused for a small result (`N` and `v` near `ADF_LBALL_EXP_MAX`; `ADF_LROOT_BRANCH_MAX`); `capacity`
  too small, negative, exactly the count; outputs changed on a status other than `OK` (all slots of `y`,
  `ids`, `len`, including a late `LIMIT` after an exact first branch); aliasing `x` with any slot of `y`.
- Exact inputs: a rational root missed (`-8` at 3 with `n = 3`; `1/9` at 5 with `n = 2`; `-1` at 2 with
  `n = 3`; `16` at 5 with `n = 4`, both signs; large numerators) or claimed where none exists; the sign of a
  rational root of even degree; a ball at `N` whose centre is not the root modulo `p^N`; the behaviour at
  `N <= j` (R4 step 5); a huge `N` for a rational branch (must be ignored, no power formed).
- A false step in R1 to R7 (referee them): R1's constancy of the criterion on the guard; R2's "exactly
  `b + p^E Z_p`" at `p = 2` with `r - s = c`; R3's completeness of the rational-root test for negative `A`
  and even `n`; R4's working precision `L + s` for `Log` and the exactness of the division by `n`; R6's
  overflow arithmetic (`j`, `E`, `K - j`, `L + s`) at the limits.
- `_at` forms: the wrong component, `where` not the prime on a status, the real place (`UNSUPPORTED` after
  the membership check), a place that is not a place of the ball, the array of `roots_at` against the
  storage of the partial ball, aliasing `y = x`.
- Driver: `roots_at`, `root_at` with an operand of another type (unit coset, idele, class, complex adele,
  a real ball), a prime that is not a prime, a degree 0 or non-numeric, a seed `-1` at an odd prime, a fifth
  operand, a trailing ` with `; the printed text of a root against the library's value.
- A test of the lane that cannot fail; a sentence of `lroot.h`, `docs/api-1f5.md` or N-D13 that the code
  does not keep; a cost that is clearly avoidable (the lane names one: all-branch evaluation recomputes the
  principal-unit logarithm and exponential for each branch; measure it once at `p = 65537`, `n = 16`,
  `N = 200`, not more). Say in one paragraph whether N-D13's exponent rule for balls (the exact image, not
  `min(N, E)`) is what a user of the library wants, with a reason; it is the item open to reversal.

## Report

`lanes/f-review6/result.md`, written once, at the end (the harness refuses the name `report.md` for a Claude
subagent; the orchestrator reads `result.md`); running notes in `lanes/f-review6/progress.md` as you go (if
the session is cut off, they are the report). For each finding: severity (BLOCKER: a wrong enclosure, a
wrong exponent that loses points, a memory fault, undefined behaviour; MAJOR; MINOR), the input, what the
code returns, what is true and why, and the command that reproduces it with a program in your lane
directory. Then what you attacked without result, with counts and what would have made a case fail. No
praise, no summary. Give the same text as your final message.
