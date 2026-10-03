# Lane f-repair5: the three minors of review f-review7 (powers at a prime, branch list of the roots) and the shared Teichmueller lift

Review f-review7 (`lanes/f-review7/progress.md`: the lane's notes and the orchestrator's addendum; the lane was cut
off before its report) found no wrong enclosure, exponent, status or branch list in the powers at a prime (lane
f-slice9: `include/adelefeld/lpow.h`, `src/lpow.c`, `docs/api-1f6.md`) or in the branch enumeration of the roots
(lane f-repair4: `src/lroot.c`, statement R8 of `docs/api-1f5.md`). It found three minors. Lane f-repair4 named one
avoidable cost that nobody removed (`lanes/f-repair4/result.md`, "Findings against the specification or the
review", item 3). You repair the four. The values the library returns do not change except where item 2 turns a
`LIMIT` into `OK`.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold for the
comment blocks of `lpow.h` and `lroot.h`: you may change their sentences and the statements they cite; the
declarations stay; rule 7 does not hold for item 4, which is a cost repair), `lanes/f-review7/progress.md` with
`limits.in` and `early.in`, `lanes/f-repair4/brief.md` and `result.md` (how F1 of f-review6, the same shape as
item 2, was repaired), `docs/api-1f6.md` (P4 to P6, P8), `docs/api-1f5.md` (R2, R6, R8), `docs/api-1f.md` L12
(`adf_lball_pow_si`), `lpow.h`, `lroot.h`, `src/lpow.c`, `src/lroot.c`, `include/adelefeld/lball.h`
(`adf_lball_teichmuller`, `adf_lball_pow_si`), `tests/test_lpow.c`, `tests/test_lroot.c`. `refs/src/` is on disk
(FLINT is `refs/src/flint-3.0.1/`); cite by file and line.

**You own:** `include/adelefeld/lpow.h` and `include/adelefeld/lroot.h` (comment blocks only), `src/lpow.c`,
`src/lroot.c`, `docs/api-1f5.md`, `docs/api-1f6.md`, `tests/test_lpow.c`, `tests/test_lroot.c`,
`lanes/f-repair4/result.md` (ONE appended erratum paragraph, nothing else), `lanes/f-repair5/`. Everything else is
read-only. No git command that changes state, no `bd`. At most 2 cores; every program under `timeout`; build into
`BUILD=lanes/f-repair5/build` while you work. Another session on this machine runs emulators: keep to 2 cores.

## The four repairs

1. **Stale cost sentence** (finding 1). `lpow.h` (the comment of `adf_lball_powunit`, near line 114) says "Cost: one
   Log at min(A, N - B) for alpha"; `principal()` in `src/lpow.c` computes alpha by one exact subtraction (decision
   10 of `docs/api-1f6.md`). State the cost the code has.
2. **`LIMIT` for a small result** (finding 2). `powunit(6 + 5^(2^40) Z_5, s = 1 or 2 exact, N = 2^40)` is `LIMIT`;
   the image is `6 + 5^(2^40) Z_5` (`36 + ...` for `s = 2`) and `adf_lball_pow_si` returns it `OK`
   (`lanes/f-review7/limits.in` lines 2 to 4). Decide and prove ONE of: (a) an exact exponent that is a rational
   integer fitting a `slong` is routed through `adf_lball_pow_si`, with a statement (P9, in `docs/api-1f6.md`) that
   the two results are the same SET and the same ball for every such input (Proposition 18 with an exact exponent
   gives `R = A + beta`; L12 gives the exponent of `pow_si`: show they agree, the 2-adic sign factor included, or
   show the case where they do not and exclude it), so that the route changes no result that was `OK` before;
   (b) the `LIMIT` stays and `lpow.h` says so with this example. Take (a) unless the proof fails; if it fails, say
   where, and take (b). The compatibility test of `tests/test_lpow.c` (powunit against `pow_si` on integer
   exponents) must be byte-identical before and after on every case that was `OK`.
3. **False order sentence** (finding 3). `lanes/f-repair4/result.md` says "early_status() now returns, before the
   listing, every LIMIT that some branch would return". For `1 + 7^(2^40) Z_7`, `n = 2`, `N = LONG_MAX`,
   `early_status` returns `OK`, branch 1 is computed, branch 6 then returns `LIMIT` from `unit_mod`; the same at 2
   for `1 + 2^(2^27) Z_2`, `n = 2` (`lanes/f-review7/early.in` lines 8 to 13). The status is right and the outputs
   are untouched. Look for the same claim in `lroot.h` and `docs/api-1f5.md` (line 31: "That LIMIT is decided
   before any branch is listed (R6 step 6)"; line 203): correct every sentence that the two inputs refute, and
   append one erratum paragraph to `lanes/f-repair4/result.md` (dated 2026-10-03, naming f-review7 finding 3).
   Either make `early_status` decide these two cases before the listing (if that is a few lines and a proof), or
   state exactly which `LIMIT` is early and which may come after some branches were computed. Add the two inputs
   as tests (status `LIMIT`, outputs untouched).
4. **One Teichmueller lift per branch.** `adf_lball_roots` runs a Teichmueller Newton lift for every non-rational
   branch (`src/lroot.c` near line 216): 149 us per branch at `p = 2^64 - 59`, `N = 20`, 44.6 s for `d = 299756`.
   Teichmueller representatives are multiplicative: `omega(t0 zeta^i) = omega(t0) omega(zeta)^i` modulo `p^L`.
   Two lifts and `d` multiplications suffice. Prove it as a statement (R9: the representative is the unique root
   of `T^(p-1) = 1` congruent to the residue; a product of two such roots is such a root), implement it, and keep
   the result of every branch byte-identical to the present code (the reduced representative modulo `p^L` is
   unique, so identity is the right test): the stored fixtures and `adf_lball_root_seed` on each listed seed check
   it. The seeded single-branch function keeps its one lift.

## Order of work (red-green, `lanes/f-repair5/redgreen.log`)

1. Tests first. In `tests/test_lpow.c`: the three `limits.in` cases of item 2 (expected `OK` and the ball, under
   (a)). In `tests/test_lroot.c`: the two inputs of item 3; for item 4 a wall-clock guard (the pattern of the
   guards already in the file): all branches for a `d` of some thousands at `p = 2^64 - 59`, `N = 20`, in a time
   the present code misses by a factor of 5 or more and the new code meets with a margin of 5 or more on a loaded
   machine (measure both, give the numbers), and the identity of every listed branch with `adf_lball_root_seed`
   at its seed for a sample. See them red, then the code, then the documents.
2. Checks: build and run `test_lpow`, `test_lroot`, `test_rfunc_prime` in your build directory; the same three
   under `SAN=1` with `ASAN_OPTIONS=detect_leaks=1` in `BUILD=lanes/f-repair5/build-san`; the reviewer's attacks
   that take the library path, against your build: `lanes/f-review7/attack_roots.py`, `attack_roots_big.py`,
   `attack_shared.py`, `attack_early.py`, `attack_powunit.py` (read their heads; the harness `h.c` needs
   `-std=gnu11`; adapt copies in your lane directory if needed; give counts and failures); `timeout 900 make -j2
   check-all` ONCE at the end in `build/` (this one run may take 10 minutes; give its last line).
3. Mutation testing of the lines you changed in `src/lpow.c` and `src/lroot.c`, `lanes/COMMON-C.md` rule 5: at most
   40 mutants per file, `--seed 1`, 2 jobs, `--san`, `timeout 1300` in all. NOTE the tool's defect: `--san` is
   defeated when the `--make` string contains "SAN" (as in `ASAN_OPTIONS`), and mutants that did not compile are
   reported as killed: check in the log that the mutants compiled.

Report: `lanes/f-repair5/result.md`, written ONCE, AT THE END (the harness refuses the name `report.md` for a
Claude subagent); running notes in `lanes/f-repair5/progress.md` as you go. In it: the four repairs; the decision
of item 2 with its proof or the place where the proof failed; every check with its command and numbers and what
would have made a case fail; timings before and after for item 4; mutation survivors one line each; what is not
done; findings against the specification or the review. Give the same text as your final message.
