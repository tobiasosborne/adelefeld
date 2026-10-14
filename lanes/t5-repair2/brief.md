# Lane t5-repair2: the closure of the milestone 5 review (m5-review1 F1, F2, F3)

The one adversarial review of milestone 5 (lane m5-review1, codex; record `docs/reviews/m5/review-m5.md`, the
same text as `lanes/m5-review1/report.md`; its harnesses in `lanes/m5-review1/`: `extra_checks.py`,
`cap_checks.py`, `faults.py`, `runner.c`, `models.py`, `checks.py`) found 0 BLOCKER, 2 MAJOR, 3 MINOR over
about 450000 checks. F5 (the precision-cap HEADER-FINDING) is closed by decision N-D25 item 6 (`docs/SPEC.md`
15.4; `docs/api-5.md:33-34`), and F4 (a cap-adjacent call over 60 s) is recorded as the cost bead adf-m3bw with
no change. You repair F1, F2 and F3, in that order, each as the smallest change that closes the finding, with
its test seen red first (`lanes/COMMON-C.md` rule 1).

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md`, `docs/reviews/m5/review-m5.md` (F1 to F3 with
their reproduction commands, the fault table, the check-line audit), `docs/api-5.md` section 2 (the local
integrals; P9, P10), `docs/api-5a.md` (the statements of slices 5a and 5b; lines 99-105 state the fixture
width rule), `docs/api-5b.md` (S9, S12 at lines 248-257, S20 at 394-402, the `Check:` lines),
`docs/proofs/analysis.md` P9 to P11 (331-470) and 576-577 (the poles of Lambda for C = 1), `src/localfactor.c`
(the real path: `real_factor` at 274, `gamma_real` at 739, `rgamma_shifted` at 699-727 which is the model of
the repair), `include/adelefeld/localfactor.h` (fixed), `tests/test_tate_local.c`, `tests/test_tate_gamma.c`,
`tests/test_tate_fe.c`, `tools/adf/adf.c` (the handler `adf_drv_tate_fe` near line 3599), `tests/driver/tate-fe.cmd`
and `.out`, `tests/driver/README.md` or `tools/adf/README.md` for the fixture format.

1. **F1 (MAJOR, bead adf-w43f).** `adf_local_tate_at` at the real place for `eta0 = (1,1)` or `(5,4)`, `alpha = 1`,
   `prec = 160`, `S = -3/4 +- 2^-12 (1 +- i)` returns an enclosure 77 times wider than the sampled image (the
   fixture target is 64, `docs/api-5a.md:101-104`): the odd integral is `pi^(-(s+e)/2) Gamma((s+e)/2)` and
   Gamma is evaluated by `acb_gamma` on a rectangle near its pole at `z = (s+e)/2 = -1/8 +- 2^-13`, where FLINT's
   enclosure is loose. Repair in the real path (`real_factor` and whatever it calls; read `gamma_real` too: the
   5b lane already shifts `rgamma` by the recurrence `rgamma(z) = z (z+1) ... (z+n-1) rgamma(z+n)`, lines
   699-727): shift Gamma away from its poles by `Gamma(z) = Gamma(z+n) / (z (z+1) ... (z+n-1))` with `n` chosen so
   that `Re(z+n) >= 1` or `3/2` (an exact division by a product of balls that exclude 0; if any factor contains
   0 the ball meets a pole and the existing DOMAIN or NOT_DETERMINED rule applies: read `localfactor.h` for the
   rule and keep it), and cite `refs/src/flint-3.0.1/acb.rst` for `acb_gamma` and the recurrence source the 5b
   lane cited at `src/localfactor.c:699-705`. Bound `n` by `ADF_LOCAL_ZETA_SHIFT_MAX` as the 5b code does.
   Red: add to `tests/test_tate_gamma.c` or `test_tate_local.c` (where the real-place width tests live: find
   them) the review's two inputs with the factor-64 rule, seen failing before the change; green after. Then
   rerun `taskset -c 0,1 timeout 170 python3 -B lanes/m5-review1/extra_checks.py real` against your build (read
   the script for how it finds the library and the runner; build the runner from `lanes/m5-review1/runner.c`
   against your archive) and report its result: it must exit 0. Every existing vector of
   `tests/ref/vectors/t5-slice1` and `t5-slice3` still passes (the tests run them whole).
2. **F2 (MAJOR against the statements, bead adf-pgpa).** `docs/api-5b.md:250-252` says Lambda is an entire
   function including conductor 1: for `C = 1` it has poles at 0 and 1 (`analysis.md:576-577`); the finiteness at
   the trivial zeros `s0 = -2k` follows from the local regularity of the split S9 at `s0`, not from entireness.
   Rewrite S12's justification and the sentence at `:59` with the conductor-1 qualification. The reference
   description at `:255` writes the completed limit as `2 (-1)^k L'(s0)/k!`; the true limit is
   `2 (-1)^k (pi/C)^k L'(s0)/k!` (the generator applies it: `lanes/t5-slice4/gen_vectors.py:114`; the review's
   numbers: for `chi = (1,1)`, `s0 = -2`, the value is `0.19131329801558517...`, pi times the naked number). Derive
   the factor in one or two lines in the statement (from the definition of Lambda with `(pi/C)^(-s/2)` or
   whatever the design's normalisation is: read `docs/api-5.md` section 3 and cite the line) and correct `:255`.
   No code change; the vectors are right.
3. **F3 (MINOR, bead adf-qnz4).** The production handler `adf_drv_tate_fe` (`tools/adf/adf.c` near 3599) with
   `acb_mul(R, W, B, wp)` replaced by `acb_set(R, A)` passes all five C tests; `docs/api-5b.md:401` claims an
   independence check that exercises the test's own `fe_sides`, not the handler. Add to `tests/driver/tate-fe.cmd`
   one line whose expected output (derived by hand before the run, as the slice did: `lanes/t5-slice5/
   derive_fixture.py`) has visibly different radii for `A` and `R`, so that the copy fault changes the `.out`
   (the review says the existing fixture already distinguishes the printed radii: verify by planting the fault in
   a scratch copy and running `sh tests/test_driver.sh` or its targeted form; if the existing fixture already
   catches it, add no line and say so), and amend the `Check:` line of S20 at `:401` to name the driver fixture
   as the check of the production handler. Replant the fault in a scratch copy at the end and show the driver
   test fails.

**You own:** `src/localfactor.c`, `tests/test_tate_local.c`, `tests/test_tate_gamma.c` (append tests only),
`docs/api-5a.md` (a "Closure of m5-review1" section appended: what changed, the new statement of the real
width), `docs/api-5b.md` (S12, `:59`, `:255`, `:401` only), `tests/driver/tate-fe.cmd` and `tate-fe.out` (append),
`lanes/t5-repair2/`. Everything else is read-only (no header change: HEADER-FINDING if needed; `tools/adf/adf.c`
is not changed). No git command that changes state, no `bd`. At most 2 cores, every program under `timeout`,
builds under `lanes/t5-repair2/build*`, deleted at the end; INV abort children clear their objects. Mutation:
`src/localfactor.c` with `--limit 60`, 20 minutes, 2 jobs, `--san`, survivors listed (`lanes/COMMON-C.md` rule
5). Checks at the end: `test_tate_local`, `test_tate_gamma`, `test_localfactor`, `test_tate`, `test_tate_continue`,
`test_tate_fe` under plain, `SAN=1`, `INV=1`, `CC=clang`; the driver test; NOT check-all. Lines at most 116
characters; files end with a newline; plain sober prose; no LaTeX.

Report `lanes/t5-repair2/report.md`: for each finding the change, the red and green runs with their commands
and counts, the review harness rerun (F1), the fault replant (F3), the mutation table, what is not done.
Aim to finish within about ninety minutes; F1 first.
