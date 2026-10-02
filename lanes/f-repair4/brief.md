# Lane f-repair4: the findings of review f-review6 on the local roots (1F.5) and decision N-D14

Review f-review6 (`docs/reviews/f1/review-lroot.md`; its programs and oracle are in `lanes/f-review6/`) found no
wrong result in the local roots of lane f-slice8 (`include/adelefeld/lroot.h`, `src/lroot.c`; the `_at` forms in
`include/adelefeld/rfunc.h`, `src/rfunc.c`; the driver commands `roots_at`, `root_at` in `tools/adf/adf.c`; the
statements R1 to R7 of `docs/api-1f5.md`; the oracle `proto/lroot_checks.py` with fixtures under
`tests/ref/vectors/f-slice8/`; the tests `tests/test_lroot.c`, `tests/test_rfunc_prime.c`, `tests/julia/lroot.jl`,
`tests/driver/root-*`). It found four minors, F1 to F4, and recommended reversing the exponent rule of N-D13 for
ball inputs. The orchestrator took that decision as N-D14 (`docs/SPEC.md` 15.4, last row): read it first. You
implement N-D14 and repair F1 to F4. The mathematics of Propositions 13, 15, 16 (`docs/proofs/functions.md`) and of
R1 to R6 does not change; what changes is the exponent of a ball result, the order of two checks, the enumeration
of the branches and one sentence.

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold for
`lroot.h`: you may change its comment block and the statements it cites; its declarations stay), the review, N-D13
and N-D14, `docs/api-1f5.md`, `lroot.h`, `src/lroot.c`, `lanes/f-review6/result.md` and `oracle.py` (the reviewer's
own oracle, exact integers only; you may use it as a second oracle), `tests/test_lroot.c`, the F6 rule of
`docs/api-1f4.md` and its use in `src/lfunc.c` (the pattern `K = min(N, E)` you are to follow). `refs/src/` is on
disk; FLINT's `n_primitive_root_prime` is `refs/src/flint-3.0.1/ulong_extras.rst:1410`, `nmod_poly` roots
`nmod_poly.rst:2392-2396`. Cite by file and line.

**You own:** `include/adelefeld/lroot.h` (comment block), `src/lroot.c`, `docs/api-1f5.md`, `tests/test_lroot.c`,
`tests/test_rfunc_prime.c` (the root parts), `tests/julia/lroot.jl`, `tests/driver/root-*` (only if a golden line
changes; say which and why), `tools/adf/README.md` (only if the text of `roots_at`, `root_at` states the exponent
rule), `lanes/f-repair4/`. `src/rfunc.c` and `rfunc.h` only if a comment states the old rule. Everything else is
read-only; lane f-repair3 is writing `src/lfunc.c`, `docs/api-1f4.md`, `tests/test_lfunc_trig.c` at this moment.
No git command that changes state, no `bd`. At most 2 cores; every program under `timeout`; build into
`BUILD=lanes/f-repair4/build` while you work. Julia is installed (`julia` on the PATH).

## The four repairs and the decision

1. **N-D14.** For a BALL input, `K = min(N, E)`, `E = M - s - (n-1) j` as before: the result is `b + p^E Z_p`
   (the exact image, R2) when `N >= E`, and the ball at `N` containing it otherwise; for `N <= j` the centre is 0 at
   exponent `N` (R4 step 5, which already exists). Exact inputs are unchanged. The `_at` forms, the driver and the
   Julia example inherit the rule. Rewrite the sentences of `lroot.h` and of R2, R4, R7 that state "regardless of
   N" or "K = E for ball inputs"; keep the proofs, which prove the `N >= E` case; add the one line that proves the
   `N < E` case (the image is inside the ball at `N` because `E > N`).
2. **F1.** `src/lroot.c:154` tests `power_ok(p, L + s)` before calling `Log` whether or not that power is formed, so
   `1 + 2^(2^27) Z_2` with `n = 2` is `LIMIT` although its image `1 + 2^(2^27 - 1) Z_2` is inside the limits and
   `adf_lball_Log` and `adf_lball_pow_si` return `OK` on it. Test the power only where it is formed (the exact unit
   1 needs none; `lfunc.h` makes its own checks). The limits that remain are those of `lroot.h` and `lfunc.h`.
3. **F2.** (a) `identifiers()` finds the `d = gcd(n, p-1)` torsion branches with `nmod_poly_find_distinct_nonzero_roots`
   on `T^d - w^e`: 5.9 s at `p = 65537`, `d = 65536`, and over 100 s at `2^64 - 59`, `d = 299756`. List them instead
   as `t_0 zeta^i`, `i = 0..d-1`, with `zeta = g^((p-1)/d)` for a primitive root `g` (`n_primitive_root_prime`) and
   `t_0` one root found as before or as `w^e`-based: write the proof as a statement R8 (the roots of `T^d - w^e` are
   `t_0 zeta^i`; they are distinct; sorting by identifier gives the order the header promises). The reviewer measured
   0.55 ms for the generator route at `p = 65537`. (b) `roots` decides `LIMIT` for a too small `capacity` or a count
   above `ADF_LROOT_BRANCH_MAX` AFTER the enumeration (5.9 s before a `LIMIT` at `p = 65537`, `n = 65536`); the count
   is `gcd(n, p-1)`, known at once: decide `LIMIT` before any enumeration or evaluation. Say whether
   `ADF_LROOT_BRANCH_MAX` is still needed once no coefficient vector of degree `d` exists; keep the macro if a
   binding may refer to it, and say what it bounds now. (c) The cost the lane named: every branch recomputes `Log`
   and `exp` of the principal unit (1.47 ms per branch, of which 1.2 ms is this); compute `p^j exp(Log(a/p^m)/n)` once
   and multiply by each torsion factor (R2 step 1 is exactly this product). Results must be identical to the current
   ones where the exponent rule gives the same `K` (the stored fixtures check it).
4. **F4.** `docs/api-1f5.md:199`, R7 step 2: "E = M - s - (n-1) j <= M" is false for `j < 0` (`p = 3`, `n = 2`,
   `x = 3^-2 (1 + 3 Z_3)`: `M = -1`, `j = -1`, `E = 0`); limit the sentence to `j >= 0` and say what the rows scaled
   by `shift = -1` of the fixture rely on instead (the scaling, which is correct).

## Order of work (red-green, `lanes/f-repair4/redgreen.log`)

1. Tests first: in `tests/test_lroot.c`, (a) the N-D14 cases: a ball with `N < E` gives exponent `N` and a centre that
   is the root modulo `p^N` (compare with the reviewer's oracle or with the exact image reduced); `N <= j` gives the
   centre 0 at `N`; `N >= E` gives the exact image as before; the fixture comparisons pass `N >= E` (for example
   `ADF_LBALL_EXP_MAX`) so that they still check the exact image; (b) F1: `1 + 2^(2^27) Z_2`, `n = 2`, seed 1, `N`
   large, is `OK` with exponent `2^27 - 1`, and the same at `2^60`; (c) F2: `roots` of exact 1 at 65537 with
   `n = 65536` and capacity 1 returns `LIMIT` in well under a second (a wall-clock guard of 10 s, as
   `test_text_idele` does); all 65536 branches at 65537 with a sufficient capacity are listed in order in under
   10 s and their identifiers are the `d` distinct `d`-th roots of unity modulo `p` (check `t^d = 1` and
   distinctness); the same for `d = 6028` at `2^64 - 59` (the reviewer's second case); (d) the `_at` forms and the
   driver inherit N-D14: a `root_at` command with the driver's `N` on a ball with `E > N` prints exponent `N`
   (say what `N` the driver passes: `tools/adf/adf.c:1770`). Run them red (the current code fails the N-D14, F1
   and F2 timing cases), then the code.
2. The code, then the documents (`lroot.h` comment block, R2, R4, R7, new R8, the line for `N < E`).
3. Checks: `timeout 600 make -j2 BUILD=lanes/f-repair4/build lanes/f-repair4/build/test_lroot
   lanes/f-repair4/build/test_rfunc_prime` and run both; `ASAN_OPTIONS=detect_leaks=0` sanitizer build of the same
   two in `BUILD=lanes/f-repair4/build-san` with `SAN=1`, run both; the reviewer's attacks
   `lanes/f-review6/attack1.py` etc. against your build if they take a library path (read their heads; adapt a copy
   in your lane directory if they do not; expected: the N-D14 cases now differ from N-D13 where `N < E`, and the
   oracle's exact-image test must be applied at `N >= E` only; say how many cases you ran and how many differ, and
   why each difference is right); `sh tests/test_driver.sh` and `sh tests/test_julia.sh` from a tree built in
   `build/` (`timeout 600 make -j2 check` is allowed once at the end, then these two scripts); the four timings of
   the review once (before and after: `polyroots`, `perf`, `nd13 100000` which must now be fast at `N = 10`).
4. Mutation testing of `src/lroot.c`, `lanes/COMMON-C.md` rule 5: at most 60 mutants, `--limit`, `--seed 1`, 2 jobs,
   `--san`, bounded by 20 minutes (`timeout 1300`). A survivor that is a gap of the tests gets a test; the others
   one line each.

Report: `lanes/f-repair4/result.md`, written once, at the end (the harness refuses the name `report.md` for a Claude
subagent); running notes in `lanes/f-repair4/progress.md` as you go. In it: the four repairs and N-D14; every check
with its command and numbers; the timings before and after; the mutation survivors; what is not done; findings
against the specification or the review. Give the same text as your final message.
