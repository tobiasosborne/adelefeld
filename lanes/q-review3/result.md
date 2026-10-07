# Lane q-review3: adversarial review of adf_qclass_reduce (Claude Opus, 2026-10-07)

Reviewed: `adf_qclass_reduce` (`src/qclass.c`, `include/adelefeld/qclass.h`), the union printer
(`adf_qclass_get_str`, `src/text.c`), the driver command `qreduce` (`tools/adf/adf.c`) and the lane's test
`tests/test_qclass_reduce.c`, all as in the worktree (master at 2cb9e57 for the code under review).
This file stands in for `report.md` (a Claude subagent cannot write that name).

## Findings

No BLOCKER and no defect in the reduction, the printer or the driver was found. The two findings are gaps of
the lane's test (hunt 7); three notes on documented behaviour follow.

### R1. MAJOR (test gap): a dropped last piece at `h = integer + tiny` passes `test_qclass_reduce`

- Fault F2: in the first and second pass, when `0 < frac(h) < 2^-100` and `floor(h) > floor(l)`, `stop` is
  set to `floor(h)` instead of `ceil(h)`, so the piece `n = floor(h)`, `[0, frac(h)] x (-n + A Zhat)`, is not
  constructed and not counted.
- The lane's test passes it (exit 0, 150122 checks). No vector has an upper end within `2^-100` above an
  integer.
- Smallest input found: the lift `[-2^-212, 2^-210] x {0}` (midpoint `3 * 2^-213`, radius `5 * 2^-213`,
  finite part the exact 0), prec 53.
  - Correct code: construction count 2; limit 2 gives OK with two pieces, `[1 - 2^-212, 1] x {1}` (rounded,
    midpoint 1) and `[0, 2^-210] x {0}`; limit 1 gives LIMIT.
  - With F2: count 1; limit 1 gives OK with the single piece of midpoint 1, radius `2^-212 (1 + 2^-29)`,
    finite part 1. The input point `(2^-210 ; 0)` is not in the result (it is `(1 + 2^-210 ; 1)` after
    translation by 1, and the stored ball ends at `1 + 2^-212 (1 + 2^-29)`). The enclosure is lost and the
    status is wrong.
- True: "For l_j < h_j enumerate floor(l_j) <= n < ceil(h_j)" (`docs/api-3.md` 2.2, algorithm R step 3) and
  "OK: the exact pre-rounding union equals x's represented set" (header of `adf_qclass_reduce`).
- When the dropped sliver lies next to a long piece, the Q1 spill of that piece above 1 hides the loss; only
  the count shows it. My checker found the fault in 49 of 600 calls (11 wrong statuses, 4 lost enclosures).
- Reproduce: `python3 lanes/q-review3/faults.py --only F2` from the worktree root (needs the archive below).
- Repair of the test: add vectors whose upper end is an integer plus `2^-200`, at a short and a long interval.

### R2. MINOR (test gap): no test reaches a LIMIT of the second pass, so a write to `y` before it is not detected

- Fault F8: `y->form = ADF_QCLASS_PIECES` written at the start of the second pass (after the array is
  allocated, before the last LIMIT exit). The lane's test passes it (exit 0).
- A LIMIT in the second pass is reachable: `q_round` refuses a midpoint or radius above the exact-work bounds.
  Input: the lift `[5/8, 7/8] x (1/3 + 2 Zhat)` (midpoint `3/4`, radius `1/8`), prec `ADF_REAL_PREC_MAX`
  (2097152), limit 10. Correct code: LIMIT, `y` untouched. With F8: LIMIT, `y->form` changed to PIECES
  (`adf_qclass_identical(y, saved)` is 0, `adf_qclass_is_canonical(y)` is 0).
- True: "On LIMIT y is untouched" (header). All LIMIT checks of the lane's test (`refused` in `bounds`,
  `remaining_preflight`, the K-1 calls in `vectors`) leave the first pass.
- Rated MINOR by the brief's rule (the fault changes neither enclosure nor status). It would make the code
  violate a BLOCKER-class promise.
- Reproduce: `python3 lanes/q-review3/faults.py --only F8`, then
  `python3 lanes/q-review3/resource.py --harness lanes/q-review3/fault/bin/harness_F8` (row "prec cap,
  centre 1/3": `R 10 1 1 0 0 0`, that is status 10, form changed, not canonical, not untouched).

### Notes (documented behaviour, no contract broken)

- N1. At `prec = ADF_REAL_PREC_MAX` a piece with a non-dyadic end returns LIMIT: the Q1 midpoint of `p` bits
  has a denominator of `p - e + 1` bits, above `ADF_QCLASS_BITS_MAX` when `p` is at the cap, and the following
  cross-product with the exact end is projected above the bound as well. The input of R2 gives LIMIT at
  2097152 and OK at 2097088 (0.31 s). The header lists "exact-work bounds" as a LIMIT cause, so this is
  covered; the usable precision of `qreduce` is a little below the cap, depending on the end denominators.
- N2. Memory follows the raw count, before deduplication. `[1/2 - 499999998, 1/2 + 499999998] x (1/3 + Zhat)`
  with limit `10^9` has count 999999997 and would store 3 pieces; the call allocates 96 bytes per
  constructed piece first, and under a 4 GB address-space limit the process ends with SIGABRT in
  `flint_malloc` after 0.22 s (no status). The same for the exact finite point. The header says "Memory
  O(K b)", K the raw count, so the caller's limit is the only guard; a limit of `10^9` admits 96 GB.
  A count of `2^62 + 1` with limit `LONG_MAX` returns LIMIT in 10 microseconds (byte-product check).
- N3. Reduction is not idempotent on its own output: every piece that touches 0 or 1 has Q1 spill, so
  re-reduction constructs extra slivers (in 988 re-reductions the piece count grew; 56331 stored pieces were
  checked, none lost). `docs/api-3a.md` says so ("This is not a fixed point of storage").

## Findings against the specification

None. One remark on the brief's wording: the brief asks that "the stored radius exceeds the exact half-width by
at most 2^-28 of it". Q1 (`docs/api-3.md` section 4, step 3) bounds `rho - d <= 2^-28 d` with
`d = max(m - l, h - m) = (h - l)/2 + eta`, not with the half-width `(h - l)/2`; at low precision the half-width
form is false and is not claimed (prec 2, `[0, 9/10]`: `m = 1/2`, `rho = 1/2 + 2^-30`, half-width `9/20`).
I checked the statement as written in Q1, and also the endpoint excess `<= 2 eta + 2^-28 d`. The proof of the
Q1 bound appended to `docs/api-3a.md` ("Q1 radius bound used by this code") was checked step by step: both
cases (`u < 2^e`, `u = 2^e`) and the endpoint-excess identity are correct.

## What was done, and every check with its result

Build (once each): `timeout 600 make -j2 BUILD=lanes/q-review3/build lanes/q-review3/build/libadelefeld.a`
(exit 0) and the same with `SAN=1 INV=1 BUILD=lanes/q-review3/build-san` (exit 0). Harness:
`gcc -std=c11 -O2 -Iinclude lanes/q-review3/harness.c lanes/q-review3/build/libadelefeld.a -lflint -lgmp -lm
-o lanes/q-review3/harness`; the sanitized one with `-O1 -g -fsanitize=address,undefined
-fno-omit-frame-pointer -DADF_CHECK_INVARIANTS` against `build-san`. All build trees and binaries were deleted
at the end.

Oracle (own, not codex): `model.py`.
- Exact containment in `A/Q` without algorithm R: the input ball `[lo,hi] x (a + (A/B) Zhat)` is split into
  the `B` cosets `a + jA/B + A Zhat` (refined to the lcm of all stored moduli); for a coset `z0 + M Zhat`, the
  class of `(s, z)` lies in a stored `[L,U] x (c + H Zhat)` iff some `q` in `c - z0 + H Z` has `s + q` in
  `[L,U]`, because `H Zhat intersect Q = H Z`. Then `[lo,hi]` must be covered by the union of the translated
  intervals (exact sweep). This decides containment of the whole input class, glued points included.
- Point membership for sampled points (end points, points `w/10^9` inside each end, integers in `[lo,hi]` for
  the glue `t = 0 ~ t = 1`, random points), each translated by a random rational.
- Own algorithm R (`docs/api-3.md` 2.2) and own Q1 (section 4) in `fractions.Fraction`; the stored list must
  be EXACTLY equal (midpoint, radius, A, H, d) after sort by `(lo, hi, H, A)` and deduplication; Q1 bound
  `rho - d <= 2^-28 d` and enclosure of each exact constructed piece; own storage predicate (d = 1,
  `0 <= A < H` or H = 0, midpoint in `[0,1]`, keys strictly increasing).
- Self-test (`selftest.py`, 192 real outputs with 2 or more pieces): dropping one stored piece is detected in
  111, shrinking one in 100; I inspected the undetected ones: all were full-image inputs (width >= N, Q5),
  where pieces are redundant.

Hunts (counts are calls of `adf_qclass_reduce`; each case is called with limit = my count and count - 1;
20% of cases also call `reduce(x, x)` and compare):

- Hunts 1 and 2, lifts: `check.py --cases 2000 --seed S --points 200`, S = 101..110. 20000 cases, 40000
  calls, 804564 stored pieces, 4 million sampled points; 0 findings; 3329 OK calls (seeds 104-110, the
  counter was added after seed 103) where deduplication removed pieces and limit count - 1 gave LIMIT.
- Hunts 1 and 2, PIECES: `check.py --pieces --cases 1500 --seed S`, S = 201..203. 4500 cases (1 to 4 pieces,
  spill on both sides, midpoint exactly 0 and 1, moduli 0, 1, 2, 3, 4, 6, 12 mixed), 9000 calls, 24102
  pieces; 0 findings; 478 deduplication cases.
- Hunts 1 and 2, re-reduction: `check.py --twice --cases 250 --seed S`, S = 301..304. 988 outputs reduced
  again, 1976 calls, 56331 pieces; 0 findings; 29 deduplication cases.
- Hunt 3, resources: `resource.py` (one process per case, 70 s timeout, 4 GB address-space limit). 37 cases,
  all with the documented status; slowest 16.9 s (width 10^6, N = 0, 1000001 pieces); N2 above.
- Hunt 4, printer: `printer.py --cases 500 --seed S`, S = 1..4. 2000 values at digits 1, 2, 3, 5, 20, 1000:
  text equal to my printer of conventions 9.4 and 9.5 in all 2000; printed order differs from stored order in
  720 (as 9.4 requires); printed duplicates removed in 75 values, and the removed texts were identical to the
  kept ones in all (so the printed union is the same set); every printed interval encloses its stored ball.
- Hunt 5, memory: `sanrun.py --cases 1000 --seed S`, S = 1..5, and `resource.py --harness ./harness-san
  --as 0`. 4991 cases and the 37 resource cases under ASan, UBSan, LSan (`detect_leaks=1`; LSan checked to
  report a planted 77-byte leak) and INV: exit 0, no report, output equal to that of the plain build.
- Hunt 6, driver: `lanes/q-review3/driver/cases.cmd`, 60 `qreduce` lines through `tools/adf/adf.c` built
  against the lane archive: all 60 as derived by hand below; the SAN driver gives the same output and no
  report.
- Hunt 7, faults: `faults.py`; table below.

Generated cases cover the brief's list: midpoint exponents -260 to 200; widths 0 to 40 (and up to 2^100 in
hunt 3); end points at integers and half-integers ("ends" kind); finite radius in
`0, 1, 2, 3, 12, 360, 1/2, 1/3, 2/3, 3/2, 5/4, 7/360`; centres with denominators up to 360, negative, and of
2000 bits; local backend with blocks 8, 9, 5 (K = 360) and `d` in `1, 2, 3, 5, 7, 11, 13, 720` (radius
`360/d`, so also fractional); prec 2, 3, 10, 20, 30, 53, 64, 128, 300.

Hunt 3 detail (plain build, wall clock): `B = 10^6` with limit `10^6`: OK, 10^6 pieces, 11.3 s; limit
`10^6 - 1`: LIMIT, 0.01 s; limit `LONG_MAX`: OK, 15.2 s; `B = 10^30`, `B = 2^5000`: LIMIT, < 0.01 s;
`A = 2^5000 + 1, B = 2`: OK; width `10^6`, N = 1: OK with 3 stored pieces, 8.7 s, limit `10^6`: LIMIT;
width `2^100`: LIMIT, < 0.01 s; `piece_limit` 0, -1, `LONG_MIN`: LIMIT; prec 2, 1, 0, -5, `LONG_MIN`: OK
(max(prec, 2)); prec cap: OK for a dyadic piece, LIMIT for the R2 input (N1); cap + 1 and `LONG_MAX`: LIMIT;
midpoint exponent edges `+-2^20` and radius exponent edges: as the bounds state; centre with a numerator of
`2^21 - 1` bits: LIMIT (the projected cross-product of `1/2 - c` is `2^21 + 1` bits; the exact result would
have `2^21` bits; conservative projection, as `docs/api-3a.md` states); `y = x`: covered by the alias calls.
Every LIMIT left `y` identical to its sentinel. Under ASan the 10^6-piece cases took 29 to 38 s.

Hunt 6, derivations before the run (digits 2, the driver's default prec unless stated), and the results:
- E1 `(1 +/- 0.1 ; 0 mod 2) + Q with 2`: `[0.9, 1.1]` read as an enclosure; n = 0 gives `[0.9,1] x (0 mod 2)`,
  n = 1 gives `[0, 0.1] x (1 mod 2)`; printed `union((0.05 +/- 0.051 ; 1 mod 2), (0.95 +/- 0.051 ; 0 mod 2))
  + Q`; with 1: LIMIT. Both as derived.
- E2 `(0.5 +/- 0.5 ; 0 mod 2) + Q with 1`: one piece `[0,1]`, Q1 radius `1/2 + 2^-30`:
  `union((0.5 +/- 0.51 ; 0 mod 2)) + Q`. As derived.
- E3 `(0 ; 0 mod 1/2) + Q with 2`: fibres 0 and 1/2 give `(0 ; 0 mod 1)` and `(0.5 ; 0 mod 1)`; with 1: LIMIT.
  As derived.
- E4 `(0 ; 0) + Q with 1`: `union((0 ; 0)) + Q`. As derived.
- Wrap `(1 +/- 0.25 ; 0) + Q with 2`: `[0.75,1] x {0}`, `[0,0.25] x {-1}`; midpoints 0.875 and 0.125 round
  (ties to even) to 0.88 and 0.12, radius `ceil2(1/8 + 2^-33 + 0.005) = 0.14`:
  `union((0.12 +/- 0.14 ; -1), (0.88 +/- 0.14 ; 0)) + Q`; with 1: LIMIT. As derived.
- Several wraps `(0 +/- 1.5 ; 0 mod 5) + Q with 4`: n = -2..1, classes 2, 1, 0, 4; printed order by printed
  lower end, upper end, then A: `union((0.25 +/- 0.26 ; 4 mod 5), (0.5 +/- 0.51 ; 0 mod 5),
  (0.5 +/- 0.51 ; 1 mod 5), (0.75 +/- 0.26 ; 2 mod 5)) + Q`; with 3: LIMIT. As derived.
- Fractional `(0.25 ; 1/3 mod 2/3) + Q with 3`: fibres 1/3, 1, 5/3 give `11/12 x (1 mod 2)`, `1/4 x (1 mod 2)`,
  `7/12 x (0 mod 2)`: `union((0.25 ; 1 mod 2), (0.58 +/- 0.0034 ; 0 mod 2), (0.92 +/- 0.0034 ; 1 mod 2)) + Q`;
  with 2: LIMIT. As derived.
- Non-dyadic end `(0.5 +/- 0.5 ; 1/10) + Q with 2`: `[0.9,1] x {1}` and `[0,0.9] x {0}`:
  `union((0.45 +/- 0.46 ; 0), (0.95 +/- 0.051 ; 1)) + Q`; with 1: LIMIT; at prec 2 the Q1 midpoints are 1 and
  1/2: `union((0.5 +/- 0.51 ; 0), (1 +/- 0.11 ; 1)) + Q`. As derived. `(0.95 +/- 0.05 ; 0 mod 1)` read from
  decimal exceeds 1, so limit 1 gives LIMIT and limit 2 adds a sliver piece `(2.3e-11 +/- 2.4e-11 ; 0 mod 1)`.
- Exact translation: `(1.25 +/- 0.25 ; 1/3 mod 2)`, `(3.25 +/- 0.25 ; 7/3 mod 2)` (by 2) and
  `(1.75 +/- 0.25 ; 5/6 mod 2)` (by 1/2) print the same text, `union((0.083 +/- 0.084 ; 1 mod 2),
  (0.83 +/- 0.18 ; 0 mod 2)) + Q`. As derived.
- Count `k + 1`: `(2.5 +/- 2.5 ; 0 mod 7) + Q with 5` gives five `[0,1]` pieces, classes 0, 3, 4, 5, 6; with 4:
  LIMIT. Point: `(3 ; 0 mod 7) + Q with 1` gives `union((0 ; 4 mod 7)) + Q`. Count before deduplication:
  `(2.5 +/- 2.5 ; 0 mod 1) + Q` with 5 gives one stored piece, with 4 LIMIT. As derived.
- Limits: `LONG_MAX` OK; `2^63`, `2^64`, `-2^63`, -1, 0, `-0`, `10^30`: LIMIT; `1/2`, `-3/2`: DOMAIN; `4/2`: OK
  (read as 2); `2.0`, `1e3`, `0x10`: PARSE; an adele, a class or an fball as the limit: DOMAIN.
- Operands: `0`, `1/3`, an adele, an fball, an idele: DOMAIN; three union forms: UNSUPPORTED; missing or
  extra operand, empty limit: PARSE; `(0 ; 0 mod -1)`: PARSE; `(0 ; 1/2 mod 0)`: read as the point 1/2, gives
  `union((0.5 ; 1)) + Q`; `1/10^6` radius with limit 999999 and with `10^30`: LIMIT; width `2*10^30`: LIMIT.
- Digits 1: `union((0.05 +/- 0.051 ; 1 mod 2), (0.9 +/- 0.11 ; 0 mod 2)) + Q`; digits 3 and 20: as at 2
  where the text allows. Agrees with my printer.

## Fault table (hunt 7)

Each fault in a scratch copy of `src/qclass.c`; `tests/test_qclass_reduce.c` copied beside it (it includes
`../src/qclass.c`) and run from the worktree root; my checker: `check.py --cases 300 --seed 900 --points 20`
against a harness linked with the faulty object.

| Fault | Lane test | Own checker |
|---|---|---|
| F1 upper end `min(h, n+1)` replaced by `n+1` | fails, line 135 | 280 stored lists wrong, 437 too wide |
| F2 last piece dropped when `frac(h) < 2^-100` | **passes** (R1) | 11 wrong statuses, 4 enclosures lost |
| F3 fibres `0 <= j <= B` | fails, line 190 | 300 wrong statuses |
| F4 `-n` with the wrong sign for negative n | fails, line 131 | 120 enclosures lost |
| F5 no Q1 successor when `u` is a power of two | fails, line 135 | 120 stored lists wrong |
| F6 deduplication on the real key only | fails, line 131 | 68 enclosures lost |
| F7 limit compared after deduplication | fails, line 187 | 73 wrong statuses |
| F8 `y->form` written at the start of pass 2 | **passes** (R2) | random: none; resource case: y changed |
| F9 midpoint clamped into `[0,1]` instead of the shift | fails, line 135 | 141 stored lists wrong |
| F10 exponent bound `EXP_MAX + 1` | fails, line 225 | random: none; resource case: OK, not LIMIT |
| F11 (own) count checked after the first fibre only | fails, line 187 | 73 wrong statuses |
| F12 (own) point case `l = h` counts `ceil(h) - floor(l)` | fails, line 130 | crash (non-canonical output) |
| F13 (own) Q1 midpoint rounded toward zero | fails, line 135 | 161 stored lists wrong |

F2 differs from the brief's wording (`h = integer + 2^-200`) only in the threshold `2^-100`.

## Attacked without result

- Enclosure: 25488 inputs reduced (lifts, PIECES, re-reductions), each decided exactly for the whole input
  class, plus more than 4 million sampled rational points; a failure would have been a coset and a real point of
  the input not covered by the translated stored intervals.
- Tightness and count: every stored piece equal to my Q1 of my algorithm R, so `rho - d <= 2^-28 d` and the
  midpoint in `[0,1]` hold for all 884997 stored pieces; status exact at limit = count and count - 1 in all
  calls, including 3836 OK calls where deduplication removed pieces and the limit lay between the stored and the
  constructed count.
- Canonicality: `adf_qclass_is_canonical` and my own predicate on every OK result; never failed.
- Aliasing: about 10000 calls `reduce(x, x)` (20% of the calls of hunts 1 and 2, all 4991 of hunt 5) compared
  with `reduce(y, x)`: identical on OK, `x` unchanged on LIMIT.
- Local backend: about 2500 inputs; output identical in form to global inputs, all checks passed.
- Printer: 2000 values; non-finite real balls are not canonical (`probe_inf.c`), so they cannot reach reduce.
- Code reading: `end_cmp` (sign of a 2-bit `arf_sum` of the exact sum is exact, `arf.rst:638-645`), the mag
  construction of rho (normalised 30-bit mantissa, carry to the next binade), the two-pass allocation and its
  clean-up on every `goto done`, the deduplication swap loop, the order of the LIMIT checks.

## Not done

- Valgrind was not run (LSan works and was used).
- The Julia binding `tests/julia/qclass.jl` was not exercised.
- Fuzzing in the sense of the repository (a long differential run) was not done; the runs above total about
  25 minutes of differential checks with exact containment, not a fuzzer.
- The `tests/ref/vectors/q-slice2/` and `proto/quotient3_checks.py` were not used for any verdict.

## Sources pending

None. Formulas used: algorithm R and Q1 from `docs/api-3.md` 2.2 and 4; printing from `docs/conventions.md` 9.4
and 9.5; FLINT rounding from `refs/src/flint-3.0.1/arf.rst:24-35` and `:638-645`, mag format from
`refs/src/flint-3.0.1/mag.rst:6-15`. The membership rule `H Zhat intersect Q = H Z` is proved in `model.py`.

## Files (lane directory)

- Kept: `harness.c`, `model.py`, `check.py`, `selftest.py`, `printer.py`, `resource.py`, `sanrun.py`,
  `faults.py`, `probe_inf.c`, `driver/cases.cmd`, `logs/*.txt` (44 KB of run summaries), `progress.md`,
  `result.md`.
- Replaced: pi's `harness.c`, `model.py`, `check.py` (rewritten); pi's `probe1.c` to `probe9.c` and
  `work/cases.txt` deleted (unverified FLINT probes, superseded).
- Deleted: `build/`, `build-san/`, the scratch fault tree `fault/`, all binaries. Nothing was written outside
  the lane directory except the harness's own task output files.
- To rerun: build the two archives as above, compile `harness` and `harness-san` as above, then from the lane
  directory `python3 check.py ...`, `printer.py`, `resource.py`, `sanrun.py`; `faults.py` from the worktree
  root.
